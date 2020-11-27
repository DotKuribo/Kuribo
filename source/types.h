#pragma once

#include <stdint.h>

typedef int8_t 		s8;
typedef uint8_t 	u8;
typedef int16_t 	s16;
typedef uint16_t 	u16;
typedef int32_t		s32;
typedef uint32_t 	u32;
typedef uint64_t  u64;

typedef float 		f32;
typedef double 		f64;

#if defined(__GNUC__) || defined(__clang__)
#define KX_EXPORT __attribute__((visibility("default")))
#else
#define KX_EXPORT
#endif