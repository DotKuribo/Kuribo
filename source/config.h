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

enum kuribo_platform {
    KURIBO_PLATFORM_PC, //!< Windows emulation and unit testing
    KURIBO_PLATFORM_GC, //!< When compiling for the cube.
    KURIBO_PLATFORM_WII, //!< It's a revolution.

    KURIBO_PLATFORM_UNKNOWN
};

#ifdef KURIBO_PLATFORM_WII
#define KURIBO_PLATFORM KURIBO_PLATFORM_WII
#else
#define KURIBO_PLATFORM KURIBO_PLATFORM_UNKNOWN
#endif

#ifdef __cplusplus
namespace kuribo {
    enum class platform {
        PC, GC, Wii, Unknown
    };
    constexpr platform BuildPlatform = static_cast<platform>(KURIBO_PLATFORM);
}
#endif

#ifdef KURIBO_DEBUG
#define KURIBO_ENABLE_ASSERT
#define KURIBO_ENABLE_LOG
#endif