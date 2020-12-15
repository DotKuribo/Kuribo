#pragma once

#include <stdint.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#if defined(__GNUC__) || defined(__clang__)
#define KX_EXPORT __attribute__((visibility("default")))
#else
#define KX_EXPORT
#endif

static inline u32 swap32(u32 v);
static inline u16 swap16(u16 v);

#define _BSWAP_16(v) (((v & 0xff00) >> 8) | ((v & 0x00ff) << 8))

#define _BSWAP_32(v)                                                           \
  (((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8) |                        \
   ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24))

#if OISHII_PLATFORM_LE == 1
#define MAKE_BE32(x) _BSWAP_32(x)
#define MAKE_LE32(x) x
#else
#define MAKE_BE32(x) x
#define MAKE_LE32(x) _BSWAP_32(x)
#endif

#if defined(__llvm__) || (defined(__GNUC__) && !defined(__ICC))
static inline u32 swap32(u32 v) { return __builtin_bswap32(v); }
static inline u16 swap16(u16 v) { return _BSWAP_16(v); }
#elif defined(_MSC_VER)
#include <stdlib.h>
static inline u32 swap32(u32 v) { return _byteswap_ulong(v); }
static inline u16 swap16(u16 v) { return _byteswap_ushort(v); }
#else
static inline u32 swap32(u32 v) { return _BSWAP_32(v); }
static inline u16 swap16(u16 v) { return _BSWAP_16(v); }
#endif

template <typename T> constexpr T roundDown(T in, u32 align) {
  return align ? in & ~(static_cast<T>(align) - 1) : in;
}

template <typename T> constexpr T roundUp(T in, u32 align) {
  T t_align = static_cast<T>(align);
  return align ? roundDown(in + (t_align - 1), align) : in;
}