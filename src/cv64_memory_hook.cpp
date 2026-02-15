/**
 * @file cv64_memory_hook.cpp
 * @brief Castlevania 64 PC Recomp - Memory Hook Implementation
 * 
 * Hooks into N64 RDRAM to read/write game state for our patches.
 * Uses memory addresses from cv64 decomp project.
 * 
 * SAFETY NOTES:
 * - All memory access is guarded with NULL checks and bounds validation
 * - Thread-safe access to RDRAM pointer via atomic operations
 * - Addresses are validated before every read/write
 * - Failed operations return safe default values
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#define _CRT_SECURE_NO_WARNINGS

#include "../include/cv64_memory_hook.h"
#include "../include/cv64_memory_map.h"
#include "../include/cv64_camera_patch.h"
#include "../include/cv64_controller.h"
#include <Windows.h>
#include <cstring>
#include <cstdio>
#include <atomic>

/*===========================================================================
 * mupen64plus-core internals for dynarec cache invalidation
 * g_dev is the global device struct; r4300 is its FIRST member so &g_dev
 * can be cast to struct r4300_core* safely.
 *===========================================================================*/
extern "C" {
    struct r4300_core;   /* opaque */
    struct device;       /* opaque */
    extern struct device g_dev;
    void invalidate_r4300_cached_code(struct r4300_core* r4300, uint32_t address, size_t size);
}

/*===========================================================================
 * Static Variables - Thread-Safe RDRAM Access
 *===========================================================================*/

static std::atomic<u8*> s_rdram{nullptr};
static std::atomic<u32> s_rdram_size{0};
static std::atomic<bool> s_initialized{false};

/* Cached pointers to game structures (updated each frame) */
static u32 s_system_work_addr = CV64_ADDR_SYSTEM_WORK;
static std::atomic<u32> s_camera_mgr_addr{0};
static std::atomic<u32> s_player_addr{0};

/* Game state cache */
static std::atomic<u32> s_current_camera_mode{0};
static std::atomic<u32> s_current_menu{0};
static std::atomic<bool> s_is_in_gameplay{false};

/* Camera yaw offset — accumulated user rotation for mode 0 free-look */
static std::atomic<s32> s_camera_yaw_offset{0};

/* Camera zoom multiplier — accumulated from mouse wheel for distance control.
 * 1.0 = default distance, <1.0 = closer, >1.0 = farther. */
static std::atomic<s32> s_camera_zoom_mult_x1000{1000};  /* stored as int*1000 for atomic */
#define CV64_ZOOM_MIN  300   /* 0.3x */
#define CV64_ZOOM_MAX  3000  /* 3.0x */

/* Enhanced state tracking */
static std::atomic<CV64_GameState> s_game_state{CV64_STATE_NOT_RUNNING};
static std::atomic<s16> s_last_map_id{-1};
static CV64_MapChangeCallback s_map_change_callback = nullptr;
static std::atomic<bool> s_is_soft_reset{false};
static std::atomic<u32> s_game_time{0};
static std::atomic<bool> s_is_daytime{true};

/*===========================================================================
 * Forward Declarations
 *===========================================================================*/

static s16 ReadCurrentMapId(void);
static CV64_GameState ComputeGameState(u8* rdram, s16 mapId);

/*===========================================================================
 * Debug Logging
 *===========================================================================*/

static void LogInfo(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    OutputDebugStringA("[CV64_MEM] ");
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}

static void LogWarning(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    OutputDebugStringA("[CV64_MEM WARNING] ");
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}

/*===========================================================================
 * Safe Memory Access Helpers
 *===========================================================================*/

/**
 * @brief Get thread-safe RDRAM pointer with validation
 * @param out_size Optional output for RDRAM size
 * @return Valid RDRAM pointer or NULL if not available
 */
static inline u8* SafeGetRDRAM(u32* out_size = nullptr) {
    u8* rdram = s_rdram.load(std::memory_order_acquire);
    u32 size = s_rdram_size.load(std::memory_order_acquire);
    
    if (out_size) *out_size = size;
    
    if (!rdram || size == 0) {
        return nullptr;
    }
    return rdram;
}

/**
 * @brief Validate an N64 address is within RDRAM bounds
 */
static inline bool IsAddressValid(u32 addr, u32 access_size) {


    u32 size = s_rdram_size.load(std::memory_order_acquire);
    if (size == 0) return false;
    // Block writes into VI / RDP / framebuffer metadata region
    if (addr >= 0x80000000 && addr < 0x80200000)
        return false;

    return true;

    u32 offset = N64_ADDR_TO_OFFSET(addr);
    return (offset + access_size) <= size;
}
/* Convert KSEG0 virtual address (0x80000000+) to physical RDRAM offset */
static inline u32 CV64_VirtToPhys(u32 addr) {
    return addr & 0x1FFFFFFF;
}

/*===========================================================================
 * Memory Initialization
 *===========================================================================*/

void CV64_Memory_SetRDRAM(u8* rdram, u32 size) {
    /* Validate input before storing */
    if (rdram != nullptr && size > 0) {
        /* Verify the pointer is actually readable by testing access */
        __try {
            volatile u8 test = rdram[0];
            volatile u8 test_end = rdram[size > 0 ? size - 1 : 0];
            (void)test;
            (void)test_end;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            LogWarning("RDRAM pointer validation failed - access violation at %p", rdram);
            rdram = nullptr;
            size = 0;
        }
    }
    
    s_rdram.store(rdram, std::memory_order_release);
    s_rdram_size.store(size, std::memory_order_release);
    s_initialized.store(rdram != nullptr && size > 0, std::memory_order_release);
    
    if (s_initialized.load()) {
        LogInfo("=== RDRAM HOOK INITIALIZED ===");
        LogInfo("  Pointer: %p", rdram);
        LogInfo("  Size: 0x%X (%u MB)", size, size / (1024 * 1024));
        LogInfo("  Expansion Pak: %s", size >= 0x800000 ? "YES (8MB)" : "NO (4MB only)");
        if (size < 0x800000) {
            LogWarning("WARNING: Only 4MB RDRAM detected! Some CV64 hooks may fail.");
            LogWarning("60FPS gameplay cheat addresses (0x80387xxx) require 8MB!");
        }
    } else {
        LogInfo("RDRAM unmapped (pointer=%p, size=%u)", rdram, size);
    }
}

u8* CV64_Memory_GetRDRAM(void) {
    return s_rdram.load(std::memory_order_acquire);
}

bool CV64_Memory_IsInitialized(void) {
    return s_initialized.load(std::memory_order_acquire);
}

/**
 * @brief Get RDRAM size (thread-safe)
 */
u32 CV64_Memory_GetRDRAMSize(void) {
    return s_rdram_size.load(std::memory_order_acquire);
}

/*===========================================================================
 * System Work Access - With Safety Checks
 *===========================================================================*/

/**
 * @brief Get cameraMgr address — uses fixed BSS address directly
 * sym: cameraMgr at 0x80342230 (fixed BSS, no pointer chasing needed)
 * Returns 0 on any failure (null RDRAM, invalid address, etc.)
 */
static u32 ReadCameraMgrPtr(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return 0;

    /* Use the fixed BSS address directly — more reliable than pointer-chasing
     * through system_work + CV64_SYS_OFFSET_CAMERA_MGR_PTR */
    if (!IsAddressValid(CV64_ADDR_CAMERA_MGR, 0x90)) {
        static bool logged = false;
        if (!logged) {
            LogWarning("ReadCameraMgrPtr: BSS address 0x%08X out of bounds", CV64_ADDR_CAMERA_MGR);
            logged = true;
        }
        return 0;
    }

    return CV64_ADDR_CAMERA_MGR;
}

/**
 * @brief Read pointer to Player from system_work
 */
static u32 ReadPlayerPtr(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return 0;
    
    u32 addr = s_system_work_addr + CV64_SYS_OFFSET_PLAYER_PTR;
    if (!IsAddressValid(addr, 4)) return 0;
    
    return CV64_ReadU32(rdram, addr);
}

/**
 * @brief Read current menu state
 */
static u16 ReadCurrentMenu(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return 0;
    
    u32 addr = s_system_work_addr + CV64_SYS_OFFSET_CURRENT_MENU;
    if (!IsAddressValid(addr, 2)) return 0;
    
    return CV64_ReadU16(rdram, addr);
}

/*===========================================================================
 * Controller Reading - With Comprehensive Safety
 *===========================================================================*/

/**
 * @brief Read N64 controller state from RDRAM
 * @return true on success, false on any failure
 */
bool CV64_Memory_ReadController(u32 controller_id, CV64_N64_Controller* out) {
    if (!out) return false;
    
    /* Initialize output to safe defaults */
    memset(out, 0, sizeof(CV64_N64_Controller));
    
    if (controller_id >= CV64_MAX_CONTROLLERS) {
        LogWarning("ReadController: Invalid controller_id %u (max=%u)", 
                   controller_id, CV64_MAX_CONTROLLERS - 1);
        return false;
    }
    
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;
    
    u32 ctrl_addr = s_system_work_addr + CV64_SYS_OFFSET_CONTROLLERS + 
                    (controller_id * CV64_CONTROLLER_SIZE);
    
    /* Validate all addresses before reading */
    if (!IsAddressValid(ctrl_addr, CV64_CONTROLLER_SIZE)) {
        static bool logged = false;
        if (!logged) {
            LogWarning("ReadController: Controller %u address 0x%08X out of bounds", 
                       controller_id, ctrl_addr);
            logged = true;
        }
        return false;
    }
    
    /* Read controller data - each read is individually safe due to CV64_ReadU16 checks */
    out->is_connected = CV64_ReadU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_IS_CONNECTED);
    out->btns_held = CV64_ReadU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_BTNS_HELD);
    out->btns_pressed = CV64_ReadU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_BTNS_PRESSED);
    out->joy_x = (s16)CV64_ReadU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_JOY_X);
    out->joy_y = (s16)CV64_ReadU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_JOY_Y);
    out->joy_ang = (s16)CV64_ReadU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_JOY_ANG);
    out->joy_held = (s16)CV64_ReadU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_JOY_HELD);
    
    return true;
}

/**
 * @brief Write controller state to N64 RDRAM (for input override)
 * @return true on success, false on any failure
 */
bool CV64_Memory_WriteController(u32 controller_id, const CV64_N64_Controller* in) {
    if (!in) return false;
    
    if (controller_id >= CV64_MAX_CONTROLLERS) {
        LogWarning("WriteController: Invalid controller_id %u", controller_id);
        return false;
    }
    
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;
    
    u32 ctrl_addr = s_system_work_addr + CV64_SYS_OFFSET_CONTROLLERS + 
                    (controller_id * CV64_CONTROLLER_SIZE);
    
    /* Validate address range */
    if (!IsAddressValid(ctrl_addr, CV64_CONTROLLER_SIZE)) {
        LogWarning("WriteController: Address 0x%08X out of bounds", ctrl_addr);
        return false;
    }
    
    /* Write controller data */
    CV64_WriteU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_IS_CONNECTED, in->is_connected);
    CV64_WriteU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_BTNS_HELD, in->btns_held);
    CV64_WriteU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_BTNS_PRESSED, in->btns_pressed);
    CV64_WriteU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_JOY_X, (u16)in->joy_x);
    CV64_WriteU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_JOY_Y, (u16)in->joy_y);
    CV64_WriteU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_JOY_ANG, (u16)in->joy_ang);
    CV64_WriteU16(rdram, ctrl_addr + CV64_CTRL_OFFSET_JOY_HELD, (u16)in->joy_held);
    
    return true;
}

/*===========================================================================
 * Camera Manager Reading/Writing - With Comprehensive Safety
 *===========================================================================*/

/**
 * @brief Validate and read camera manager data pointer
 * @param rdram RDRAM pointer (must be valid)
 * @param cameraMgr Camera manager N64 address
 * @return Data pointer address, or 0 if invalid
 */
static u32 GetCameraDataPtr(u8* rdram, u32 cameraMgr) {
    if (!rdram || cameraMgr == 0) return 0;

    /* cameraMgr is a Module object (whether from BSS or pointer-chasing).
     * The actual cameraMgr_data is pointed to by offset +0x34 within the Module.
     * We MUST dereference this pointer — the Module header is NOT the data. */
    u32 dataPtrAddr = cameraMgr + 0x34;
    if (!IsAddressValid(dataPtrAddr, 4)) {
        static bool logged = false;
        if (!logged) {
            LogWarning("GetCameraDataPtr: dataPtrAddr 0x%08X out of bounds", dataPtrAddr);
            logged = true;
        }
        return 0;
    }

    u32 dataPtr = CV64_ReadU32(rdram, dataPtrAddr);

    /* Validate the data pointer itself points to valid memory */
    if (dataPtr != 0 && !IsAddressValid(dataPtr, 0x90)) {
        static bool logged = false;
        if (!logged) {
            LogWarning("GetCameraDataPtr: dataPtr 0x%08X points outside RDRAM", dataPtr);
            logged = true;
        }
        return 0;
    }

    return dataPtr;
}

