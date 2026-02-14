#ifndef PLATFORM_H
#define PLATFORM_H

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_LINUX   0
    #define PLATFORM_MACOS   0
#elif defined(__linux__)
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_LINUX   1
    #define PLATFORM_MACOS   0
#elif defined(__APPLE__)
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_LINUX   0
    #define PLATFORM_MACOS   1
#else
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_LINUX   0
    #define PLATFORM_MACOS   0
#endif

// Compiler detection
#if defined(_MSC_VER)
    #define COMPILER_MSVC 1
#else
    #define COMPILER_MSVC 0
#endif

// MSVC-specific extensions -> cross-platform equivalents
#if COMPILER_MSVC
    #define YDS_FORCE_INLINE __forceinline
    #define YDS_ALIGNED_MALLOC(size, align) _aligned_malloc(size, align)
    #define YDS_ALIGNED_FREE(ptr)           _aligned_free(ptr)
#else
    #define YDS_FORCE_INLINE inline __attribute__((always_inline))
    #include <stdlib.h>
    #define YDS_ALIGNED_MALLOC(size, align) aligned_alloc(align, size)
    #define YDS_ALIGNED_FREE(ptr)           free(ptr)
#endif

// strcpy_s / sprintf_s shims for non-MSVC
#if !COMPILER_MSVC
    #include <string.h>
    #include <stdio.h>
    #ifndef strcpy_s
        #define strcpy_s(dst, size, src) strncpy(dst, src, size)
    #endif
    #ifndef sprintf_s
        #define sprintf_s(buf, size, fmt, ...) snprintf(buf, size, fmt, ##__VA_ARGS__)
    #endif
#endif

// MAX_PATH shim
#if !PLATFORM_WINDOWS
    #ifndef MAX_PATH
        #define MAX_PATH 260
    #endif
#endif

#endif /* PLATFORM_H */