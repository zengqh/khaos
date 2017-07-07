#pragma once
#include "KhaosRenderTarget.h"

namespace Khaos
{
    class RenderBufferPool;
    struct EffSetterParams;
    class ImageProcess;

    class GBuffer : public AllocatedObject
    {
    public:
        GBuffer();
        ~GBuffer();

        void init();
        void clean();

        void prepareResource( RenderBufferPool& renderBufferPool, TextureObj* sceneBuffer );
        void beginRender( Camera* currCamera );
        void endRender( EffSetterParams& params );

    public:
        TextureObj* getDiffuseBuffer();
        TextureObj* getSpecularBuffer();
        TextureObj* getNormalBuffer();
        TextureObj* getDepthBuffer();

    private:
        void _convertDepth( EffSetterParams& params );

    private:
        RenderTarget m_gBuf;
        TextureObj*  m_depthLinearBuf;
        ImageProcess* m_procConverter;
    };
}

