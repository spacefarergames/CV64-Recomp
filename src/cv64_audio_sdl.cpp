/**
 * @file cv64_audio_sdl.cpp
 * @brief SDL-based audio plugin for CV64_RMG with SFX support (SDL2-compatible)
 */

#define _CRT_SECURE_NO_WARNINGS

#ifdef CV64_STATIC_MUPEN64PLUS

#include "../include/cv64_threading.h"
#include <Windows.h>
#include <SDL.h>
#include <cstring>
#include <cstdio>
#include <algorithm>

extern "C" {
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_plugin.h"
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_types.h"
}

/*===========================================================================
 * Configuration
 *===========================================================================*/

#define AUDIO_BUFFER_SIZE 16384
#define SDL_AUDIO_SAMPLES 2048
#define DEFAULT_FREQUENCY 33600
#define PRIMARY_BUFFER_TARGET 2048

 /*===========================================================================
  * State
  *===========================================================================*/

static SDL_AudioSpec g_obtainedSpec;

static struct {
    bool initialized;
    bool romOpen;
    SDL_AudioDeviceID deviceId;

    AUDIO_INFO audioInfo;

    int frequency;
    int speedFactor;
    int volume;
    bool muted;

    uint8_t* buffer;
    int bufferSize;
    volatile int readPos;
    volatile int writePos;
    volatile int bufferLevel;

    SDL_mutex* mutex;
} g_audio = {
    false, false, 0,
    {},
    DEFAULT_FREQUENCY, 100, 100, false,
    nullptr, 0, 0, 0, 0,
    nullptr
};

/*===========================================================================
 * Logging
 *===========================================================================*/

static void AudioLog(const char* msg) {
    OutputDebugStringA("[CV64_AUDIO_SDL] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

/*===========================================================================
 * Correct DLL-based asset path (fixes intermittent WAV loading)
 *===========================================================================*/

static void BuildAssetPath(char* outPath, size_t outSize, const char* filename)
{
    char modulePath[MAX_PATH];

    HMODULE hMod = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&BuildAssetPath,
        &hMod
    );

    GetModuleFileNameA(hMod, modulePath, MAX_PATH);

    // Strip DLL name
    char* lastSlash = strrchr(modulePath, '\\');
    if (lastSlash)
        *lastSlash = '\0';

    // Build: <dll folder>\assets\filename
    snprintf(outPath, outSize, "%s\\assets\\%s", modulePath, filename);
}

/*===========================================================================
 * Sound Effects System
 *===========================================================================*/

struct LoadedSFX {
    Uint8* data = nullptr;
    Uint32 length = 0;
    Uint32 position = 0;
    bool playing = false;
};

static LoadedSFX g_sfx_itemPickup;
static LoadedSFX g_sfx_powerupPickup;
static LoadedSFX g_sfx_goldPickup;   // NEW

static bool LoadSFX(LoadedSFX& sfx, const char* filename)
{
    char fullPath[MAX_PATH];
    BuildAssetPath(fullPath, sizeof(fullPath), filename);

    char msg[256];
    sprintf(msg, "Loading SFX: %s", fullPath);
    AudioLog(msg);

    SDL_AudioSpec wavSpec;
    Uint8* wavBuffer = nullptr;
    Uint32 wavLength = 0;

    if (SDL_LoadWAV(fullPath, &wavSpec, &wavBuffer, &wavLength) == nullptr) {
        sprintf(msg, "Failed to load WAV: %s", SDL_GetError());
        AudioLog(msg);
        return false;
    }

    SDL_AudioSpec deviceSpec = g_obtainedSpec;

    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(
        &cvt,
        wavSpec.format, wavSpec.channels, wavSpec.freq,
        deviceSpec.format, deviceSpec.channels, deviceSpec.freq
    ) < 0)
    {
        SDL_FreeWAV(wavBuffer);
        AudioLog("SDL_BuildAudioCVT failed");
        return false;
    }

    cvt.len = wavLength;
    cvt.buf = (Uint8*)SDL_malloc(cvt.len * cvt.len_mult);
    memcpy(cvt.buf, wavBuffer, wavLength);

    SDL_FreeWAV(wavBuffer);

    if (SDL_ConvertAudio(&cvt) < 0) {
        SDL_free(cvt.buf);
        AudioLog("SDL_ConvertAudio failed");
        return false;
    }

    sfx.data = cvt.buf;
    sfx.length = cvt.len_cvt;
    sfx.position = 0;
    sfx.playing = false;

    sprintf(msg, "Loaded OK (%u bytes)", sfx.length);
    AudioLog(msg);

    return true;
}

/*===========================================================================
 * SDL Audio Callback
 *===========================================================================*/

