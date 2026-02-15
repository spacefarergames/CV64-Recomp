/**
 * @file cv64_audio.h
 * @brief Castlevania 64 PC Recomp - Enhanced Audio System
 * 
 * Provides modern audio capabilities:
 * - High quality audio output
 * - Configurable sample rate
 * - Volume controls
 * - Audio enhancement options
 * - Original N64 audio compatibility
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_AUDIO_H
#define CV64_AUDIO_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Audio Backend Types
 *===========================================================================*/

typedef enum CV64_AudioBackend {
    CV64_AUDIO_BACKEND_AUTO = 0,    /**< Auto-detect best backend */
    CV64_AUDIO_BACKEND_WASAPI,      /**< Windows WASAPI */
    CV64_AUDIO_BACKEND_DIRECTSOUND, /**< DirectSound */
    CV64_AUDIO_BACKEND_OPENAL,      /**< OpenAL */
    CV64_AUDIO_BACKEND_SDL,         /**< SDL Audio */
    CV64_AUDIO_BACKEND_COUNT
} CV64_AudioBackend;

/*===========================================================================
 * Sample Rate Presets
 *===========================================================================*/

typedef enum CV64_SampleRate {
    CV64_SAMPLERATE_NATIVE = 0,     /**< Original N64 rate */
    CV64_SAMPLERATE_22050 = 22050,  /**< 22.05 kHz */
    CV64_SAMPLERATE_44100 = 44100,  /**< 44.1 kHz (CD quality) */
    CV64_SAMPLERATE_48000 = 48000,  /**< 48 kHz */
    CV64_SAMPLERATE_96000 = 96000,  /**< 96 kHz */
} CV64_SampleRate;

/*===========================================================================
 * Audio Configuration Structure
 *===========================================================================*/

typedef struct CV64_AudioConfig {
    /* Backend selection */
    CV64_AudioBackend backend;
    
    /* Basic settings */
    CV64_SampleRate sample_rate;
    u32 buffer_size;                /**< Buffer size in samples */
    u32 latency_ms;                 /**< Target latency in ms */
    
    /* Volume controls (0.0 - 1.0) */
    f32 master_volume;
    f32 music_volume;
    f32 sfx_volume;
    f32 voice_volume;
    f32 ambient_volume;
    
    /* Enhancement options */
    bool enable_reverb;
    bool enable_surround;
    bool enable_bass_boost;
    f32 reverb_amount;
    f32 bass_boost_amount;
    
    /* N64 accuracy options */
    bool accurate_n64_audio;
    bool enable_hle_audio;          /**< High-level audio emulation */
    
    /* Music enhancement */
    bool enable_hd_music;           /**< Load HD music packs */
    char hd_music_path[260];
    
} CV64_AudioConfig;

/*===========================================================================
 * Sound Effect Structure
 *===========================================================================*/

typedef struct CV64_SoundEffect {
    u32 id;                         /**< Sound effect ID */
    s16* samples;                   /**< Sample data */
    u32 sample_count;               /**< Number of samples */
    u32 sample_rate;                /**< Sample rate */
    u8 channels;                    /**< Channel count (1=mono, 2=stereo) */
    f32 volume;                     /**< Playback volume */
    f32 pitch;                      /**< Playback pitch */
    Vec3f position;                 /**< 3D position */
    bool is_3d;                     /**< TRUE for 3D positioned sound */
    bool looping;                   /**< TRUE for looping sound */
} CV64_SoundEffect;

/*===========================================================================
 * Music Track Structure
 *===========================================================================*/

typedef struct CV64_MusicTrack {
    u32 id;                         /**< Music track ID */
    const char* name;               /**< Track name */
    bool is_playing;
    bool is_looping;
    f32 current_time;               /**< Current playback time */
    f32 duration;                   /**< Total duration */
    f32 fade_time;                  /**< Fade transition time */
} CV64_MusicTrack;

/*===========================================================================
 * Audio API Functions
 *===========================================================================*/

/**
 * @brief Initialize audio system
 * @return TRUE on success
 */
CV64_API bool CV64_Audio_Init(void);

/**
 * @brief Shutdown audio system
 */
CV64_API void CV64_Audio_Shutdown(void);

/**
 * @brief Update audio system (call each frame)
 */
CV64_API void CV64_Audio_Update(void);

/**
 * @brief Pause all audio
 */
CV64_API void CV64_Audio_Pause(void);

/**
 * @brief Resume all audio
 */
CV64_API void CV64_Audio_Resume(void);

/*===========================================================================
 * Sound Effect Functions
 *===========================================================================*/

