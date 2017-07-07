#pragma once
#include "KhaosConfig.h"
#include "KhaosPlatform.h"

#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #if !defined(NOMINMAX) && defined(_MSC_VER)
        #define NOMINMAX // required to stop windows.h messing up std::min
    #endif
    
    #include <windows.h>
    #include <process.h>
    #include <mmsystem.h>
#endif

