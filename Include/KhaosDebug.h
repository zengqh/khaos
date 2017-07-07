#pragma once
#include "KhaosStdHeaders.h"
#include "KhaosUtil.h"

namespace Khaos
{
    // 我有两台电脑哦
#define _INST_ROOT_DIR_1_ "c:/mayhcat/alien/khaos/"
#define _INST_ROOT_DIR_2_ "d:/mayhcat/alien/khaos/"
#define _INST_ROOT_DIR_3_ "d:/mayhcat/my_proj/alien/khaos/"

#define _MY_PROJ_ROOT_DIR       _INST_ROOT_DIR_2_
#define _MAKE_PROJ_ROOT_DIR(x)  KHAOS_PP_CAT(_MY_PROJ_ROOT_DIR, x)

    void _outputDebugStr( const char* fmt, ... );
    void _outputDebugStrLn( const char* fmt, ... );

#if KHAOS_DEBUG
    #define khaosOutputStr      Khaos::_outputDebugStr
    #define khaosOutputStrLn    Khaos::_outputDebugStrLn
    #define khaosAssert         assert
#else
    #define khaosOutputStr
    #define khaosOutputStrLn
    #define khaosAssert
#endif

    #define KhaosStaticAssert(x) enum { KHAOS_PP_CAT(khaos_detail_static_assert_value_, __LINE__) = sizeof(char[x]) }
}