static void SDLCALL AudioCallback(void* userdata, Uint8* stream, int len) {
    (void)userdata;

    if (!g_audio.mutex) {
        memset(stream, 0, len);
        return;
    }

    SDL_LockMutex(g_audio.mutex);

    int available = g_audio.bufferLevel;
    int toCopy = (len < available) ? len : available;

    if (toCopy > 0 && !g_audio.muted) {
        int firstPart = g_audio.bufferSize - g_audio.readPos;
        if (firstPart >= toCopy) {
            memcpy(stream, g_audio.buffer + g_audio.readPos, toCopy);
            g_audio.readPos = (g_audio.readPos + toCopy) % g_audio.bufferSize;
        }
        else {
            memcpy(stream, g_audio.buffer + g_audio.readPos, firstPart);
            memcpy(stream + firstPart, g_audio.buffer, toCopy - firstPart);
            g_audio.readPos = toCopy - firstPart;
        }
        g_audio.bufferLevel -= toCopy;

        if (g_audio.volume < 100) {
            int16_t* samples = (int16_t*)stream;
            int numSamples = toCopy / 2;
            for (int i = 0; i < numSamples; i++) {
                samples[i] = (int16_t)((samples[i] * g_audio.volume) / 100);
            }
        }

        if (toCopy < len) {
            memset(stream + toCopy, 0, len - toCopy);
        }
    }
    else {
        memset(stream, 0, len);
    }

    /* Mix SFX */
    auto mixSFX = [&](LoadedSFX& sfx) {
        if (!sfx.playing || !sfx.data) return;

        Uint32 remaining = sfx.length - sfx.position;
        Uint32 toMix = (remaining < (Uint32)len) ? remaining : (Uint32)len;

        SDL_MixAudioFormat(
            stream,
            sfx.data + sfx.position,
            AUDIO_S16SYS,
            toMix,
            SDL_MIX_MAXVOLUME
        );

        sfx.position += toMix;
        if (sfx.position >= sfx.length) {
            sfx.playing = false;
            sfx.position = 0;
        }
        };

    mixSFX(g_sfx_itemPickup);
    mixSFX(g_sfx_powerupPickup);
    mixSFX(g_sfx_goldPickup);   // NEW

    SDL_UnlockMutex(g_audio.mutex);
}

/*===========================================================================
 * Plugin API
 *===========================================================================*/

