#pragma once
#include "KhaosStdHeaders.h"

namespace Khaos
{
    class DefaultAlloc
    {
    public:
        static void* allocBytes( size_t size );
        static void  freeBytes( void* p );
    };
}

