#pragma once
#include "KhaosRenderBase.h"

namespace Khaos
{
    class GeneralRender : public CommRenderBase
    {
    public:
        GeneralRender();
        virtual ~GeneralRender();

    public:
        virtual void init();
    };

    extern GeneralRender* g_generalRender;
}

