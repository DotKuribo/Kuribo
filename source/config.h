#pragma once

#if defined(_DEBUG) && !defined(NDEBUG)
#ifndef KURIBO_DEBUG
#define KURIBO_DEBUG
#endif
#endif

#if defined(_NDEBUG) || !defined(KURIBO_DEBUG)
#ifndef KURIBO_RELEASE
#define KURIBO_RELEASE
#endif

#ifdef KURIBO_DEBUG
#undef KURIBO_DEBUG
#endif
#endif

#if defined(KURIBO_DEBUG) && defined(KURIBO_RELEASE)
#pragma error "Cannot build for debug and release."
#endif

#define KURIBO_PL_TYPE_PC 0  //!< Windows emulation and unit testing
#define KURIBO_PL_TYPE_GC 1  //!< When compiling for the cube.
#define KURIBO_PL_TYPE_WII 2 //!< It's a revolution.
#define KURIBO_PL_TYPE_UNKNOWN 3

#ifdef KURIBO_PLATFORM_WII
#define KURIBO_PLATFORM KURIBO_PL_TYPE_WII
#else
#define KURIBO_PLATFORM KURIBO_PL_TYPE_UNKNOWN
#endif

#ifdef __cplusplus
namespace kuribo {
enum class platform { PC, GC, Wii, Unknown };
constexpr platform BuildPlatform = static_cast<platform>(KURIBO_PLATFORM);
} // namespace kuribo
#endif

#ifdef KURIBO_DEBUG
#define KURIBO_ENABLE_ASSERT
#define KURIBO_ENABLE_LOG
#endif