#pragma once
#include "KhaosRenderBase.h"

namespace Khaos
{
    class LightNode;

    class LightRender : public RenderBase
    {
    public:
        LightRender();
        virtual ~LightRender();

    public:
        virtual void init();
        virtual void shutdown() {}
        
        void clear();
        void addLight( LightNode* litNode );
        void renderLights( EffSetterParams& params );

    private:
        KindRenderableList m_points[2];
        KindRenderableList m_spots[2];
    };

    extern LightRender* g_lightRender;
}

