#pragma once
#include "KhaosRenderBase.h"

namespace Khaos
{
    class VolumeProbeNode;

    class VolumeProbeRender : public RenderBase
    {
    public:
        VolumeProbeRender();
        virtual ~VolumeProbeRender();

    public:
        virtual void init();
        virtual void shutdown() {}
        
        void clear();
        void addVolumeProbe( VolumeProbeNode* node );
        void renderVolumeProbes( EffSetterParams& params );

    private:
        KindRenderableList m_nodeList[2];
    };

    extern VolumeProbeRender* g_volumeProbeRender;
}

