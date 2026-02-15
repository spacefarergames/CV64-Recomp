/**
 * @file cv64_window_title.cpp
 * @brief CV64 Window Title Manager Implementation
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#define _CRT_SECURE_NO_WARNINGS

#include "../include/cv64_window_title.h"
#include "../include/cv64_memory_map.h"
#include <Windows.h>
#include <cstring>
#include <cstdio>
#include <atomic>

/*===========================================================================
 * Static State
 *===========================================================================*/

static std::atomic<uint8_t*> s_rdram{nullptr};
static std::atomic<uint32_t> s_rdram_size{0};

static struct {
    uint8_t playerCharacter;    // 0=Reinhardt, 1=Carrie
    uint8_t difficultyMode;     // 0=Easy, 1=Normal
    bool isValid;
    char titleBuffer[256];
} s_state = {0, 0, false, ""};

/*===========================================================================
 * Safe Memory Access
 *===========================================================================*/

static inline uint8_t SafeReadU8(uint32_t addr) {
    uint8_t* rdram = s_rdram.load(std::memory_order_acquire);
    uint32_t size = s_rdram_size.load(std::memory_order_acquire);

    if (!rdram || size == 0) return 0;

    return CV64_ReadU8(rdram, addr);
}

static inline uint16_t SafeReadU16(uint32_t addr) {
    uint8_t* rdram = s_rdram.load(std::memory_order_acquire);
    uint32_t size = s_rdram_size.load(std::memory_order_acquire);

    if (!rdram || size == 0) return 0;

    return CV64_ReadU16(rdram, addr);
}

/*===========================================================================
 * API Implementation
 *===========================================================================*/

void CV64_WindowTitle_Init(uint8_t* rdram, uint32_t rdram_size) {
    s_rdram.store(rdram, std::memory_order_release);
    s_rdram_size.store(rdram_size, std::memory_order_release);
    
    s_state.playerCharacter = 0xFF;
    s_state.difficultyMode = 0xFF;
    s_state.isValid = false;
    s_state.titleBuffer[0] = '\0';
}

const char* CV64_WindowTitle_GetPlayerName(void) {
    switch (s_state.playerCharacter) {
        case 0: return "Reinhardt";
        case 1: return "Carrie";
        default: return "Unknown";
    }
}

const char* CV64_WindowTitle_GetDifficultyName(void) {
    switch (s_state.difficultyMode) {
        case 0: return "Easy";
        case 1: return "Normal";
        default: return "Unknown";
    }
}

bool CV64_WindowTitle_IsGameActive(void) {
    return s_state.isValid && 
           (s_state.playerCharacter <= 1) && 
           (s_state.difficultyMode <= 1);
}

const char* CV64_WindowTitle_Update(void) {
    // Read current game state — player character is s16 in decomp (LH instruction)
    uint16_t character = SafeReadU16(CV64_ADDR_PLAYER_CHARACTER);
    uint8_t difficulty = SafeReadU8(CV64_ADDR_DIFFICULTY_MODE);
    
    // Validate values (0 or 1 only)
    uint8_t charVal = (character > 1) ? 0xFF : (uint8_t)character;
    if (difficulty > 1) difficulty = 0xFF;
    
    // Check if state changed or needs initialization
    bool stateChanged = (charVal != s_state.playerCharacter) || 
                        (difficulty != s_state.difficultyMode);

    if (stateChanged || s_state.titleBuffer[0] == '\0') {
        s_state.playerCharacter = charVal;
        s_state.difficultyMode = difficulty;
        s_state.isValid = (charVal <= 1) && (difficulty <= 1);
        
        // Build title string
        if (s_state.isValid) {
            const char* playerName = CV64_WindowTitle_GetPlayerName();
            const char* difficultyName = CV64_WindowTitle_GetDifficultyName();
            
            #ifdef PLUGIN_REVISION
            # ifdef _DEBUG
            sprintf_s(s_state.titleBuffer, sizeof(s_state.titleBuffer),
                "Castlevania 64 Recomp - %s (%s) [DEBUG] - Rev %s",
                playerName, difficultyName, PLUGIN_REVISION);
            # else
            sprintf_s(s_state.titleBuffer, sizeof(s_state.titleBuffer),
                "Castlevania 64 Recomp - %s (%s) - Rev %s",
                playerName, difficultyName, PLUGIN_REVISION);
            # endif
            #else
            # ifdef _DEBUG
            sprintf_s(s_state.titleBuffer, sizeof(s_state.titleBuffer),
                "Castlevania 64 Recomp - %s (%s) [DEBUG]",
                playerName, difficultyName);
            # else
            sprintf_s(s_state.titleBuffer, sizeof(s_state.titleBuffer),
                "Castlevania 64 Recomp - %s (%s)",
                playerName, difficultyName);
            # endif
            #endif
        } else {
            // Not in game yet or invalid state
            #ifdef PLUGIN_REVISION
            # ifdef _DEBUG
            sprintf_s(s_state.titleBuffer, sizeof(s_state.titleBuffer),
                "Castlevania 64 Recomp [DEBUG] - Rev %s", PLUGIN_REVISION);
            # else
            sprintf_s(s_state.titleBuffer, sizeof(s_state.titleBuffer),
                "Castlevania 64 Recomp - Rev %s", PLUGIN_REVISION);
            # endif
            #else
            # ifdef _DEBUG
            sprintf_s(s_state.titleBuffer, sizeof(s_state.titleBuffer),
                "Castlevania 64 Recomp [DEBUG]");
            # else
            sprintf_s(s_state.titleBuffer, sizeof(s_state.titleBuffer),
                "Castlevania 64 Recomp");
            # endif
            #endif
        }
    }
    
    return s_state.titleBuffer;
}