extern "C" {

    m64p_error cv64audio_PluginGetVersion(m64p_plugin_type* PluginType, int* PluginVersion,
        int* APIVersion, const char** PluginNamePtr, int* Capabilities) {
        if (PluginType) *PluginType = M64PLUGIN_AUDIO;
        if (PluginVersion) *PluginVersion = 0x020000;
        if (APIVersion) *APIVersion = 0x020000;
        if (PluginNamePtr) *PluginNamePtr = "CV64 SDL Audio";
        if (Capabilities) *Capabilities = 0;
        return M64ERR_SUCCESS;
    }

    void cv64audio_AiDacrateChanged(int SystemType) {
        if (!g_audio.audioInfo.RDRAM) return;

        int f = g_audio.frequency;
        switch (SystemType) {
        case SYSTEM_NTSC:
            f = 48681812 / (*g_audio.audioInfo.AI_DACRATE_REG + 1);
            break;
        case SYSTEM_PAL:
            f = 49656530 / (*g_audio.audioInfo.AI_DACRATE_REG + 1);
            break;
        case SYSTEM_MPAL:
            f = 48628316 / (*g_audio.audioInfo.AI_DACRATE_REG + 1);
            break;
        }

        g_audio.frequency = f;
    }

    void cv64audio_AiLenChanged(void) {
        if (!g_audio.audioInfo.RDRAM || !g_audio.romOpen) return;

        uint32_t address = *g_audio.audioInfo.AI_DRAM_ADDR_REG & 0xFFFFFF;
        uint32_t length = *g_audio.audioInfo.AI_LEN_REG & 0x3FFFF;

        if (length == 0) return;

        uint8_t* source = (uint8_t*)(g_audio.audioInfo.RDRAM + address);

        if (CV64_Threading_IsAsyncGraphicsEnabled()) {
            int16_t* samples = (int16_t*)source;
            size_t sampleCount = length / 2;

            if (CV64_Audio_QueueSamples(samples, sampleCount, g_audio.frequency)) {
                CV64_Audio_OnDMAComplete();
                return;
            }
        }

        SDL_LockMutex(g_audio.mutex);

        int available = g_audio.bufferSize - g_audio.bufferLevel;
        int toCopy = ((int)length < available) ? (int)length : available;

        if (toCopy > 0) {
            int firstPart = g_audio.bufferSize - g_audio.writePos;
            if (firstPart >= toCopy) {
                memcpy(g_audio.buffer + g_audio.writePos, source, toCopy);
                g_audio.writePos = (g_audio.writePos + toCopy) % g_audio.bufferSize;
            }
            else {
                memcpy(g_audio.buffer + g_audio.writePos, source, firstPart);
                memcpy(g_audio.buffer, source + firstPart, toCopy - firstPart);
                g_audio.writePos = toCopy - firstPart;
            }
            g_audio.bufferLevel += toCopy;
        }

        SDL_UnlockMutex(g_audio.mutex);
    }

    int cv64audio_InitiateAudio(AUDIO_INFO Audio_Info) {
        g_audio.audioInfo = Audio_Info;

        if (!g_audio.initialized) {
            if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
                return 0;
            }

            g_audio.mutex = SDL_CreateMutex();
            g_audio.bufferSize = AUDIO_BUFFER_SIZE;
            g_audio.buffer = (uint8_t*)malloc(g_audio.bufferSize);
            memset(g_audio.buffer, 0, g_audio.bufferSize);

            SDL_AudioSpec desired, obtained;
            memset(&desired, 0, sizeof(desired));
            desired.freq = 44100;
            desired.format = AUDIO_S16SYS;
            desired.channels = 2;
            desired.samples = SDL_AUDIO_SAMPLES;
            desired.callback = AudioCallback;

            g_audio.deviceId = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
            if (g_audio.deviceId == 0) {
                return 0;
            }

            g_obtainedSpec = obtained;

            g_audio.initialized = true;

            LoadSFX(g_sfx_itemPickup, "ItemPickup.wav");
            LoadSFX(g_sfx_powerupPickup, "PowerUpPickup.wav");
            LoadSFX(g_sfx_goldPickup, "GoldPickup.wav");   // NEW
        }

        return 1;
    }

    void cv64audio_ProcessAList(void) {}

    void cv64audio_RomClosed(void) {
        if (g_audio.deviceId) {
            SDL_PauseAudioDevice(g_audio.deviceId, 1);
        }

        if (g_audio.mutex) {
            SDL_LockMutex(g_audio.mutex);
            g_audio.readPos = 0;
            g_audio.writePos = 0;
            g_audio.bufferLevel = 0;
            if (g_audio.buffer) {
                memset(g_audio.buffer, 0, g_audio.bufferSize);
            }
            SDL_UnlockMutex(g_audio.mutex);
        }

        g_audio.romOpen = false;
    }

    int cv64audio_RomOpen(void) {
        g_audio.romOpen = true;
        g_audio.frequency = DEFAULT_FREQUENCY;
        g_audio.readPos = 0;
        g_audio.writePos = 0;
        g_audio.bufferLevel = 0;

        if (g_audio.deviceId) {
            SDL_PauseAudioDevice(g_audio.deviceId, 0);
        }

        return 1;
    }

    void cv64audio_SetSpeedFactor(int percentage) {
        g_audio.speedFactor = percentage;
    }

    void cv64audio_VolumeUp(void) {
        g_audio.volume = (std::min)(g_audio.volume + 5, 100);
    }

    void cv64audio_VolumeDown(void) {
        g_audio.volume = (std::max)(g_audio.volume - 5, 0);
    }

    int cv64audio_VolumeGetLevel(void) {
        return g_audio.muted ? 0 : g_audio.volume;
    }

    void cv64audio_VolumeSetLevel(int level) {
        g_audio.volume = (std::max)(0, (std::min)(level, 100));
        g_audio.muted = false;
    }

    void cv64audio_VolumeMute(void) {
        g_audio.muted = !g_audio.muted;
    }

    const char* cv64audio_VolumeGetString(void) {
        static char volumeStr[16];
        if (g_audio.muted) {
            return "Muted";
        }
        sprintf(volumeStr, "%d%%", g_audio.volume);
        return volumeStr;
    }

    void cv64audio_Shutdown(void) {
        if (g_audio.deviceId) {
            SDL_CloseAudioDevice(g_audio.deviceId);
            g_audio.deviceId = 0;
        }

        if (g_audio.mutex) {
            SDL_DestroyMutex(g_audio.mutex);
            g_audio.mutex = nullptr;
        }

        if (g_audio.buffer) {
            free(g_audio.buffer);
            g_audio.buffer = nullptr;
        }

        if (g_audio.initialized) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
            g_audio.initialized = false;
        }
    }

    /*===========================================================================
     * Public SFX API
     *===========================================================================*/

    void CV64_PlayItemPickupSFX()
    {
        g_sfx_itemPickup.position = 0;
        g_sfx_itemPickup.playing = true;
    }

    void CV64_PlayPowerupPickupSFX()
    {
        g_sfx_powerupPickup.position = 0;
        g_sfx_powerupPickup.playing = true;
    }

    void CV64_PlayGoldPickupSFX()   // NEW
    {
        g_sfx_goldPickup.position = 0;
        g_sfx_goldPickup.playing = true;
    }

} /* extern "C" */

#endif /* CV64_STATIC_MUPEN64PLUS */