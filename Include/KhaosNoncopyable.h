#pragma once
#include "KhaosStdHeaders.h"

namespace Khaos
{
    class Noncopyable
    {
    protected:
        Noncopyable()  {}
        ~Noncopyable() {}

    private:
        Noncopyable( const Noncopyable& );
        Noncopyable& operator=( const Noncopyable& );
    };
}

