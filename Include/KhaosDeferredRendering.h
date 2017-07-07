#pragma once
#include "KhaosRenderBufferPool.h"

namespace Khaos
{
    class Camera;
    class ImgDepthBoundInit;
    class ImgDepthBoundNext;
    struct EffSetterParams;

    class TBDR : public AllocatedObject
    {
    public:
        TBDR();
        ~TBDR();

    public:
        void init();
        void shutdown();

        void prepare( Camera* cam );
        void generalLightBuffer( EffSetterParams& params );
        void retrieveLightBuffer();

    private:
        void _buildDepthBound( EffSetterParams& params, RenderBufferPool::ItemTemp& bufBound );
        void _buildLightBuffer( EffSetterParams& params, RenderBufferPool::ItemTemp& bufBound );

    private:
        Camera* m_cam;
        int m_vpWidth;
        int m_vpHeight;
        int m_xgrids;
        int m_ygrids;

        RenderBufferPool::Item* m_litBuff;

        ImgDepthBoundInit* m_procDepthBoundInit;
        ImgDepthBoundNext* m_procDepthBoundNext;
    };
}