/**
 * @brief Read camera manager data from RDRAM
 */
bool CV64_Memory_ReadCameraData(CV64_N64_CameraMgrData* out) {
    if (!out) return false;
    
    /* Initialize output to safe defaults */
    memset(out, 0, sizeof(CV64_N64_CameraMgrData));
    
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;
    
    /* Get pointer to cameraMgr_data */
    u32 cameraMgr = ReadCameraMgrPtr();
    if (cameraMgr == 0) return false;
    
    u32 dataPtr = GetCameraDataPtr(rdram, cameraMgr);
    if (dataPtr == 0) return false;
    
    /* Read the data - all accesses are individually bounds-checked */
    out->player_camera_state = CV64_ReadU32(rdram, dataPtr + CV64_CAM_OFFSET_STATE);
    out->camera_mode = CV64_ReadU32(rdram, dataPtr + CV64_CAM_OFFSET_MODE);
    out->previous_camera_mode = CV64_ReadU32(rdram, dataPtr + CV64_CAM_OFFSET_PREV_MODE);
    out->camera_player_ptr = CV64_ReadU32(rdram, dataPtr + CV64_CAM_OFFSET_CAMERA_PTR);
    out->camera_ctrl_ptr = CV64_ReadU32(rdram, dataPtr + CV64_CAM_OFFSET_CTRL_PTR);
    out->camera_mode_text_ptr = CV64_ReadU32(rdram, dataPtr + CV64_CAM_OFFSET_MODE_TEXT);
    out->position_x = CV64_ReadF32(rdram, dataPtr + CV64_CAM_OFFSET_POSITION);
    out->position_y = CV64_ReadF32(rdram, dataPtr + CV64_CAM_OFFSET_POSITION + 4);
    out->position_z = CV64_ReadF32(rdram, dataPtr + CV64_CAM_OFFSET_POSITION + 8);
    out->look_at_x = CV64_ReadF32(rdram, dataPtr + CV64_CAM_OFFSET_LOOK_AT);
    out->look_at_y = CV64_ReadF32(rdram, dataPtr + CV64_CAM_OFFSET_LOOK_AT + 4);
    out->look_at_z = CV64_ReadF32(rdram, dataPtr + CV64_CAM_OFFSET_LOOK_AT + 8);
    
    return true;
}

/**
 * @brief Write camera position to RDRAM (for our camera patch)
 */
bool CV64_Memory_WriteCameraPosition(f32 x, f32 y, f32 z) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;
    
    u32 cameraMgr = ReadCameraMgrPtr();
    if (cameraMgr == 0) return false;
    
    u32 dataPtr = GetCameraDataPtr(rdram, cameraMgr);
    if (dataPtr == 0) return false;
    
    /* Write position - WriteF32 returns success/failure */
    bool success = true;
    success &= CV64_WriteF32(rdram, dataPtr + CV64_CAM_OFFSET_POSITION, x);
    success &= CV64_WriteF32(rdram, dataPtr + CV64_CAM_OFFSET_POSITION + 4, y);
    success &= CV64_WriteF32(rdram, dataPtr + CV64_CAM_OFFSET_POSITION + 8, z);
    
    return success;
}

/**
 * @brief Write camera look-at direction to RDRAM
 */
bool CV64_Memory_WriteCameraLookAt(f32 x, f32 y, f32 z) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;
    
    u32 cameraMgr = ReadCameraMgrPtr();
    if (cameraMgr == 0) return false;
    
    u32 dataPtr = GetCameraDataPtr(rdram, cameraMgr);
    if (dataPtr == 0) return false;
    
    bool success = true;
    success &= CV64_WriteF32(rdram, dataPtr + CV64_CAM_OFFSET_LOOK_AT, x);
    success &= CV64_WriteF32(rdram, dataPtr + CV64_CAM_OFFSET_LOOK_AT + 4, y);
    success &= CV64_WriteF32(rdram, dataPtr + CV64_CAM_OFFSET_LOOK_AT + 8, z);
    
    return success;
}

/*===========================================================================
 * Camera Hook — MIPS Stub Injection
 *===========================================================================*/

static bool s_camera_hook_installed = false;

/**
 * @brief Install 9-instruction MIPS stub at 0x80600000 and patch the JAL
 *        to guLookAtF inside figure_updateMatrices.
 *
 * Stub pseudocode:
 *   LUI  t0, 0x8060
 *   LW   t1, 0x050C(t0)      ; load enabled flag
 *   BEQ  t1, zero, skip      ; if 0 → keep original a1-a3
 *   NOP
 *   LW   a1, 0x0500(t0)      ; override eyeX (float bits)
 *   LW   a2, 0x0504(t0)      ; override eyeY
 *   LW   a3, 0x0508(t0)      ; override eyeZ
 *   skip:
 *   J    0x8008A6E0           ; jump to real guLookAtF
 *   NOP
 */
bool CV64_Memory_InstallCameraHook(void) {
    if (s_camera_hook_installed) return true;

    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;

    /* Verify the original JAL instruction is what we expect */
    if (!IsAddressValid(CV64_ADDR_GULOOKATF_CALL_SITE, 4)) return false;
    u32 origInstr = CV64_ReadU32(rdram, CV64_ADDR_GULOOKATF_CALL_SITE);

    // Log unexpected instruction but only warn once, then skip hook installation
    if (origInstr != CV64_JAL_ORIGINAL_GULOOKATF && origInstr != CV64_JAL_PATCHED_TO_STUB) {
        static bool logged_once = false;
        if (!logged_once) {
            LogInfo("CameraHook: unexpected instruction 0x%08X at call site 0x%08X (expected 0x%08X or 0x%08X) - skipping hook installation", 
                origInstr, CV64_ADDR_GULOOKATF_CALL_SITE, CV64_JAL_ORIGINAL_GULOOKATF, CV64_JAL_PATCHED_TO_STUB);
            logged_once = true;
        }
        return false;
    }

    /* Write the 9-instruction MIPS stub to 0x80600000 */
    const u32 stub[] = {
        0x3C088060, /* LUI  t0, 0x8060                       */
        0x8D09050C, /* LW   t1, 0x050C(t0)   ; enabled flag  */
        0x11200004, /* BEQ  t1, $zero, +4     ; skip override */
        0x00000000, /* NOP  (branch delay)                    */
        0x8D050500, /* LW   a1, 0x0500(t0)   ; eyeX          */
        0x8D060504, /* LW   a2, 0x0504(t0)   ; eyeY          */
        0x8D070508, /* LW   a3, 0x0508(t0)   ; eyeZ          */
        0x080229B8, /* J    0x8008A6E0       ; real guLookAtF */
        0x00000000, /* NOP  (jump delay)                      */
    };

    if (!IsAddressValid(CV64_ADDR_CAMERA_HOOK_STUB, sizeof(stub))) return false;

    for (int i = 0; i < _countof(stub); ++i) {
        if (!CV64_WriteU32(rdram, CV64_ADDR_CAMERA_HOOK_STUB + i * 4, stub[i])) {
            LogInfo("CameraHook: failed to write stub word %d", i);
            return false;
        }
    }

    /* Clear shared data area (eye position + enabled flag) */
    for (u32 off = 0; off <= CV64_HOOK_DATA_OFF_ENABLED; off += 4) {
        CV64_WriteU32(rdram, CV64_ADDR_CAMERA_HOOK_DATA + off, 0);
    }

    /* Patch the JAL: figure_updateMatrices now calls our stub */
    if (!CV64_WriteU32(rdram, CV64_ADDR_GULOOKATF_CALL_SITE, CV64_JAL_PATCHED_TO_STUB)) {
        LogInfo("CameraHook: failed to patch JAL");
        return false;
    }

    /* CRITICAL: Invalidate the dynarec cache so the recompiler picks up our
     * patched JAL and the new MIPS stub code. Without this the dynarec keeps
     * executing the cached original JAL 0x8008A6E0 and never calls our stub. */
    invalidate_r4300_cached_code((struct r4300_core*)&g_dev, CV64_ADDR_GULOOKATF_CALL_SITE, 4);
    invalidate_r4300_cached_code((struct r4300_core*)&g_dev, CV64_ADDR_CAMERA_HOOK_STUB, sizeof(stub));

    s_camera_hook_installed = true;
    LogInfo("CameraHook: installed — stub at 0x%08X, JAL patched at 0x%08X (dynarec invalidated)",
            CV64_ADDR_CAMERA_HOOK_STUB, CV64_ADDR_GULOOKATF_CALL_SITE);
    return true;
}

/**
 * @brief Write override eye position into shared RDRAM for the MIPS stub.
 */
void CV64_Memory_UpdateCameraOverride(f32 x, f32 y, f32 z, bool enabled) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return;

    CV64_WriteF32(rdram, CV64_ADDR_CAMERA_HOOK_DATA + CV64_HOOK_DATA_OFF_EYE_X, x);
    CV64_WriteF32(rdram, CV64_ADDR_CAMERA_HOOK_DATA + CV64_HOOK_DATA_OFF_EYE_Y, y);
    CV64_WriteF32(rdram, CV64_ADDR_CAMERA_HOOK_DATA + CV64_HOOK_DATA_OFF_EYE_Z, z);
    CV64_WriteU32(rdram, CV64_ADDR_CAMERA_HOOK_DATA + CV64_HOOK_DATA_OFF_ENABLED,
                  enabled ? 1u : 0u);
}

/**
 * @brief Read player's yaw angle for auto-center feature
 */
s32 CV64_Memory_ReadPlayerYaw(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return 0;
    
    u32 cameraMgr = ReadCameraMgrPtr();
    if (cameraMgr == 0) return 0;
    
    u32 dataPtr = GetCameraDataPtr(rdram, cameraMgr);
    if (dataPtr == 0) return 0;
    
    u32 yawAddr = dataPtr + CV64_CAM_OFFSET_PLAYER_YAW;
    if (!IsAddressValid(yawAddr, 4)) return 0;
    
    return (s32)CV64_ReadU32(rdram, yawAddr);
}

/**
 * @brief Write player camera yaw angle to game camera data
 */
bool CV64_Memory_WritePlayerYaw(s32 yaw) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;

    u32 cameraMgr = ReadCameraMgrPtr();
    if (cameraMgr == 0) return false;

    u32 dataPtr = GetCameraDataPtr(rdram, cameraMgr);
    if (dataPtr == 0) return false;

    u32 yawAddr = dataPtr + CV64_CAM_OFFSET_PLAYER_YAW;
    if (!IsAddressValid(yawAddr, 4)) return false;

    return CV64_WriteU32(rdram, yawAddr, (u32)yaw);
}

/**
 * @brief Accumulate a yaw offset for mode-0 free camera rotation.
 *        The offset is applied every frame in FrameUpdate.
 */
void CV64_Memory_AddCameraYawOffset(s32 delta) {
    s_camera_yaw_offset.fetch_add(delta, std::memory_order_relaxed);
}

/**
 * @brief Reset the accumulated camera yaw offset (e.g. on map change).
 */
void CV64_Memory_ResetCameraYawOffset(void) {
    s_camera_yaw_offset.store(0, std::memory_order_relaxed);
}

/**
 * @brief Accumulate a camera zoom offset (mouse wheel).
 * Stored as a multiplier: 1.0 = default, <1.0 = closer, >1.0 = farther.
 */
void CV64_Memory_AddCameraZoomOffset(f32 delta) {
    s32 current = s_camera_zoom_mult_x1000.load(std::memory_order_relaxed);
    s32 newVal = current + (s32)(delta * 1000.0f);
    if (newVal < CV64_ZOOM_MIN) newVal = CV64_ZOOM_MIN;
    if (newVal > CV64_ZOOM_MAX) newVal = CV64_ZOOM_MAX;
    s_camera_zoom_mult_x1000.store(newVal, std::memory_order_relaxed);
}

void CV64_Memory_ResetCameraZoomOffset(void) {
    s_camera_zoom_mult_x1000.store(1000, std::memory_order_relaxed);
}

f32 CV64_Memory_GetCameraZoomMultiplier(void) {
    return (f32)s_camera_zoom_mult_x1000.load(std::memory_order_relaxed) / 1000.0f;
}

/**
 * @brief Get current camera mode
 */
u32 CV64_Memory_GetCameraMode(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return 0;
    
    u32 cameraMgr = ReadCameraMgrPtr();
    if (cameraMgr == 0) return 0;
    
    u32 dataPtr = GetCameraDataPtr(rdram, cameraMgr);
    if (dataPtr == 0) return 0;
    
    return CV64_ReadU32(rdram, dataPtr + CV64_CAM_OFFSET_MODE);
}
/*===========================================================================
 * Boss Actor Reading
 *===========================================================================*/

#define CV64_ADDR_BOSS_ACTOR_GFX_PTR   0x800D5F18  /* ptr to boss actor gfx */
#define CV64_BOSS_ACTOR_ID_OFFSET      0x00        /* first field in actor struct */
#define CV64_BOSS_ACTOR_NAME_MAX       32

