#pragma once
#include "KhaosStdTypes.h"

namespace Khaos
{
    // int to ype
    template<uint32 n>
    struct IntToType
    {
        static const uint32 value = n;
    };
}