/**
 * @brief Play a sound effect
 * @param sfx_id Sound effect ID
 * @return Handle to playing sound, or 0 on failure
 */
CV64_API u32 CV64_Audio_PlaySFX(u32 sfx_id);

/**
 * @brief Play a 3D positioned sound effect
 * @param sfx_id Sound effect ID
 * @param position 3D position
 * @return Handle to playing sound
 */
CV64_API u32 CV64_Audio_PlaySFX3D(u32 sfx_id, Vec3f* position);

/**
 * @brief Stop a playing sound
 * @param handle Sound handle
 */
CV64_API void CV64_Audio_StopSFX(u32 handle);

/**
 * @brief Set sound effect volume
 * @param handle Sound handle
 * @param volume Volume (0.0 - 1.0)
 */
CV64_API void CV64_Audio_SetSFXVolume(u32 handle, f32 volume);

/**
 * @brief Set sound effect position
 * @param handle Sound handle
 * @param position New position
 */
CV64_API void CV64_Audio_SetSFXPosition(u32 handle, Vec3f* position);

/*===========================================================================
 * Music Functions
 *===========================================================================*/

/**
 * @brief Play music track
 * @param track_id Music track ID
 * @param loop TRUE to loop
 */
CV64_API void CV64_Audio_PlayMusic(u32 track_id, bool loop);

/**
 * @brief Stop current music
 * @param fade_time Fade out time in seconds (0 for immediate)
 */
CV64_API void CV64_Audio_StopMusic(f32 fade_time);

/**
 * @brief Pause music
 */
CV64_API void CV64_Audio_PauseMusic(void);

/**
 * @brief Resume music
 */
CV64_API void CV64_Audio_ResumeMusic(void);

/**
 * @brief Crossfade to new music track
 * @param track_id New music track ID
 * @param fade_time Crossfade time in seconds
 * @param loop TRUE to loop new track
 */
CV64_API void CV64_Audio_CrossfadeMusic(u32 track_id, f32 fade_time, bool loop);

/**
 * @brief Get current music info
 * @return Pointer to current music track info, or NULL
 */
CV64_API CV64_MusicTrack* CV64_Audio_GetCurrentMusic(void);

/*===========================================================================
 * Volume Control Functions
 *===========================================================================*/

/**
 * @brief Set master volume
 * @param volume Volume (0.0 - 1.0)
 */
CV64_API void CV64_Audio_SetMasterVolume(f32 volume);

/**
 * @brief Set music volume
 * @param volume Volume (0.0 - 1.0)
 */
CV64_API void CV64_Audio_SetMusicVolume(f32 volume);

/**
 * @brief Set sound effects volume
 * @param volume Volume (0.0 - 1.0)
 */
CV64_API void CV64_Audio_SetSFXMasterVolume(f32 volume);

/**
 * @brief Get master volume
 * @return Current master volume
 */
CV64_API f32 CV64_Audio_GetMasterVolume(void);

/*===========================================================================
 * Listener Functions (for 3D audio)
 *===========================================================================*/

/**
 * @brief Set listener position
 * @param position Listener position
 */
CV64_API void CV64_Audio_SetListenerPosition(Vec3f* position);

/**
 * @brief Set listener orientation
 * @param forward Forward direction
 * @param up Up direction
 */
CV64_API void CV64_Audio_SetListenerOrientation(Vec3f* forward, Vec3f* up);

/*===========================================================================
 * Configuration Functions
 *===========================================================================*/

/**
 * @brief Get current audio configuration
 * @return Pointer to configuration
 */
CV64_API CV64_AudioConfig* CV64_Audio_GetConfig(void);

/**
 * @brief Apply configuration changes
 * @return TRUE on success
 */
CV64_API bool CV64_Audio_ApplyConfig(void);

/**
 * @brief Load audio configuration from file
 * @param filepath Path to config file (NULL for default)
 * @return TRUE on success
 */
CV64_API bool CV64_Audio_LoadConfig(const char* filepath);

/**
 * @brief Save audio configuration to file
 * @param filepath Path to config file (NULL for default)
 * @return TRUE on success
 */
CV64_API bool CV64_Audio_SaveConfig(const char* filepath);

/*===========================================================================
 * HD Audio Functions
 *===========================================================================*/

/**
 * @brief Load HD music pack
 * @param pack_path Path to music pack
 * @return TRUE on success
 */
CV64_API bool CV64_Audio_LoadHDMusicPack(const char* pack_path);

/**
 * @brief Enable/disable HD music
 * @param enabled TRUE to enable
 */
CV64_API void CV64_Audio_SetHDMusicEnabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* CV64_AUDIO_H */