typedef struct {
    u32 actorPtr;
    u32 actorID;
    const char* name;
} CV64_BossInfo;

bool CV64_Memory_ReadBossInfo(CV64_BossInfo* out);


static const char* CV64_GetBossNameFromID(u32 id) {
    switch (id) {
    case 0x01: return "Cerberus";
    case 0x02: return "Executioner";
    case 0x03: return "Adrian Fahrenheit Tepes";
    case 0x04: return "Renon";
    case 0x05: return "Death";
    case 0x06: return "Dracula";
    default:   return " ";

        CV64_BossInfo boss;
        if (CV64_Memory_ReadBossInfo(&boss)) {
            return boss.name;
        }
        else {
            return "No Boss";
        }



    }
}

bool CV64_Memory_ReadBossInfo(CV64_BossInfo* out) {
    if (!out) return false;
    memset(out, 0, sizeof(CV64_BossInfo));

    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;

    /* Convert virtual → physical */
    u32 gfxPtrPhys = CV64_VirtToPhys(CV64_ADDR_BOSS_ACTOR_GFX_PTR);

    if (!IsAddressValid(gfxPtrPhys, 4))
        return false;

    /* Read pointer to boss actor */
    u32 actorPtr = CV64_ReadU32(rdram, gfxPtrPhys);
    if (actorPtr == 0)
        return false;

    u32 actorPhys = CV64_VirtToPhys(actorPtr);

    if (!IsAddressValid(actorPhys, 4))
        return false;

    /* Read actor ID */
    u32 actorID = CV64_ReadU32(rdram, actorPhys + CV64_BOSS_ACTOR_ID_OFFSET);

    out->actorPtr = actorPtr;
    out->actorID = actorID;
    out->name = CV64_GetBossNameFromID(actorID);

    return true;
}
/*===========================================================================
 * Game State Detection - Thread-Safe
 *===========================================================================*/

/**
 * @brief Check if we're in normal gameplay (not menu/cutscene)
 */
bool CV64_Memory_IsInGameplay(void) {
    return s_is_in_gameplay.load(std::memory_order_acquire);
}

/**
 * @brief Check if D-PAD should control camera (vs menu navigation)
 * Uses verified camera state flags from CASTLEVANIA.sym
 */
bool CV64_Memory_ShouldDPadControlCamera(void) {
    if (!s_initialized.load(std::memory_order_acquire)) return false;

    /* Use detailed game state for more accurate detection */
    CV64_GameState state = s_game_state.load(std::memory_order_acquire);

    /* Only allow camera control during active gameplay */
    if (state != CV64_STATE_GAMEPLAY) return false;

    /* Check first-person and R-lock flags from verified sym addresses */
    u8* rdram = SafeGetRDRAM();
    if (rdram) {
        /* Don't override camera during first-person view (C-Up) */
        if (IsAddressValid(CV64_ADDR_IN_FIRST_PERSON, 4)) {
            if (CV64_ReadU32(rdram, CV64_ADDR_IN_FIRST_PERSON) != 0)
                return false;
        }
        /* Don't override camera during R-button lock */
        if (IsAddressValid(CV64_ADDR_R_LOCK_VIEW, 4)) {
            if (CV64_ReadU32(rdram, CV64_ADDR_R_LOCK_VIEW) != 0)
                return false;
        }
    }

    return true;
}

/*===========================================================================
 * Gameshark Codes / Cheat System
 *===========================================================================*/

/**
 * @brief Camera Patch Cheat
 * 
 * This is our custom CV64 Recomp feature - enables full D-PAD/right stick
 * camera control by hooking into the game's camera system.
 * 
 * Unlike traditional Gameshark cheats, this doesn't write to fixed addresses.
 * Instead, it enables our camera patch system which reads the game's camera
 * manager pointer and modifies the camera angles dynamically.
 * 
 * When enabled:
 *   - D-PAD controls camera rotation
 *   - Right analog stick controls camera (on modern controllers)
 *   - Mouse can control camera (if enabled in settings)
 */

static bool s_cheat_camera_patch_enabled = true;  /* Enabled by default */
static bool s_cheat_camera_logged = false;

/**
 * @brief Enable/disable camera patch cheat
 * 
 * This enables the CV64 Recomp camera control system:
 *   - D-PAD camera rotation
 *   - Right analog stick camera control
 *   - Mouse camera control (if enabled)
 * 
 * @param enabled true to enable, false to disable
 */
void CV64_Memory_SetCameraPatchCheat(bool enabled) {
    s_cheat_camera_patch_enabled = enabled;
    s_cheat_camera_logged = false;
    
    /* Also update the camera patch system */
    CV64_CameraPatch_SetEnabled(enabled);
    
    if (enabled) {
        LogInfo("Camera Patch cheat ENABLED (D-PAD/RStick camera control)");
    } else {
        LogInfo("Camera Patch cheat DISABLED");
    }
}

/**
 * @brief Check if camera patch cheat is enabled
 * @return true if enabled
 */
bool CV64_Memory_IsCameraPatchCheatEnabled(void) {
    return s_cheat_camera_patch_enabled;
}

/**
 * @brief Safely apply a single Gameshark-style memory write
 * @return true if write succeeded, false if address out of bounds
 */
static bool SafeApplyCheatWrite16(u32 addr, u16 value) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) {
        static bool loggedNoRdram = false;
        if (!loggedNoRdram) {
            LogWarning("SafeApplyCheatWrite16: RDRAM not available");
            loggedNoRdram = true;
        }
        return false;
    }
    
    if (!IsAddressValid(addr, 2)) {
        static u32 lastFailedAddr = 0;
        if (addr != lastFailedAddr) {
            u32 offset = N64_ADDR_TO_OFFSET(addr);
            u32 size = s_rdram_size.load(std::memory_order_acquire);
            LogWarning("SafeApplyCheatWrite16: Address 0x%08X (offset 0x%06X) out of bounds (RDRAM size: 0x%X)",
                       addr, offset, size);
            lastFailedAddr = addr;
        }
        return false;
    }
    
    /* Write directly to RDRAM using the dynamic size for validation */
    u32 offset = N64_ADDR_TO_OFFSET(addr);
    u8* ptr = rdram + offset;
    
    /* Write big-endian 16-bit value */
    ptr[0] = (value >> 8) & 0xFF;
    ptr[1] = value & 0xFF;
    
    return true;
}

/**
 * @brief Apply all active Gameshark cheats
 * Called every frame to continuously apply memory modifications.
 * Uses safe memory access that validates bounds before writing.
 */
static void ApplyGamesharkCheats(void) {
    if (!s_initialized.load(std::memory_order_acquire)) return;
    
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return;
    
    /* Log RDRAM info once */
    static bool s_rdram_info_logged = false;
    if (!s_rdram_info_logged) {
        u32 size = s_rdram_size.load(std::memory_order_acquire);
        LogInfo("ApplyGamesharkCheats: RDRAM available at %p, size=0x%X (%u MB)",
                rdram, size, size / (1024 * 1024));
        s_rdram_info_logged = true;
    }
    
    /* Camera Patch - Our main feature! 
     * This enables D-PAD/right stick camera control by updating camera angles
     * based on user input. The actual camera update happens in CV64_CameraPatch_Update
     * which is called from CV64_Memory_FrameUpdate */
    if (s_cheat_camera_patch_enabled) {
        if (!s_cheat_camera_logged) {
            LogInfo("Camera Patch active: D-PAD and Right Stick will control camera rotation");
            s_cheat_camera_logged = true;
        }
        /* Camera patch is applied via CV64_CameraPatch_Update in FrameUpdate */
    }
}

/*===========================================================================
 * Additional Cheats - Lag Reduction & Performance Fixes
 *===========================================================================*/

/**
 * @brief Lag Reduction Cheat
 * 
 * Uses verified process meter addresses from CASTLEVANIA.sym:
 *   processBar_sizeDivisor (0x80096AC4, float) — controls frame budget allocation
 *   processMeter_number_of_divisions (0x80096AC0, u32) — number of time slices
 * Writing a larger divisor gives the game more time budget per frame,
 * reducing visible lag when many objects/enemies are on screen.
 */
#define CV64_CHEAT_LAG_DIVISOR_ADDR     0x80096AC4  /* float: processBar_sizeDivisor */
#define CV64_CHEAT_LAG_DIVISIONS_ADDR   0x80096AC0  /* u32: processMeter_number_of_divisions */

/**
 * @brief Forest of Silence FPS Fix
 * 
 * Uses verified addresses from CASTLEVANIA.sym to reduce processing load:
 *   processBar_sizeDivisor (0x80096AC4, float) — increase frame budget
 *   fog_distance_end (0x80387AE2, u16) — push fog end farther to reduce re-draws
 *   dont_update_map_lighting (0x80185F7C, u32) — skip per-frame lighting recalc
 */
#define CV64_CHEAT_FOREST_DIVISOR_ADDR  0x80096AC4  /* float: processBar_sizeDivisor */
#define CV64_CHEAT_FOREST_FOG_END_ADDR  0x80387AE2  /* u16: fog_distance_end */
#define CV64_CHEAT_FOREST_LIGHTING_ADDR 0x80185F7C  /* u32: dont_update_map_lighting */

/**
 * @brief Infinite Health Cheat
 * Verified CV64 v1.0 USA cheat code from mupencheat.txt:
 *   81389C3E 0064  - Set HP to 100
 * sym: Player_health at 0x80389C3E (system_work + 0x26186)
 */
#define CV64_CHEAT_HEALTH_ADDR          0x80389C3E
#define CV64_CHEAT_HEALTH_VALUE         0x0064

/**
 * @brief Infinite Sub-weapon Ammo
 * Verified CV64 v1.0 USA cheat code from mupencheat.txt:
 *   81389C48 0064  - Set sub-weapon count to 100
 * system_work + 0x26190
 */
#define CV64_CHEAT_SUBWEAPON_ADDR       0x80389C48
#define CV64_CHEAT_SUBWEAPON_VALUE      0x0063

/**
 * @brief Moon Jump - Increase Y velocity when L is held
 * Verified CV64 v1.0 USA cheat code from mupencheat.txt:
 *   D0387D7F 0020  - If L pressed (button check in system_work)
 *   81350810 3FCB  - Set Y velocity high (in player object)
 */
#define CV64_CHEAT_MOONJUMP_CHECK_ADDR  0x80387D7F
#define CV64_CHEAT_MOONJUMP_WRITE_ADDR  0x80350810
#define CV64_CHEAT_MOONJUMP_L_BUTTON    0x0020
#define CV64_CHEAT_MOONJUMP_VALUE       0x3FCB

/* Cheat enable flags */
static bool s_cheat_lag_reduction_enabled = false;  /* Uses processBar_sizeDivisor */
static bool s_cheat_forest_fix_enabled = false;     /* Uses fog + lighting tweaks */
static bool s_cheat_infinite_health_enabled = false;
static bool s_cheat_infinite_subweapon_enabled = false;
static bool s_cheat_moon_jump_enabled = false;
static bool s_cheat_extended_draw_enabled = false;   /* Extended draw distance — DISABLED */

/* Extended draw distance state */
static u16 s_fog_original_start = 0;
static u16 s_fog_original_end = 0;
static s16 s_fog_snapshot_map = -1;  /* map ID when snapshot was taken */
static f32 s_draw_distance_multiplier = 2.5f;  /* how much farther to push fog */

/* Logging flags */
static bool s_cheat_lag_logged = false;
static bool s_cheat_forest_logged = false;
static bool s_cheat_health_logged = false;
static bool s_cheat_subweapon_logged = false;
static bool s_cheat_moonjump_logged = false;
static bool s_cheat_draw_logged = false;

void CV64_Memory_SetLagReductionCheat(bool enabled) {
    s_cheat_lag_reduction_enabled = enabled;
    s_cheat_lag_logged = false;
    LogInfo("Lag Reduction cheat %s", enabled ? "ENABLED" : "DISABLED");
}

bool CV64_Memory_IsLagReductionCheatEnabled(void) {
    return s_cheat_lag_reduction_enabled;
}

void CV64_Memory_SetForestFpsFixCheat(bool enabled) {
    s_cheat_forest_fix_enabled = enabled;
    s_cheat_forest_logged = false;
    LogInfo("Forest FPS Fix cheat %s", enabled ? "ENABLED" : "DISABLED");
}

bool CV64_Memory_IsForestFpsFixCheatEnabled(void) {
    return s_cheat_forest_fix_enabled;
}

void CV64_Memory_SetInfiniteHealthCheat(bool enabled) {
    s_cheat_infinite_health_enabled = enabled;
    s_cheat_health_logged = false;
    LogInfo("Infinite Health cheat %s", enabled ? "ENABLED" : "DISABLED");
}

bool CV64_Memory_IsInfiniteHealthCheatEnabled(void) {
    return s_cheat_infinite_health_enabled;
}

