#include "KhaosPreHeaders.h"
#include "KhaosRenderSystem.h"
#include "KhaosSceneGraph.h"
#include "KhaosGeneralRender.h"
#include "KhaosRenderDevice.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class _SGFindEnvReflByCamera : public SGFindBase
    {
    public:
        _SGFindEnvReflByCamera()
        {
            KHAOS_SFG_SET_RESULT( _SGFindEnvReflByCamera, NT_RENDER, _onFindRenderNode );
        }

    public:
        void _onFindRenderNode( SceneNode* node )
        {
            static_cast<RenderableSceneNode*>(node)->_addToRenderer();
        }
    };

    //////////////////////////////////////////////////////////////////////////
    void RenderSystem::_generateEnvMaps( EffSetterParams& params )
    {
        // 生成envprobe map
        KHAOS_FOR_EACH_CONST( EnvProbeNodeList, m_envpResultsMain, it )
        {
            _generateEnvProbe( params, *it );
        }
    }

    void RenderSystem::_generateEnvProbe( EffSetterParams& params, EnvProbeNode* envProbeNode )
    {
        // 分配rtt
        EnvProbe* envProbe = envProbeNode->getEnvProbe();
        int rttSize = envProbe->getResolution();

        TextureObj* rtt = m_renderBufferPool.getRTTCubeBuffer( PIXFMT_A8R8G8B8, rttSize, &(envProbeNode->getName()) );
        RenderBufferPool::ItemTemp depthTmp = m_renderBufferPool.getDepthBufferTemp( rttSize, rttSize );

        RenderTargetCube* renderTarget = envProbe->getRenderTarget();
        renderTarget->linkRTT( rtt );
        renderTarget->linkDepth( depthTmp );
        renderTarget->setClearColor( g_backColor ); // 这里没有画天空，暂时使用背景色

        // 渲染每个cube face
        m_renderPassStage = MP_REFLECT;
        params.clear();

        for ( int i = 0; i < 6; ++i )
        {
            m_currCamera = renderTarget->getViewport( (CubeMapFace)i )->getCamera();
            renderTarget->beginRender( (CubeMapFace)i, 0 );

            g_generalRender->beginAdd( m_renderPassStage );
            _SGFindEnvReflByCamera finder;
            finder.setFinder( m_currCamera );
            finder.find( m_sceneGraph );
            g_generalRender->endAdd();

            params.stampGlobal();

            g_generalRender->renderSolidTransWS(params);

            renderTarget->endRender();
        }
    }
}

