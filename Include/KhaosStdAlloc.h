#pragma once
#include "KhaosStdHeaders.h"

namespace Khaos
{
    class StdAlloc
    {
    public:
        static void* allocBytes( size_t size )
        {
            return ::operator new( size );
        }

        static void  freeBytes( void* p )
        {
            return ::operator delete( p );
        }
    };
}

