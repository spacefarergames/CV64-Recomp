/**
 * @file cv64_window_title.h
 * @brief CV64 Window Title Manager - Shows Player Name & Difficulty
 * 
 * Reads game state from memory and updates window title to show:
 * - Player character (Reinhardt or Carrie)
 * - Difficulty mode (Easy or Normal)
 * - Game title
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_WINDOW_TITLE_H
#define CV64_WINDOW_TITLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * Player & Difficulty Memory Addresses
 * Verified against CASTLEVANIA.sym decomp symbols, v1.0 USA cheat codes
 * (CRC 4BCDFF47-AAA3AF8F), and ROM disassembly of baserom.z64
 *
 * These are within system_work (0x80363AB8) save data area.
 *===========================================================================*/

/* Player character (s16 within system_work save data)
 * Address: 0x80389C3C (system_work + 0x26184)
 * 0 = Reinhardt Schneider
 * 1 = Carrie Fernandez
 * Verified: getCurrentCharacter() at 0x8003CA24 reads LH from 0x80389C3C
 */
#define CV64_ADDR_PLAYER_CHARACTER  0x80389C3C

/* Difficulty setting
 * Address: 0x80389CC8 (system_work + 0x26210)
 * sym: save_difficulty
 */
#define CV64_ADDR_DIFFICULTY_MODE   0x80389CC8

/* Current map/level (s16)
 * Address: 0x80389EE0 (system_work + 0x26428)
 * sym: map_ID (live value, not stale map_ID_copy)
 * Non-zero when a level is loaded (in-game)
 */
#define CV64_ADDR_CURRENT_MAP       0x80389EE0

/* Player health (u16)
 * Address: 0x80389C3E (system_work + 0x26186)
 * sym: Player_health
 */
#define CV64_ADDR_PLAYER_HEALTH     0x80389C3E

/*===========================================================================
 * API Functions
 *===========================================================================*/

/**
 * @brief Initialize window title manager
 * @param rdram Pointer to N64 RDRAM
 * @param rdram_size Size of RDRAM
 */
void CV64_WindowTitle_Init(uint8_t* rdram, uint32_t rdram_size);

/**
 * @brief Update window title if game state changed
 * Call this every frame or when you want to update the title
 * @return Updated title string (valid until next call)
 */
const char* CV64_WindowTitle_Update(void);

/**
 * @brief Get player character name
 * @return "Reinhardt", "Carrie", or "Unknown"
 */
const char* CV64_WindowTitle_GetPlayerName(void);

/**
 * @brief Get difficulty mode name  
 * @return "Easy", "Normal", or "Unknown"
 */
const char* CV64_WindowTitle_GetDifficultyName(void);

/**
 * @brief Check if game state is available
 * @return true if in-game with valid character/difficulty
 */
bool CV64_WindowTitle_IsGameActive(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_WINDOW_TITLE_H */
