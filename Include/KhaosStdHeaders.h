#pragma once
#include "KhaosConfig.h"
#include "KhaosPlatform.h"

// 标准c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <io.h>

// 标准c++
#include <limits>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <type_traits>

#if KHAOS_COMPILER == KHAOS_COMPILER_MSVC
    #if _MSC_VER >= 1600 // vc10
        #include <unordered_map>
        #include <unordered_set>
    #else // vc9
        #include <unordered_map>
        #include <unordered_set>
    #endif
    
    #define unordered_map_  std::tr1::unordered_map
    #define unordered_set_  std::tr1::unordered_set
    #define hash_           std::tr1::hash
#else
    #include <tr1/unordered_map>
    #include <tr1/unordered_set>

    #define unordered_map_  std::unordered_map
    #define unordered_set_  std::unordered_set
    #define hash_           std::hash
#endif

