#pragma once
#include "KhaosRenderBase.h"

namespace Khaos
{
    class ImageProcess;
    class SkyLayer;

    class SkyRender : public RenderBase
    {
    public:
        SkyRender();
        virtual ~SkyRender();

    public:
        virtual void init();
        virtual void shutdown() {}

        void renderBackground( EffSetterParams& params );

    public:
        const Color& _getCurrentSimpleColor() const { return m_simpleClr; }

    private:
        void _renderSimple( EffSetterParams& params, SkyLayer* layer );

    private:
        ImageProcess* m_simpleRender;
        Color         m_simpleClr;
    };

    extern SkyRender* g_skyRender;
}