void CV64_Memory_SetInfiniteSubweaponCheat(bool enabled) {
    s_cheat_infinite_subweapon_enabled = enabled;
    s_cheat_subweapon_logged = false;
    LogInfo("Infinite Sub-weapon cheat %s", enabled ? "ENABLED" : "DISABLED");
}

bool CV64_Memory_IsInfiniteSubweaponCheatEnabled(void) {
    return s_cheat_infinite_subweapon_enabled;
}

void CV64_Memory_SetMoonJumpCheat(bool enabled) {
    s_cheat_moon_jump_enabled = enabled;
    s_cheat_moonjump_logged = false;
    LogInfo("Moon Jump cheat %s", enabled ? "ENABLED" : "DISABLED");
}

bool CV64_Memory_IsMoonJumpCheatEnabled(void) {
    return s_cheat_moon_jump_enabled;
}

void CV64_Memory_SetExtendedDrawDistance(bool enabled) {
    s_cheat_extended_draw_enabled = enabled;
    s_cheat_draw_logged = false;
    /* Reset fog snapshot so it re-captures on next frame */
    s_fog_snapshot_map = -1;
    s_fog_original_start = 0;
    s_fog_original_end = 0;
    LogInfo("Extended Draw Distance %s (multiplier=%.1f)", 
            enabled ? "ENABLED" : "DISABLED", s_draw_distance_multiplier);
}

bool CV64_Memory_IsExtendedDrawDistanceEnabled(void) {
    return s_cheat_extended_draw_enabled;
}

void CV64_Memory_SetDrawDistanceMultiplier(f32 multiplier) {
    if (multiplier < 1.0f) multiplier = 1.0f;
    if (multiplier > 10.0f) multiplier = 10.0f;
    s_draw_distance_multiplier = multiplier;
    /* Reset snapshot to re-apply with new multiplier */
    s_fog_snapshot_map = -1;
    s_cheat_draw_logged = false;
    LogInfo("Draw Distance multiplier set to %.1f", multiplier);
}

f32 CV64_Memory_GetDrawDistanceMultiplier(void) {
    return s_draw_distance_multiplier;
}

/**
 * @brief Per-map lag tuning table.
 * Maps that are heavier (more objects/enemies/fog) get a higher baseline divisor.
 * The divisor controls the N64 process meter frame budget — higher = more budget
 * per frame = less stutter, but may cause visual artifacts if pushed too high.
 *
 * Default game value is ~2.0f. We tune per-map based on typical object load.
 */
struct MapLagProfile {
    f32 baselineDivisor;    /* Minimum divisor to always apply */
    f32 maxDivisor;         /* Maximum divisor under heavy load */
    u32 baseDivisions;      /* Number of time slices */
};

static const MapLagProfile s_mapLagProfiles[] = {
    /* 0x00 MORI  - Forest of Silence: dense trees, wolves, fog   */ { 4.0f, 8.0f, 2 },
    /* 0x01 TOU   - Castle Wall Main: open area, moderate load    */ { 3.0f, 6.0f, 2 },
    /* 0x02 TOUOKUJI - Castle Wall Tower: vertical, few enemies   */ { 3.0f, 5.0f, 2 },
    /* 0x03 NAKANIWA - Villa Front Yard: open, hedge maze         */ { 3.5f, 6.0f, 2 },
    /* 0x04 HIDE  - Villa Hallway: indoor, moderate               */ { 3.0f, 5.0f, 2 },
    /* 0x05 TOP   - Villa Maze/Storeroom: complex geometry        */ { 3.5f, 6.0f, 2 },
    /* 0x06 EAR   - Tunnel: enclosed, lower load                  */ { 3.0f, 5.0f, 2 },
    /* 0x07 MIZU  - Underground Waterway: water effects, enemies  */ { 3.5f, 6.0f, 2 },
    /* 0x08 MACHINE_TOWER - Castle Center Machine: complex, heavy */ { 4.5f, 8.0f, 3 },
    /* 0x09 MEIRO - Castle Center Main: many enemies, heaviest    */ { 5.0f, 9.0f, 3 },
    /* 0x0A TOKEITOU_NAI - Clock Tower Interior: gears, platforms */ { 4.0f, 7.0f, 2 },
    /* 0x0B TOKEITOU_MAE - Clock Tower Approach: moderate         */ { 3.5f, 6.0f, 2 },
    /* 0x0C HONMARU_B1F - Castle Keep Basement: enemies           */ { 3.5f, 6.0f, 2 },
    /* 0x0D SHOKUDOU - Room of Clocks: indoor, moderate           */ { 3.0f, 5.0f, 2 },
    /* 0x0E SHINDENTOU_UE - Tower of Science Upper: complex       */ { 4.0f, 7.0f, 2 },
    /* 0x0F SHINDENTOU_NAKA - Tower of Science Mid: complex       */ { 4.0f, 7.0f, 2 },
    /* 0x10 KETTOUTOU - Duel Tower: boss fights                   */ { 4.0f, 7.0f, 2 },
    /* 0x11 MAKAIMURA - Art Tower: unique geometry                 */ { 3.5f, 6.0f, 2 },
    /* 0x12 HONMARU_1F - Castle Keep 1F: enemies, vertical        */ { 3.5f, 6.0f, 2 },
    /* 0x13 HONMARU_2F - Castle Keep 2F: enemies, vertical        */ { 3.5f, 6.0f, 2 },
    /* 0x14 HONMARU_3F_S - Castle Keep 3F South: boss area        */ { 4.0f, 7.0f, 2 },
    /* 0x15 HONMARU_4F_S - Castle Keep 4F South: final boss       */ { 4.5f, 8.0f, 3 },
    /* 0x16 HONMARU_3F_N - Castle Keep 3F North                   */ { 3.5f, 6.0f, 2 },
    /* 0x17 ROSE  - Tower of Sorcery: boss, effects               */ { 4.0f, 7.0f, 2 },
    /* 0x18 Ending Reinhardt                                      */ { 2.0f, 3.0f, 1 },
    /* 0x19 Ending Carrie                                         */ { 2.0f, 3.0f, 1 },
    /* 0x1A Ending                                                */ { 2.0f, 3.0f, 1 },
    /* 0x1B Game Over                                             */ { 2.0f, 3.0f, 1 },
};

#define CV64_MAP_LAG_PROFILE_COUNT (sizeof(s_mapLagProfiles) / sizeof(s_mapLagProfiles[0]))

/* Adaptive frame budget state */
static f32 s_adaptive_divisor = 2.0f;
static bool s_adaptive_logged = false;

/**
 * @brief Apply performance-related cheats (per-map lag tuning + adaptive frame budget)
 */
static void ApplyPerformanceCheats(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return;

    static int applyCount = 0;
    applyCount++;

    /* Per-Map Lag Tuning + Adaptive Frame Budget
     * -------------------------------------------
     * 1. Look up the per-map baseline from s_mapLagProfiles[]
     * 2. Read the process meter (green + blue bar sizes) to detect frame pressure
     * 3. When combined load exceeds threshold, ramp divisor up toward max
     * 4. When under budget, relax back down toward baseline
     * This replaces the old flat divisor=4.0 approach. */
    if (s_cheat_lag_reduction_enabled) {
        s16 mapId = CV64_Memory_GetCurrentMapId();
        f32 baseline = 3.0f;
        f32 maxDiv   = 6.0f;
        u32 divisions = 2;

        /* Lookup per-map profile */
        if (mapId >= 0 && mapId < (s16)CV64_MAP_LAG_PROFILE_COUNT) {
            baseline  = s_mapLagProfiles[mapId].baselineDivisor;
            maxDiv    = s_mapLagProfiles[mapId].maxDivisor;
            divisions = s_mapLagProfiles[mapId].baseDivisions;
        }

        /* Read process meter to detect frame pressure */
        f32 greenBar = 0.0f, blueBar = 0.0f;
        if (IsAddressValid(CV64_ADDR_PROCESS_GREEN_SIZE, 4))
            greenBar = CV64_ReadF32(rdram, CV64_ADDR_PROCESS_GREEN_SIZE);
        if (IsAddressValid(CV64_ADDR_PROCESS_BLUE_SIZE, 4))
            blueBar = CV64_ReadF32(rdram, CV64_ADDR_PROCESS_BLUE_SIZE);

        f32 totalLoad = greenBar + blueBar;

        /* Adaptive ramping:
         * - totalLoad > 0.85 → frame is over budget, ramp UP toward maxDiv
         * - totalLoad < 0.60 → frame has headroom, relax DOWN toward baseline
         * - in between → hold current value */
        const f32 RAMP_UP_RATE   = 0.05f;  /* How fast to increase (per frame) */
        const f32 RAMP_DOWN_RATE = 0.02f;  /* How fast to decrease (slower) */
        const f32 LOAD_HIGH      = 0.85f;
        const f32 LOAD_LOW       = 0.60f;

        if (totalLoad > LOAD_HIGH) {
            s_adaptive_divisor += RAMP_UP_RATE;
            if (s_adaptive_divisor > maxDiv) s_adaptive_divisor = maxDiv;
        } else if (totalLoad < LOAD_LOW) {
            s_adaptive_divisor -= RAMP_DOWN_RATE;
            if (s_adaptive_divisor < baseline) s_adaptive_divisor = baseline;
        }
        /* else: hold current divisor */

        CV64_WriteF32(rdram, CV64_CHEAT_LAG_DIVISOR_ADDR, s_adaptive_divisor);
        CV64_WriteU32(rdram, CV64_CHEAT_LAG_DIVISIONS_ADDR, divisions);

        if (!s_cheat_lag_logged) {
            LogInfo("Per-Map Lag Tuning (Map %d): baseline=%.1f, max=%.1f, divisions=%u, load=%.2f",
                    mapId, baseline, maxDiv, divisions, totalLoad);
            s_cheat_lag_logged = true;
        }

        /* Log adaptive changes periodically */
        if (applyCount % 300 == 0) {
            LogInfo("Adaptive Budget (Map %d): divisor=%.2f, load=%.2f (green=%.2f blue=%.2f)",
                    mapId, s_adaptive_divisor, totalLoad, greenBar, blueBar);
        }
    }

    /* Forest FPS Fix — Only apply in Forest areas (map ID 0x02 or 0x03)
     * Increases process budget further and reduces lighting recalculation */
    if (s_cheat_forest_fix_enabled) {
        s16 mapId = CV64_Memory_GetCurrentMapId();
        if (mapId == 0x00) {  // MORI = Forest of Silence
            bool success = true;
            /* Larger budget for forest's heavy object load */
            success &= CV64_WriteF32(rdram, CV64_CHEAT_FOREST_DIVISOR_ADDR, 6.0f);
            /* Push fog end slightly farther to reduce overdraw recalculation */
            success &= CV64_WriteU16(rdram, CV64_CHEAT_FOREST_FOG_END_ADDR, 1200);
            /* Skip per-frame lighting recalc (trees/shadows) */
            success &= CV64_WriteU32(rdram, CV64_CHEAT_FOREST_LIGHTING_ADDR, 1);

            if (!s_cheat_forest_logged) {
                if (success) {
                    LogInfo("Forest FPS Fix applied (Map %d): divisor=6.0, fogEnd=1200, skipLighting=1", mapId);
                } else {
                    LogWarning("Forest FPS Fix: Some addresses failed (Map %d)", mapId);
                }
                s_cheat_forest_logged = true;
            }
        }
    }

    /* Extended Draw Distance — push fog start/end farther to increase visibility.
     * Reads the game's default fog values on map load, then applies a multiplier.
     * Each map has different fog settings so we scale rather than hardcode. */
    if (s_cheat_extended_draw_enabled) {
        s16 mapId = CV64_Memory_GetCurrentMapId();

        /* Snapshot the game's original fog values when entering a new map */
        if (mapId != s_fog_snapshot_map && mapId >= 0) {
            if (IsAddressValid(CV64_ADDR_FOG_DIST_START, 2) &&
                IsAddressValid(CV64_ADDR_FOG_DIST_END, 2)) {
                u16 curStart = CV64_ReadU16(rdram, CV64_ADDR_FOG_DIST_START);
                u16 curEnd   = CV64_ReadU16(rdram, CV64_ADDR_FOG_DIST_END);
                /* Only snapshot if game has set real values (not 0) */
                if (curEnd > 0) {
                    s_fog_original_start = curStart;
                    s_fog_original_end = curEnd;
                    s_fog_snapshot_map = mapId;
                    s_cheat_draw_logged = false;
                    LogInfo("Draw Distance: Captured map %d fog defaults: start=%u, end=%u",
                            mapId, curStart, curEnd);
                }
            }
        }

        /* Apply extended fog distances every frame */
        if (s_fog_original_end > 0 && mapId >= 0) {
            u16 newStart = (u16)((f32)s_fog_original_start * s_draw_distance_multiplier);
            u16 newEnd   = (u16)((f32)s_fog_original_end * s_draw_distance_multiplier);

            /* Clamp to N64 fog range (max ~32000 for s16 range) */
            if (newStart > 30000) newStart = 30000;
            if (newEnd > 32000) newEnd = 32000;
            if (newEnd <= newStart) newEnd = newStart + 100;

            CV64_WriteU16(rdram, CV64_ADDR_FOG_DIST_START, newStart);
            CV64_WriteU16(rdram, CV64_ADDR_FOG_DIST_END, newEnd);

            if (!s_cheat_draw_logged) {
                LogInfo("Draw Distance applied (Map %d): fog start %u->%u, end %u->%u (x%.1f)",
                        mapId, s_fog_original_start, newStart,
                        s_fog_original_end, newEnd, s_draw_distance_multiplier);
                s_cheat_draw_logged = true;
            }
        }
    }

    /* Log periodically to show cheats are running */
    if (applyCount == 60 || applyCount == 300 || (applyCount % 1800 == 0)) {
        LogInfo("Performance cheats active: LagReduction=%s, ForestFix=%s, DrawDist=%s (frame %d)",
                s_cheat_lag_reduction_enabled ? "ON" : "OFF",
                s_cheat_forest_fix_enabled ? "ON" : "OFF",
                s_cheat_extended_draw_enabled ? "ON" : "OFF",
                applyCount);
    }
}

