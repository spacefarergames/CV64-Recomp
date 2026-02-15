/**
 * @file cv64_types.h
 * @brief Castlevania 64 PC Recomp - Core Type Definitions
 * 
 * This header provides cross-platform type definitions compatible with
 * the original N64 types from the decomp project while targeting modern PC.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_TYPES_H
#define CV64_TYPES_H

/* Ensure Windows headers are included first to avoid conflicts */
#if defined(_WIN32) || defined(_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * N64 Compatible Type Definitions
 *===========================================================================*/

/* Unsigned integer types */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Signed integer types */
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

/* Floating point types */
typedef float    f32;
typedef double   f64;

/* Size types for N64 compatibility */
typedef uintptr_t uintptr;
typedef intptr_t  intptr;

/*===========================================================================
 * N64 Ultra64 Compatible Definitions
 *===========================================================================*/

/* N64 Display List command type */
typedef u64 Gfx;

/* N64 Matrix types */
typedef f32 Mat4f[4][4];
typedef s16 Mtx[4][4][2];

/*===========================================================================
 * Vector Types (from cv64 decomp)
 *===========================================================================*/

typedef struct Vec2 {
    s16 x;
    s16 y;
} Vec2;

typedef struct Vec2f {
    f32 x;
    f32 y;
} Vec2f;

typedef struct Vec3 {
    s16 x;
    s16 y;
    s16 z;
} Vec3;

typedef struct Vec3f {
    f32 x;
    f32 y;
    f32 z;
} Vec3f;

typedef struct Vec4f {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} Vec4f;

/*===========================================================================
 * Angle Type (from cv64 decomp)
 *===========================================================================*/

typedef struct Angle {
    s16 pitch;
    s16 yaw;
    s16 roll;
    s16 flags;
} Angle;

/*===========================================================================
 * N64 Controller Definitions
 *===========================================================================*/

/* Maximum controllers supported */
#define MAXCONTROLLERS 4

/* Controller button masks - N64 layout matching mupen64plus BUTTONS */
#define CONT_A          0x0080
#define CONT_B          0x0040
#define CONT_G          0x0020  /* Z trigger */
#define CONT_START      0x0010
#define CONT_UP         0x0008
#define CONT_DOWN       0x0004
#define CONT_LEFT       0x0002
#define CONT_RIGHT      0x0001
#define CONT_L          0x2000
#define CONT_R          0x1000
#define CONT_E          0x0800  /* C-Up */
#define CONT_D          0x0400  /* C-Down */
#define CONT_C          0x0200  /* C-Left */
#define CONT_F          0x0100  /* C-Right */

/* D-Pad masks (for camera control patch) */
#define CONT_DPAD_UP    CONT_UP
#define CONT_DPAD_DOWN  CONT_DOWN
#define CONT_DPAD_LEFT  CONT_LEFT
#define CONT_DPAD_RIGHT CONT_RIGHT

/*===========================================================================
 * Memory and Address Macros
 *===========================================================================*/

/* N64 memory segment macros - adapted for PC */
#define SEGMENT_NUMBER(a)   (((u32)(a) >> 24) & 0x0F)
#define SEGMENT_OFFSET(a)   ((u32)(a) & 0x00FFFFFF)
#define SEGMENT_ADDRESS(s, o) (((s) << 24) | (o))

/*===========================================================================
 * Boolean and Status Definitions
 *===========================================================================*/

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL  ((void*)0)
#endif

/*===========================================================================
 * Platform Detection
 *===========================================================================*/

#if defined(_WIN32) || defined(_WIN64)
    #define CV64_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define CV64_PLATFORM_LINUX 1
#elif defined(__APPLE__)
    #define CV64_PLATFORM_MACOS 1
#endif

/*===========================================================================
 * Attribute Macros
 *===========================================================================*/

#ifdef _MSC_VER
    #define CV64_ALIGNED(x)    __declspec(align(x))
    #define CV64_INLINE        __forceinline
    #define CV64_NOINLINE      __declspec(noinline)
    #define CV64_EXPORT        __declspec(dllexport)
    #define CV64_IMPORT        __declspec(dllimport)
#else
    #define CV64_ALIGNED(x)    __attribute__((aligned(x)))
    #define CV64_INLINE        __attribute__((always_inline)) inline
    #define CV64_NOINLINE      __attribute__((noinline))
    #define CV64_EXPORT        __attribute__((visibility("default")))
    #define CV64_IMPORT
#endif

#ifdef CV64_BUILD_DLL
    #define CV64_API CV64_EXPORT
#elif defined(CV64_IMPORT_DLL)
    #define CV64_API CV64_IMPORT
#else
    /* Static build - no import/export needed */
    #define CV64_API
#endif

#ifdef __cplusplus
}
#endif

#endif /* CV64_TYPES_H */
