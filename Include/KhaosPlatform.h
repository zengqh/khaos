#pragma once

// 编译平台
#define KHAOS_PLATFORM_WIN32    1
#define KHAOS_PLATFORM_IOS      2
#define KHAOS_PLATFORM_ANDROID  3

// 编译器
#define KHAOS_COMPILER_MSVC     1
#define KHAOS_COMPILER_GCC      2

// 字节序
#define KHAOS_ENDIAN_LITTLE     1
#define KHAOS_ENDIAN_BIG        2

// 32/64架构
#define KHAOS_ARCHITECTURE_32   1
#define KHAOS_ARCHITECTURE_64   2


// 测试参数
#if defined(__WIN32__) || defined(_WIN32)
    #define KHAOS_PLATFORM  KHAOS_PLATFORM_WIN32
#else
    #pragma error "Unknown platform."
#endif

#if defined(_MSC_VER)
    #define KHAOS_COMPILER  KHAOS_COMPILER_MSVC
#else
    #pragma error "Unknown compiler."
#endif

#if defined(__x86_64__) || defined(_M_X64)
    #define KHAOS_ARCH_TYPE KHAOS_ARCHITECTURE_64
#else
    #define KHAOS_ARCH_TYPE KHAOS_ARCHITECTURE_32
#endif

// 不同平台配置
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
    #define KHAOS_ENDIAN KHAOS_ENDIAN_LITTLE

    #if defined(_UNICODE) || defined(UNICODE)
        #define KHAOS_UNICODE   1
    #else
        #define KHAOS_UNICODE   0
    #endif

    #if defined(_DEBUG) || defined(DEBUG)
        #define KHAOS_DEBUG 1
    #else
        #define KHAOS_DEBUG 0
    #endif
#endif