/**
 * @brief Apply gameplay cheats (infinite health, sub-weapon, etc.)
 */
static void ApplyGameplayCheats(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return;
    
    /* Infinite Health */
    if (s_cheat_infinite_health_enabled) {
        if (SafeApplyCheatWrite16(CV64_CHEAT_HEALTH_ADDR, CV64_CHEAT_HEALTH_VALUE)) {
            if (!s_cheat_health_logged) {
                LogInfo("Infinite Health cheat applied: HP locked at %d", CV64_CHEAT_HEALTH_VALUE);
                s_cheat_health_logged = true;
            }
        }
    }
    
    /* Infinite Sub-weapon */
    if (s_cheat_infinite_subweapon_enabled) {
        if (SafeApplyCheatWrite16(CV64_CHEAT_SUBWEAPON_ADDR, CV64_CHEAT_SUBWEAPON_VALUE)) {
            if (!s_cheat_subweapon_logged) {
                LogInfo("Infinite Sub-weapon cheat applied: Ammo locked at %d", CV64_CHEAT_SUBWEAPON_VALUE);
                s_cheat_subweapon_logged = true;
            }
        }
    }
    
    /* Moon Jump - check if L button is held (byte check per D0 cheat code format) */
    if (s_cheat_moon_jump_enabled) {
        if (IsAddressValid(CV64_CHEAT_MOONJUMP_CHECK_ADDR, 1)) {
            u8 buttons = CV64_ReadU8(rdram, CV64_CHEAT_MOONJUMP_CHECK_ADDR);
            if (buttons & CV64_CHEAT_MOONJUMP_L_BUTTON) {
                SafeApplyCheatWrite16(CV64_CHEAT_MOONJUMP_WRITE_ADDR, CV64_CHEAT_MOONJUMP_VALUE);
                if (!s_cheat_moonjump_logged) {
                    LogInfo("Moon Jump cheat active: Hold L to jump higher");
                    s_cheat_moonjump_logged = true;
                }
            }
        }
    }
}

/**
 * @brief Public API to apply all cheats
 * This is automatically called each frame from CV64_Memory_FrameUpdate
 */
void CV64_Memory_ApplyAllCheats(void) {
    if (!s_initialized.load(std::memory_order_acquire)) return;
    
    ApplyPerformanceCheats();
    ApplyGameplayCheats();
}

/*===========================================================================
 * Frame Update Hook
 *===========================================================================*/

/**
 * @brief Called each frame to update our patches with game state
 * PERFORMANCE: Only update cache every few frames to reduce memory read overhead
 * SAFETY: All memory access is validated before use
 * NOTE: No memory patching or adjustments are performed while on the Title Screen
 *       or Character Select screen, as doing so causes save/load issues.
 */
void CV64_Memory_FrameUpdate(void) {
    static int frameCount = 0;
    static bool loggedFirstSuccess = false;
    static bool loggedHookRunning = false;
    static bool loggedTitleScreenSkip = false;
    static const int CACHE_UPDATE_INTERVAL = 4;  /* Update cache every 4 frames */

    if (!s_initialized.load(std::memory_order_acquire)) {
        /* Log once if we're being called but not initialized */
        static bool loggedNotInit = false;
        if (!loggedNotInit) {
            LogWarning("CV64_Memory_FrameUpdate called but RDRAM not initialized!");
            loggedNotInit = true;
        }
        return;
    }

    /* Log that hook is running (once) */
    if (!loggedHookRunning) {
        LogInfo("=== CV64 MEMORY HOOK IS RUNNING ===");
        loggedHookRunning = true;
    }

    frameCount++;

    u8* rdram = SafeGetRDRAM();
    if (!rdram) {
        /* RDRAM not available - reset state and skip processing */
        s_camera_mgr_addr.store(0, std::memory_order_release);
        s_player_addr.store(0, std::memory_order_release);
        s_is_in_gameplay.store(false, std::memory_order_release);
        return;
    }

    /* Read current map ID for state tracking */
    s16 currentMap = ReadCurrentMapId();

    /* Check if we're on the Title Screen or Character Select.
     * Map IDs 0+ are all real stages (0=Forest, 1=Castle Wall, etc.)
     * The title screen is NOT a map — it has inPlayerGameplayLoop == 0 AND
     * no valid map loaded. If a valid map (0x00-0x1F) is loaded, we are in
     * gameplay even if inPlayerGameplayLoop hasn't been set yet (e.g. during
     * the first frames of a map load or a cutscene). Only skip patches when
     * the gameplay loop flag is 0 AND the map ID is out of the valid range. */
    bool onTitleScreen = false;
    if (IsAddressValid(CV64_ADDR_IN_GAMEPLAY_LOOP, 4)) {
        u32 inLoop = CV64_ReadU32(rdram, CV64_ADDR_IN_GAMEPLAY_LOOP);
        if (inLoop == 0 && !(currentMap >= 0 && currentMap < 0x20)) {
            onTitleScreen = true;
        }
    }

    if (onTitleScreen) {
        if (!loggedTitleScreenSkip) {
            LogInfo("Title Screen detected (inPlayerGameplayLoop=0, Map=%d) - skipping memory patches", currentMap);
            loggedTitleScreenSkip = true;
        }
        /* Still update the state cache for display purposes (read-only) */
        s_camera_mgr_addr.store(0, std::memory_order_release);
        s_player_addr.store(0, std::memory_order_release);
        s_is_in_gameplay.store(false, std::memory_order_release);
        s_current_menu.store(0, std::memory_order_release);
        return;
    }
    loggedTitleScreenSkip = false;  /* Reset so we log again if we return to title */

    /* Apply Gameshark cheats every frame (before other processing) */
    ApplyGamesharkCheats();

    /* Apply gameplay cheats every frame (health/subweapon must be continuous) */
    ApplyGameplayCheats();

    /* Apply performance cheats EVERY frame — the game re-sets fog values each
     * frame, so our extended draw distance override must be written continuously
     * or the game's defaults immediately overwrite ours (3 out of 4 frames). */
    ApplyPerformanceCheats();

    /* Apply accumulated camera yaw offset every frame when camera mode is 0.
     * The game recalculates player_angle_yaw each frame from the player's
     * facing direction, so we must re-apply our offset continuously. */
    {
        s32 yawOff = s_camera_yaw_offset.load(std::memory_order_relaxed);
        if (yawOff != 0) {
            u32 camMode = s_current_camera_mode.load(std::memory_order_acquire);
            if (camMode == 0 && s_is_in_gameplay.load(std::memory_order_acquire)) {
                s32 baseYaw = CV64_Memory_ReadPlayerYaw();
                CV64_Memory_WritePlayerYaw(baseYaw + yawOff);
            }
        }
    }

    /* Apply camera zoom multiplier every frame.
     * Reads the game's camera_distance_to_player Vec3f and scales it by our
     * zoom multiplier, then writes it back. The game recalculates this each
     * frame, so we must override continuously (same pattern as yaw offset). */
    {
        s32 zoomX1000 = s_camera_zoom_mult_x1000.load(std::memory_order_relaxed);
        if (zoomX1000 != 1000 && s_is_in_gameplay.load(std::memory_order_acquire)) {
            u32 cameraMgrLocal = ReadCameraMgrPtr();
            if (cameraMgrLocal != 0) {
                u32 dataPtr = GetCameraDataPtr(rdram, cameraMgrLocal);
                if (dataPtr != 0) {
                    f32 mult = (f32)zoomX1000 / 1000.0f;
                    u32 distAddr = dataPtr + CV64_CAM_OFFSET_DISTANCE;
                    if (IsAddressValid(distAddr, 12)) {
                        f32 dx = CV64_ReadF32(rdram, distAddr + 0);
                        f32 dy = CV64_ReadF32(rdram, distAddr + 4);
                        f32 dz = CV64_ReadF32(rdram, distAddr + 8);
                        CV64_WriteF32(rdram, distAddr + 0, dx * mult);
                        CV64_WriteF32(rdram, distAddr + 4, dy * mult);
                        CV64_WriteF32(rdram, distAddr + 8, dz * mult);
                    }
                }
            }
        }
    }

    /* Process camera input from controller/keyboard/mouse
     * This is separate from mupen64plus input handling - it only reads
     * input devices to control OUR camera patch, not the N64 game input */
    CV64_Controller_UpdateCameraInput();

    /* Always update game state cache (needed for window title, cheats, etc.)
     * This runs regardless of camera patch state */
    if (frameCount % CACHE_UPDATE_INTERVAL == 0) {
        u32 cameraMgr = ReadCameraMgrPtr();
        u32 player = ReadPlayerPtr();
        u32 menu = ReadCurrentMenu();

        s_camera_mgr_addr.store(cameraMgr, std::memory_order_release);
        s_player_addr.store(player, std::memory_order_release);
        s_current_menu.store(menu, std::memory_order_release);

        /* Read camera mode via proper pointer chain (cameraMgr Module → +0x34 → data) */
        if (cameraMgr != 0) {
            u32 dataPtr = GetCameraDataPtr(rdram, cameraMgr);
            if (dataPtr != 0) {
                u32 mode = CV64_ReadU32(rdram, dataPtr + CV64_CAM_OFFSET_MODE);
                s_current_camera_mode.store(mode, std::memory_order_release);
            }
        }

        /* Read game_time (sym: game_time at 0x800A7900) */
        if (IsAddressValid(CV64_ADDR_GAME_TIME, 4)) {
            u32 gt = CV64_ReadU32(rdram, CV64_ADDR_GAME_TIME);
            s_game_time.store(gt, std::memory_order_release);
            /* Day/Night: day when cycle position in [0x4000, 0xC000) */
            u16 cyclePos = (u16)(gt & 0xFFFF);
            s_is_daytime.store(cyclePos >= 0x4000 && cyclePos < 0xC000, std::memory_order_release);
        }

        /* Check soft reset (sym: isSoftReset at 0x800947F8) */
        if (IsAddressValid(CV64_ADDR_IS_SOFT_RESET, 2)) {
            u16 sr = CV64_ReadU16(rdram, CV64_ADDR_IS_SOFT_RESET);
            s_is_soft_reset.store(sr != 0, std::memory_order_release);
        }

        /* Compute detailed game state using multi-signal detection */
        CV64_GameState newState = ComputeGameState(rdram, currentMap);
        s_game_state.store(newState, std::memory_order_release);

        /* Map change detection & callback */
        s16 prevMap = s_last_map_id.load(std::memory_order_acquire);
        if (currentMap != prevMap && prevMap != -1 && currentMap > 1) {
            if (s_map_change_callback) {
                s_map_change_callback(prevMap, currentMap);
            }
            /* Reset camera yaw offset on map change */
            s_camera_yaw_offset.store(0, std::memory_order_relaxed);
            /* Reset zoom to default on map change */
            s_camera_zoom_mult_x1000.store(1000, std::memory_order_relaxed);
            /* Reset adaptive divisor so it re-profiles for the new map */
            s_adaptive_divisor = 2.0f;
            s_adaptive_logged = false;
            s_cheat_lag_logged = false;
            LogInfo("Map changed: %d (%s) -> %d (%s)",
                    prevMap, CV64_Memory_GetMapName(prevMap),
                    currentMap, CV64_Memory_GetMapName(currentMap));
        }
        s_last_map_id.store(currentMap, std::memory_order_release);

        /* Update gameplay state — use detailed state for more accuracy */
        bool inGameplay = (newState == CV64_STATE_GAMEPLAY);
        s_is_in_gameplay.store(inGameplay, std::memory_order_release);
    }
    
    /* Debug: Log game state periodically - heartbeat to confirm hook is active */
    if (frameCount == 60 || frameCount == 300 || frameCount == 600 || (frameCount % 1800 == 0)) {
        u32 mapAddr = s_system_work_addr + CV64_SYS_OFFSET_MAP_ID;
        s16 mapId = IsAddressValid(mapAddr, 2) ? (s16)CV64_ReadU16(rdram, mapAddr) : -1;
        u32 cameraMgr = s_camera_mgr_addr.load(std::memory_order_acquire);
        u32 player = s_player_addr.load(std::memory_order_acquire);
        u32 mode = s_current_camera_mode.load(std::memory_order_acquire);
        u32 menu = s_current_menu.load(std::memory_order_acquire);
        bool inGameplay = s_is_in_gameplay.load(std::memory_order_acquire);
        CV64_GameState state = s_game_state.load(std::memory_order_acquire);
        u32 gt = s_game_time.load(std::memory_order_acquire);

        LogInfo("Frame %d: Map=%d (%s), State=%d, CameraMgr=0x%08X, Mode=%u, Menu=%u, Time=%u (%s), InGameplay=%s",
                frameCount, mapId, CV64_Memory_GetMapName(mapId), (int)state,
                cameraMgr, mode, menu, gt, CV64_Memory_GetTimeOfDayString(),
                inGameplay ? "YES" : "NO");
    }
    
    /* Early exit if camera patch is disabled - skip camera-specific processing */
    if (!CV64_CameraPatch_IsEnabled()) return;
    
    /* Only read camera data when actually needed and during cache update frames
     * This reduces memory reads significantly when many enemies are on screen */
    static CV64_N64_CameraMgrData cachedCamData;
    static bool camDataValid = false;
    
    if (frameCount % CACHE_UPDATE_INTERVAL == 0) {
        /* Read current game camera state */
        camDataValid = CV64_Memory_ReadCameraData(&cachedCamData);
    }
    
    if (camDataValid) {
        /* Update our camera patch with game's look-at target */
        Vec3f lookAt = { cachedCamData.look_at_x, cachedCamData.look_at_y, cachedCamData.look_at_z };
        CV64_CameraPatch_SetLookAt(&lookAt);

        /* Lock camera when not in active gameplay, or during first-person/R-lock */
        CV64_GameState state = s_game_state.load(std::memory_order_acquire);
        bool locked = (state != CV64_STATE_GAMEPLAY);
        /* Also lock if player is using first-person or R-lock (native camera modes) */
        if (!locked && rdram) {
            if (IsAddressValid(CV64_ADDR_IN_FIRST_PERSON, 4) &&
                CV64_ReadU32(rdram, CV64_ADDR_IN_FIRST_PERSON) != 0)
                locked = true;
            if (IsAddressValid(CV64_ADDR_R_LOCK_VIEW, 4) &&
                CV64_ReadU32(rdram, CV64_ADDR_R_LOCK_VIEW) != 0)
                locked = true;
        }
        CV64_CameraPatch_SetLocked(locked);

        u32 cameraMgr = s_camera_mgr_addr.load(std::memory_order_acquire);
        if (!loggedFirstSuccess && cameraMgr != 0) {
            LogInfo("Camera data read success! Pos=(%.1f,%.1f,%.1f) LookAt=(%.1f,%.1f,%.1f)",
                    cachedCamData.position_x, cachedCamData.position_y, cachedCamData.position_z,
                    cachedCamData.look_at_x, cachedCamData.look_at_y, cachedCamData.look_at_z);
            loggedFirstSuccess = true;
        }
    }
    
    /* If camera patch is active and not locked, update and apply our camera position */
    if (s_is_in_gameplay.load(std::memory_order_acquire)) {
        /* Install the MIPS stub once (hooks guLookAtF inside figure_updateMatrices) */
        CV64_Memory_InstallCameraHook();

        /* Calculate delta time - assume 60fps, ~16.67ms per frame */
        const f32 deltaTime = 1.0f / 60.0f;

        /* Update camera patch state (calculates new position from yaw/pitch) */
        CV64_CameraPatch_Update(deltaTime);

        Vec3f pos;
        CV64_CameraPatch_GetPosition(&pos);

        bool locked = CV64_CameraPatch_IsLocked();

        /* Debug: Log camera override state periodically */
        if (frameCount % 300 == 0) {
            f32 yaw, pitch;
            CV64_CameraPatch_GetRotation(&yaw, &pitch);
            LogInfo("Camera override: Pos=(%.1f,%.1f,%.1f) Yaw=%.1f Pitch=%.1f Locked=%s HookInstalled=%s",
                    pos.x, pos.y, pos.z, yaw, pitch,
                    locked ? "YES" : "NO",
                    s_camera_hook_installed ? "YES" : "NO");

            /* Verify the patched JAL is still in place */
            u8* rdram = SafeGetRDRAM();
            if (rdram && IsAddressValid(CV64_ADDR_GULOOKATF_CALL_SITE, 4)) {
                u32 jalInstr = CV64_ReadU32(rdram, CV64_ADDR_GULOOKATF_CALL_SITE);
                u32 enabledFlag = CV64_ReadU32(rdram, CV64_ADDR_CAMERA_HOOK_DATA + CV64_HOOK_DATA_OFF_ENABLED);
                LogInfo("  JAL@0x%08X = 0x%08X (%s), EnabledFlag = %u",
                        CV64_ADDR_GULOOKATF_CALL_SITE, jalInstr,
                        jalInstr == CV64_JAL_PATCHED_TO_STUB ? "PATCHED" : "ORIGINAL",
                        enabledFlag);
            }
        }

        /* Write override position to shared RDRAM — the MIPS stub reads this */
        CV64_Memory_UpdateCameraOverride(pos.x, pos.y, pos.z, !locked);
    }
    
    /* D-PAD camera control is now handled by CV64_Controller_Update
     * which calls CV64_CameraPatch_ProcessDPad directly */
}

