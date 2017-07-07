#pragma once
#include "KhaosSharedPtr.h"

namespace Khaos
{
    class RSCmd : public AllocatedObject
    {
    public:
        virtual ~RSCmd() {}

        virtual void doCmd() = 0;
    };

    typedef SharedPtr<RSCmd> RsCmdPtr;
}