/*===========================================================================
 * Debug Functions
 *===========================================================================*/

void CV64_Memory_DumpState(void) {
    u8* rdram = SafeGetRDRAM();
    u32 rdramSize = s_rdram_size.load(std::memory_order_acquire);
    
    if (!s_initialized.load(std::memory_order_acquire)) {
        LogInfo("Memory not initialized");
        return;
    }
    
    LogInfo("=== Memory State Dump ===");
    LogInfo("RDRAM: %p, Size: 0x%X (%u MB)", rdram, rdramSize, rdramSize / (1024 * 1024));
    LogInfo("System Work: 0x%08X", s_system_work_addr);
    LogInfo("Camera Mgr: 0x%08X", s_camera_mgr_addr.load(std::memory_order_acquire));
    LogInfo("Player: 0x%08X", s_player_addr.load(std::memory_order_acquire));
    LogInfo("Camera Mode: %u", s_current_camera_mode.load(std::memory_order_acquire));
    LogInfo("Current Menu: %u", s_current_menu.load(std::memory_order_acquire));
    LogInfo("In Gameplay: %s", s_is_in_gameplay.load(std::memory_order_acquire) ? "YES" : "NO");
    
    CV64_N64_Controller ctrl;
    if (CV64_Memory_ReadController(0, &ctrl)) {
        LogInfo("Controller 0: Connected=%u, Held=0x%04X, X=%d, Y=%d",
                ctrl.is_connected, ctrl.btns_held, ctrl.joy_x, ctrl.joy_y);
    } else {
        LogInfo("Controller 0: Failed to read");
    }
    
    CV64_N64_CameraMgrData camData;
    if (CV64_Memory_ReadCameraData(&camData)) {
        LogInfo("Camera: Mode=%u, Pos=(%.1f, %.1f, %.1f), LookAt=(%.1f, %.1f, %.1f)",
                camData.camera_mode,
                camData.position_x, camData.position_y, camData.position_z,
                camData.look_at_x, camData.look_at_y, camData.look_at_z);
    } else {
        LogInfo("Camera: Failed to read camera data");
    }
}

/*===========================================================================
 * Map Names Lookup Table
 * Map IDs from cv64 decomp: src/system_work.h
 *===========================================================================*/

/**
 * Map ID to name lookup table for Castlevania 64
 * Based on cv64 decomp map enum values
 */
static const char* const s_mapNames[] = {
    /* 0x00 */ "Forest of Silence",          // MORI
    /* 0x01 */ "Castle Wall - Main",           // TOU
    /* 0x02 */ "Castle Wall - Tower",          // TOUOKUJI
    /* 0x03 */ "Villa - Front Yard",           // NAKANIWA
    /* 0x04 */ "Villa - Hallway",              // HIDE
    /* 0x05 */ "Villa - Maze / Storeroom",     // TOP
    /* 0x06 */ "Tunnel",                       // EAR
    /* 0x07 */ "Underground Waterway",         // MIZU
    /* 0x08 */ "Castle Center - Machine Tower",// MACHINE_TOWER
    /* 0x09 */ "Castle Center - Main",         // MEIRO
    /* 0x0A */ "Clock Tower - Interior",       // TOKEITOU_NAI
    /* 0x0B */ "Clock Tower - Approach",       // TOKEITOU_MAE
    /* 0x0C */ "Castle Keep - Basement",       // HONMARU_B1F
    /* 0x0D */ "Room of Clocks",              // SHOKUDOU
    /* 0x0E */ "Tower of Science - Upper",     // SHINDENTOU_UE
    /* 0x0F */ "Tower of Science - Middle",    // SHINDENTOU_NAKA
    /* 0x10 */ "Duel Tower",                   // KETTOUTOU
    /* 0x11 */ "Art Tower",                    // MAKAIMURA
    /* 0x12 */ "Castle Keep - 1F",             // HONMARU_1F
    /* 0x13 */ "Castle Keep - 2F",             // HONMARU_2F
    /* 0x14 */ "Castle Keep - 3F South",       // HONMARU_3F_MINAMI
    /* 0x15 */ "Castle Keep - 4F South",       // HONMARU_4F_MINAMI
    /* 0x16 */ "Castle Keep - 3F North",       // HONMARU_3F_KITA
    /* 0x17 */ "Tower of Sorcery",             // ROSE
    /* 0x18 */ "Ending - Reinhardt",
    /* 0x19 */ "Ending - Carrie",
    /* 0x1A */ "Ending",
    /* 0x1B */ "Game Over",
    /* 0x1C */ NULL,
    /* 0x1D */ NULL,
    /* 0x1E */ NULL,
    /* 0x1F */ NULL,
};

#define CV64_MAP_COUNT (sizeof(s_mapNames) / sizeof(s_mapNames[0]))

/*===========================================================================
 * Player Status Memory Addresses
 * Verified from CASTLEVANIA.sym decomp symbols
 * SaveStruct_gameplay base: 0x80389BE4 (system_work + 0x2612C)
 *===========================================================================*/

/* Save data structure offsets (within SaveStruct_gameplay at 0x80389BE4) */
#define CV64_SAVE_OFFSET_CHARACTER      0x58   /* s16: 0=Reinhardt, 1=Carrie (at 0x80389C3C) */
#define CV64_SAVE_OFFSET_HEALTH         0x5A   /* u16: Current HP (at 0x80389C3E, sym: Player_health) */
#define CV64_SAVE_OFFSET_SUBWEAPON      0x5E   /* u16: Sub-weapon ID (at 0x80389C42) */
#define CV64_SAVE_OFFSET_GOLD           0x60   /* Player_gold (at 0x80389C44) */
#define CV64_SAVE_OFFSET_JEWELS         0x65   /* Player_red_jewels (at 0x80389C49) */
#define CV64_SAVE_OFFSET_MAP_ID         0xAC   /* map_ID_copy (at 0x80389C90) */
#define CV64_SAVE_OFFSET_DIFFICULTY     0xE4   /* save_difficulty (at 0x80389CC8) */
#define CV64_SAVE_OFFSET_POWERUP        0x108  /* current_PowerUp_level (at 0x80389CEC) */

/* Direct addresses for reading game state (verified from CASTLEVANIA.sym) */
#define CV64_DIRECT_CHARACTER_ADDR      0x80389C3C  /* sym: getCurrentCharacter reads s16 here */
#define CV64_DIRECT_HEALTH_ADDR         0x80389C3E  /* sym: Player_health */

/* Player health direct address (verified from CASTLEVANIA.sym)
 * sym: Player_health at 0x80389C3E (u16, within system_work) */
/* NOTE: CV64_PLAYER_OFFSET_HP (0x14C) in the player actor struct is unverified.
 * Use the direct system_work address CV64_DIRECT_HEALTH_ADDR (0x80389C3E) instead. */

/* Character names */
static const char* const s_characterNames[] = {
    "Reinhardt",
    "Carrie",
    " "
};

/* Subweapon names */
static const char* const s_subweaponNames[] = {
    "None",
    "Knife",
    "Holy Water",
    "Cross",
    "Axe"
};

/* Difficulty names */
static const char* const s_difficultyNames[] = {
    "Easy",
    "Normal"
};

/*===========================================================================
 * Game Info API Implementation
 *===========================================================================*/

/**
 * @brief Read current map ID from game memory
 * Uses the LIVE map_ID (not the save copy) to avoid stale values during transitions
 */
static s16 ReadCurrentMapId(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return -1;

    /* Use CV64_SYS_OFFSET_MAP_ID (live map ID at system_work + 0x26428)
     * instead of CV64_SYS_OFFSET_MAP_ID_COPY (save copy at +0x261D8)
     * to get immediate map changes without waiting for save data update */
    u32 addr = s_system_work_addr + CV64_SYS_OFFSET_MAP_ID;

    /* Debug: Try reading from the configured offset first */
    if (IsAddressValid(addr, 2)) {
        s16 mapId = (s16)CV64_ReadU16(rdram, addr);

        /* Debug: Log map ID reads periodically and on change */
        static int readCount = 0;
        static s16 lastMapId = -999;
        if (++readCount % 300 == 1 || mapId != lastMapId) {
            if (mapId != lastMapId) {
                LogInfo("Map ID CHANGED: %d -> %d (addr=0x%08X)", lastMapId, mapId, addr);
                lastMapId = mapId;
            }
        }

        /* If mapId is in valid range (0-31), return it */
        if (mapId >= 0 && mapId < 0x20) {
            return mapId;
        }

        /* Debug: If map ID seems invalid, log more details */
        static int invalidCount = 0;
        if (++invalidCount % 600 == 1) {
            LogWarning("Map ID out of range: %d (0x%04X) at 0x%08X - game may not be in gameplay yet",
                       mapId, (u16)mapId, addr);
        }
    } else {
        static bool loggedInvalid = false;
        if (!loggedInvalid) {
            LogWarning("ReadCurrentMapId: Address 0x%08X is invalid (base=0x%08X, offset=0x%X)", 
                       addr, s_system_work_addr, CV64_SYS_OFFSET_MAP_ID);
            loggedInvalid = true;
        }
    }

    return -1;
}

/**
 * @brief Get the name of a map by ID
 */
const char* CV64_Memory_GetMapName(s16 mapId) {
    if (mapId < 0 || mapId >= (s16)CV64_MAP_COUNT || s_mapNames[mapId] == NULL) {
        return "Unknown Area";
    }
    return s_mapNames[mapId];
}

/**
 * @brief Get the current map name
 */
const char* CV64_Memory_GetCurrentMapName(void) {
    s16 mapId = ReadCurrentMapId();
    return CV64_Memory_GetMapName(mapId);
}

/**
 * @brief Get the current map ID
 */
s16 CV64_Memory_GetCurrentMapId(void) {
    return ReadCurrentMapId();
}

/**
 * @brief Read player's current health from system_work
 * Uses verified address: sym Player_health at 0x80389C3E
 */
s16 CV64_Memory_GetPlayerHealth(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return -1;

    if (!IsAddressValid(CV64_DIRECT_HEALTH_ADDR, 2)) return -1;

    return (s16)CV64_ReadU16(rdram, CV64_DIRECT_HEALTH_ADDR);
}

/**
 * @brief Read player's max health
 * Uses verified address: Player_max_health at 0x80389C40 (system_work + 0x26188)
 * Follows Player_health in SaveStruct_gameplay.
 */
s16 CV64_Memory_GetPlayerMaxHealth(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return -1;

    if (!IsAddressValid(CV64_ADDR_SAVE_MAX_HEALTH, 2)) return -1;

    return (s16)CV64_ReadU16(rdram, CV64_ADDR_SAVE_MAX_HEALTH);
}

/**
 * @brief Get character name (Reinhardt or Carrie)
 * Reads s16 from verified address 0x80389C3C (sym: getCurrentCharacter)
 */
const char* CV64_Memory_GetCharacterName(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return " ";

    if (!IsAddressValid(CV64_DIRECT_CHARACTER_ADDR, 2)) return " ";

    s16 charId = (s16)CV64_ReadU16(rdram, CV64_DIRECT_CHARACTER_ADDR);
    if (charId < 0 || charId > 1) return " ";

    return s_characterNames[charId];
}

/*===========================================================================
 * Day/Night Cycle API (sym: game_time at 0x800A7900)
 *
 * CV64 day/night cycle uses game_time (u32) that counts gameplay frames.
 * The game uses a 24-hour cycle mapped to the counter. Day starts at dawn
 * and night starts at dusk. The game's isDaytime()/isNightTime() functions
 * compare game_time against threshold values.
 *
 * Approximate mapping (from decomp analysis):
 *   0x0000 - 0x2000 = Night (midnight to ~2AM)
 *   0x2000 - 0x4000 = Late Night / Dawn
 *   0x4000 - 0xC000 = Daytime
 *   0xC000 - 0xFFFF = Dusk / Night
 *
 * The exact thresholds depend on the map's time-of-day settings.
 * We use a simplified boundary: day when time & 0xFFFF is in [0x4000, 0xC000)
 *===========================================================================*/

u32 CV64_Memory_GetGameTime(void) {
    return s_game_time.load(std::memory_order_acquire);
}

bool CV64_Memory_IsDaytime(void) {
    return s_is_daytime.load(std::memory_order_acquire);
}

const char* CV64_Memory_GetTimeOfDayString(void) {
    u32 time = s_game_time.load(std::memory_order_acquire);
    if (time == 0) return " ";

    /* Use lower 16 bits for cycle position (wraps every cycle) */
    u16 cyclePos = (u16)(time & 0xFFFF);

    if (cyclePos >= 0x3800 && cyclePos < 0x4800)
        return "Dawn";
    else if (cyclePos >= 0x4800 && cyclePos < 0xB800)
        return "Day";
    else if (cyclePos >= 0xB800 && cyclePos < 0xC800)
        return "Dusk";
    else
        return "Night";
}

/*===========================================================================
 * Enhanced Game State Detection
 *===========================================================================*/

/**
 * @brief Compute detailed game state from multiple memory signals
 * Uses verified sym addresses for accurate multi-signal detection
 */
static CV64_GameState ComputeGameState(u8* rdram, s16 mapId) {
    if (!rdram) return CV64_STATE_NOT_RUNNING;

    /* Check soft reset first */
    if (IsAddressValid(CV64_ADDR_IS_SOFT_RESET, 2)) {
        u16 softReset = CV64_ReadU16(rdram, CV64_ADDR_IS_SOFT_RESET);
        if (softReset != 0)
            return CV64_STATE_SOFT_RESET;
    }

    /* Title screen / character select detection:
     * Map IDs 0+ are all real stages (0=Forest of Silence, 1=Castle Wall, etc.)
     * The title screen is NOT a map — detect it via inPlayerGameplayLoop == 0
     * and map_ID being 0 with no gameplay active. */

    /* Check fade state (map transitions) */
    if (IsAddressValid(CV64_ADDR_FADE_SETTINGS, 2)) {
        u16 fadeSettings = CV64_ReadU16(rdram, CV64_ADDR_FADE_SETTINGS);
        if (fadeSettings != 0) {
            if (IsAddressValid(CV64_ADDR_FADE_CURRENT_TIME, 2) &&
                IsAddressValid(CV64_ADDR_FADE_MAX_TIME, 2)) {
                u16 fadeTime = CV64_ReadU16(rdram, CV64_ADDR_FADE_CURRENT_TIME);
                u16 fadeMax  = CV64_ReadU16(rdram, CV64_ADDR_FADE_MAX_TIME);
                if (fadeTime > 0 && fadeTime < fadeMax)
                    return CV64_STATE_FADING;
            }
        }
    }

    /* Check menu state */
    if (IsAddressValid(CV64_ADDR_CURRENT_MENU, 2)) {
        u16 menu = CV64_ReadU16(rdram, CV64_ADDR_CURRENT_MENU);
        if (menu != 0)
            return CV64_STATE_MENU;
    }

    /* Check text reading */
    if (IsAddressValid(CV64_ADDR_READING_TEXT, 2)) {
        u16 reading = CV64_ReadU16(rdram, CV64_ADDR_READING_TEXT);
        if (reading != 0)
            return CV64_STATE_READING_TEXT;
    }

    /* Check cutscene */
    if (IsAddressValid(CV64_ADDR_CUTSCENE_ID, 2)) {
        u16 cutscene = CV64_ReadU16(rdram, CV64_ADDR_CUTSCENE_ID);
        if (cutscene != 0)
            return CV64_STATE_CUTSCENE;
    }

    /* Check first-person view */
    if (IsAddressValid(CV64_ADDR_IN_FIRST_PERSON, 4)) {
        s32 fp = (s32)CV64_ReadU32(rdram, CV64_ADDR_IN_FIRST_PERSON);
        if (fp != 0)
            return CV64_STATE_FIRST_PERSON;
    }

    /* Check gameplay loop flag */
    if (IsAddressValid(CV64_ADDR_IN_GAMEPLAY_LOOP, 4)) {
        u32 inLoop = CV64_ReadU32(rdram, CV64_ADDR_IN_GAMEPLAY_LOOP);
        if (inLoop != 0)
            return CV64_STATE_GAMEPLAY;
    }

    /* Not in gameplay loop and no other state detected.
     * Distinguish between title screen and being on a real map:
     * - Map IDs >= 0 are valid maps (0=Forest, 3=Villa Front, 4=Villa Hallway, etc.)
     * - If we have a valid map ID, we're likely in gameplay but in a temporary state
     * - If map ID is invalid (-1 or out of range), we're on the title screen */
    if (mapId >= 0 && mapId < 0x20) {
        /* We're on a valid map but gameplay loop is not active.
         * This can happen during cutscenes, loading, or other transitional states.
         * Return GAMEPLAY state so the game doesn't think we're on title screen. */
        return CV64_STATE_GAMEPLAY;
    }

    /* Invalid or no map ID — we're on title screen or loading */
    return CV64_STATE_TITLE_SCREEN;
}

CV64_GameState CV64_Memory_GetGameState(void) {
    return s_game_state.load(std::memory_order_acquire);
}

const char* CV64_Memory_GetSubweaponName(u16 type) {
    if (type <= 4) return s_subweaponNames[type];
    return " ";
}

bool CV64_Memory_IsMapTransition(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;

    if (!IsAddressValid(CV64_ADDR_FADE_CURRENT_TIME, 2)) return false;
    if (!IsAddressValid(CV64_ADDR_FADE_MAX_TIME, 2)) return false;

    u16 fadeTime = CV64_ReadU16(rdram, CV64_ADDR_FADE_CURRENT_TIME);
    u16 fadeMax  = CV64_ReadU16(rdram, CV64_ADDR_FADE_MAX_TIME);

    return (fadeTime > 0 && fadeTime < fadeMax);
}

void CV64_Memory_SetMapChangeCallback(CV64_MapChangeCallback callback) {
    s_map_change_callback = callback;
}

/*===========================================================================
 * Difficulty Toggle (sym: save_difficulty at 0x80389CC8)
 *===========================================================================*/

s32 CV64_Memory_ToggleDifficulty(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return -1;

    if (!IsAddressValid(CV64_ADDR_SAVE_DIFFICULTY, 1)) return -1;

    u8 current = CV64_ReadU8(rdram, CV64_ADDR_SAVE_DIFFICULTY);
    u8 newDiff = (current == 0) ? 1 : 0;

    if (CV64_WriteU8(rdram, CV64_ADDR_SAVE_DIFFICULTY, newDiff)) {
        LogInfo("Difficulty toggled: %s -> %s",
                (current == 0) ? "Easy" : "Normal",
                (newDiff == 0) ? "Easy" : "Normal");
        return (s32)newDiff;
    }
    return -1;
}

bool CV64_Memory_SetDifficulty(u8 difficulty) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;

    if (difficulty > 1) return false;
    if (!IsAddressValid(CV64_ADDR_SAVE_DIFFICULTY, 1)) return false;

    bool ok = CV64_WriteU8(rdram, CV64_ADDR_SAVE_DIFFICULTY, difficulty);
    if (ok) {
        LogInfo("Difficulty set to %s", (difficulty == 0) ? "Easy" : "Normal");
    }
    return ok;
}

s32 CV64_Memory_GetDifficulty(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return -1;

    if (!IsAddressValid(CV64_ADDR_SAVE_DIFFICULTY, 1)) return -1;

    return (s32)CV64_ReadU8(rdram, CV64_ADDR_SAVE_DIFFICULTY);
}

const char* CV64_Memory_GetDifficultyName(u8 difficulty) {
    if (difficulty == 0) return "Easy";
    if (difficulty == 1) return "Normal";
    return " ";
}

/*===========================================================================
 * Process Meter API (sym: processBar_sizeDivisor, greenBarSize, blueBarSize)
 *===========================================================================*/

bool CV64_Memory_GetProcessMeter(CV64_ProcessMeterInfo* out) {
    if (!out) return false;
    memset(out, 0, sizeof(CV64_ProcessMeterInfo));

    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;

    if (IsAddressValid(CV64_ADDR_PROCESS_GREEN_SIZE, 4))
        out->greenBarSize = CV64_ReadF32(rdram, CV64_ADDR_PROCESS_GREEN_SIZE);

    if (IsAddressValid(CV64_ADDR_PROCESS_BLUE_SIZE, 4))
        out->blueBarSize = CV64_ReadF32(rdram, CV64_ADDR_PROCESS_BLUE_SIZE);

    if (IsAddressValid(CV64_ADDR_PROCESS_BAR_DIVISOR, 4))
        out->sizeDivisor = CV64_ReadF32(rdram, CV64_ADDR_PROCESS_BAR_DIVISOR);

    if (IsAddressValid(CV64_ADDR_PROCESS_DIVISIONS, 4))
        out->numDivisions = CV64_ReadU32(rdram, CV64_ADDR_PROCESS_DIVISIONS);

    return true;
}

/*===========================================================================
 * Boss Health Monitor (sym: boss_bar_fill_color at 0x801804B0)
 *===========================================================================*/

bool CV64_Memory_GetBossHealth(CV64_BossHealthInfo* out) {
    if (!out) return false;
    memset(out, 0, sizeof(CV64_BossHealthInfo));

    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;

    /* Boss bar fill color: non-zero means boss bar is visible on HUD */
    if (IsAddressValid(CV64_ADDR_BOSS_BAR_FILL_COLOR, 4)) {
        out->bossBarFillColor = CV64_ReadU32(rdram, CV64_ADDR_BOSS_BAR_FILL_COLOR);
        out->bossBarVisible = (out->bossBarFillColor != 0);
    }

    return true;
}

/*===========================================================================
 * Fog & Ambient Lighting API
 *===========================================================================*/

bool CV64_Memory_GetFogInfo(CV64_FogInfo* out) {
    if (!out) return false;
    memset(out, 0, sizeof(CV64_FogInfo));

    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;

    if (IsAddressValid(CV64_ADDR_MAP_FOG_COLOR, 4))
        out->fogColor = CV64_ReadU32(rdram, CV64_ADDR_MAP_FOG_COLOR);

    if (IsAddressValid(CV64_ADDR_FOG_DIST_START, 2))
        out->fogDistanceStart = CV64_ReadU16(rdram, CV64_ADDR_FOG_DIST_START);

    if (IsAddressValid(CV64_ADDR_FOG_DIST_END, 2))
        out->fogDistanceEnd = CV64_ReadU16(rdram, CV64_ADDR_FOG_DIST_END);

    if (IsAddressValid(CV64_ADDR_MAP_AMBIENT_BRIGHT, 4))
        out->ambientBrightness = CV64_ReadF32(rdram, CV64_ADDR_MAP_AMBIENT_BRIGHT);

    if (IsAddressValid(CV64_ADDR_MAP_DIFFUSE_COLOR, 4))
        out->diffuseColor = CV64_ReadU32(rdram, CV64_ADDR_MAP_DIFFUSE_COLOR);

    if (IsAddressValid(CV64_ADDR_DONT_UPDATE_LIGHTING, 4))
        out->lightingUpdateDisabled = (CV64_ReadU32(rdram, CV64_ADDR_DONT_UPDATE_LIGHTING) != 0);

    return true;
}

/*===========================================================================
 * Save File Monitoring (sym: contPak_file_no at 0x80389CDA)
 *===========================================================================*/

s16 CV64_Memory_GetContPakFileNo(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return -1;

    if (!IsAddressValid(CV64_ADDR_CONTPAK_FILE_NO, 1)) return -1;

    return (s16)CV64_ReadU8(rdram, CV64_ADDR_CONTPAK_FILE_NO);
}

/*===========================================================================
 * Enemy Target / Lock-On Awareness
 * sym: ptr_enemyTargetGfx at 0x800D5F18 — non-zero when reticle active
 *===========================================================================*/

bool CV64_Memory_IsEnemyTargetActive(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;

    if (!IsAddressValid(CV64_ADDR_PTR_ENEMY_TARGET_GFX, 4)) return false;

    u32 ptr = CV64_ReadU32(rdram, CV64_ADDR_PTR_ENEMY_TARGET_GFX);
    return (ptr != 0);
}

u32 CV64_Memory_GetEnemyTargetPtr(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return 0;

    if (!IsAddressValid(CV64_ADDR_PTR_ENEMY_TARGET_GFX, 4)) return 0;

    return CV64_ReadU32(rdram, CV64_ADDR_PTR_ENEMY_TARGET_GFX);
}

/*===========================================================================
 * Player Status Flags
 * sym: PLAYER_HAS_MAX_HEALTH at 0x80180490 (u8)
 *===========================================================================*/

bool CV64_Memory_PlayerHasMaxHealth(void) {
    u8* rdram = SafeGetRDRAM();
    if (!rdram) return false;

    if (!IsAddressValid(CV64_ADDR_HAS_MAX_HEALTH, 1)) return false;

    return (CV64_ReadU8(rdram, CV64_ADDR_HAS_MAX_HEALTH) != 0);
}

/**
 * @brief Read all extended save data fields from RDRAM
 * Called from GetGameInfo and during frame update
 */
static void ReadExtendedSaveData(u8* rdram, CV64_GameInfo* info) {
    /* Gold (u32 at 0x80389C44) — sym: Player_gold */
    if (IsAddressValid(CV64_ADDR_SAVE_GOLD, 4))
        info->gold = CV64_ReadU32(rdram, CV64_ADDR_SAVE_GOLD);

    /* Red jewels (u8 at 0x80389C49) — sym: Player_red_jewels */
    if (IsAddressValid(CV64_ADDR_SAVE_JEWELS, 1))
        info->jewels = CV64_ReadU8(rdram, CV64_ADDR_SAVE_JEWELS);

    /* Subweapon type (u16 at 0x80389C42) */
    if (IsAddressValid(CV64_ADDR_SAVE_SUBWEAPON, 2))
        info->subweapon = CV64_ReadU16(rdram, CV64_ADDR_SAVE_SUBWEAPON);

    /* Subweapon ammo (u16 at 0x80389C48) */
    if (IsAddressValid(CV64_ADDR_SAVE_SUBWEAPON_AMMO, 2))
        info->subweaponAmmo = CV64_ReadU16(rdram, CV64_ADDR_SAVE_SUBWEAPON_AMMO);

    /* Powerup level (u8 at 0x80389CEC) — sym: current_PowerUp_level */
    if (IsAddressValid(CV64_ADDR_SAVE_POWERUP, 1))
        info->powerupLevel = CV64_ReadU8(rdram, CV64_ADDR_SAVE_POWERUP);

    /* Alternate costume (u8 at 0x80389CEE) — sym: alternate_costume */
    if (IsAddressValid(CV64_ADDR_SAVE_COSTUME, 1))
        info->alternateCostume = CV64_ReadU8(rdram, CV64_ADDR_SAVE_COSTUME);

    /* Difficulty (u8 at 0x80389CC8) — sym: save_difficulty */
    if (IsAddressValid(CV64_ADDR_SAVE_DIFFICULTY, 1))
        info->difficulty = CV64_ReadU8(rdram, CV64_ADDR_SAVE_DIFFICULTY);

    /* Save file number (u8 at 0x80389CDB) — sym: save_file_number */
    if (IsAddressValid(CV64_ADDR_SAVE_FILE_NUMBER, 1))
        info->saveFileNumber = CV64_ReadU8(rdram, CV64_ADDR_SAVE_FILE_NUMBER);

    /* Map entrance ID (u16 at 0x80389C92) — sym: map_entrance_ID_copy */
    if (IsAddressValid(CV64_ADDR_SAVE_MAP_ENTRANCE, 2))
        info->mapEntrance = (s16)CV64_ReadU16(rdram, CV64_ADDR_SAVE_MAP_ENTRANCE);

    /* Player position from current_player_position (Vec3f at 0x8009E1B0) */
    u32 phys = CV64_VirtToPhys(CV64_ADDR_CURRENT_PLAYER_POS);

    if (IsAddressValid(phys, 12)) {
        info->playerX = CV64_ReadF32(rdram, phys);
        info->playerY = CV64_ReadF32(rdram, phys + 4);
        info->playerZ = CV64_ReadF32(rdram, phys + 8);
    }


    /* Cutscene ID */
    if (IsAddressValid(CV64_ADDR_CUTSCENE_ID, 2))
        info->cutsceneId = CV64_ReadU16(rdram, CV64_ADDR_CUTSCENE_ID);

    /* Camera mode */
    info->cameraMode = s_current_camera_mode.load(std::memory_order_acquire);

    /* First person flag */
    if (IsAddressValid(CV64_ADDR_IN_FIRST_PERSON, 4))
        info->isFirstPerson = (CV64_ReadU32(rdram, CV64_ADDR_IN_FIRST_PERSON) != 0);

    /* R-lock flag */
    if (IsAddressValid(CV64_ADDR_R_LOCK_VIEW, 4))
        info->isRLocked = (CV64_ReadU32(rdram, CV64_ADDR_R_LOCK_VIEW) != 0);

    /* Game time & day/night */
    info->gameTime = s_game_time.load(std::memory_order_acquire);
    info->isDaytime = s_is_daytime.load(std::memory_order_acquire);

    /* Soft reset */
    info->isSoftReset = s_is_soft_reset.load(std::memory_order_acquire);

    /* Detailed game state */
    info->gameState = s_game_state.load(std::memory_order_acquire);

    /* Process meter (frame budget) */
    if (IsAddressValid(CV64_ADDR_PROCESS_GREEN_SIZE, 4))
        info->processMeterGreen = CV64_ReadF32(rdram, CV64_ADDR_PROCESS_GREEN_SIZE);
    if (IsAddressValid(CV64_ADDR_PROCESS_BLUE_SIZE, 4))
        info->processMeterBlue = CV64_ReadF32(rdram, CV64_ADDR_PROCESS_BLUE_SIZE);

    /* Boss bar visibility */
    if (IsAddressValid(CV64_ADDR_BOSS_BAR_FILL_COLOR, 4))
        info->bossBarVisible = (CV64_ReadU32(rdram, CV64_ADDR_BOSS_BAR_FILL_COLOR) != 0);

    /* Fog info */
    if (IsAddressValid(CV64_ADDR_FOG_DIST_START, 2))
        info->fogDistStart = CV64_ReadU16(rdram, CV64_ADDR_FOG_DIST_START);
    if (IsAddressValid(CV64_ADDR_FOG_DIST_END, 2))
        info->fogDistEnd = CV64_ReadU16(rdram, CV64_ADDR_FOG_DIST_END);
    if (IsAddressValid(CV64_ADDR_MAP_AMBIENT_BRIGHT, 4))
        info->ambientBrightness = CV64_ReadF32(rdram, CV64_ADDR_MAP_AMBIENT_BRIGHT);

    /* Controller pak file number */
    if (IsAddressValid(CV64_ADDR_CONTPAK_FILE_NO, 1))
        info->contPakFileNo = (s16)CV64_ReadU8(rdram, CV64_ADDR_CONTPAK_FILE_NO);
    else
        info->contPakFileNo = -1;

    /* Enemy target lock-on */
    if (IsAddressValid(CV64_ADDR_PTR_ENEMY_TARGET_GFX, 4))
        info->isEnemyTargetActive = (CV64_ReadU32(rdram, CV64_ADDR_PTR_ENEMY_TARGET_GFX) != 0);

    /* Player has max health */
    if (IsAddressValid(CV64_ADDR_HAS_MAX_HEALTH, 1))
        info->hasMaxHealth = (CV64_ReadU8(rdram, CV64_ADDR_HAS_MAX_HEALTH) != 0);
}

/**
 * @brief Get full game info structure for window title display & PC HUD
 */
bool CV64_Memory_GetGameInfo(CV64_GameInfo* info) {
    if (!info) return false;

    /* Initialize with defaults */
    memset(info, 0, sizeof(CV64_GameInfo));
    info->mapId = -1;
    info->mapEntrance = -1;
    info->health = -1;
    info->maxHealth = -1;
    info->gameState = CV64_STATE_NOT_RUNNING;
    info->contPakFileNo = -1;

    u8* rdram = SafeGetRDRAM();
    if (!s_initialized.load(std::memory_order_acquire) || !rdram) {
        strncpy(info->mapName, "Not Running", sizeof(info->mapName) - 1);
        strncpy(info->characterName, " ", sizeof(info->characterName) - 1);
        return false;
    }

    /* Read map info */
    info->mapId = ReadCurrentMapId();

    const char* mapName = CV64_Memory_GetMapName(info->mapId);
    strncpy(info->mapName, mapName, sizeof(info->mapName) - 1);

    /* Read health — always try, don't require player pointer */
    info->health = CV64_Memory_GetPlayerHealth();
    info->maxHealth = CV64_Memory_GetPlayerMaxHealth();

    /* Get character name */
    const char* charName = CV64_Memory_GetCharacterName();
    strncpy(info->characterName, charName, sizeof(info->characterName) - 1);

    /* Check if in gameplay */
    info->isInGameplay = s_is_in_gameplay.load(std::memory_order_acquire);

    /* Read all extended data (gold, jewels, subweapon, position, etc.) */
    ReadExtendedSaveData(rdram, info);

    return true;
}
