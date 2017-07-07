#include "KhaosPreHeaders.h"
#include "KhaosRenderSystem.h"
#include "KhaosSceneGraph.h"
#include "KhaosGeneralRender.h"
#include "KhaosRenderDevice.h"
#include "KhaosLightRender.h"
#include "KhaosVolumeProbeRender.h"
#include "KhaosImageProcess.h"
#include "KhaosImageProcessComposite.h"

namespace Khaos
{
    void RenderSystem::_generateGBuffers( EffSetterParams& params )
    {
        _prepareSceneBuffers();

        if ( m_renderMode == RM_FORWARD )
            return;

        m_gBuf.prepareResource( m_renderBufferPool, m_sceneBuf );

        if ( m_currSSAOEnabled ) // 需要half/quarter depth buffer
        {
            int depthWidth  = m_mainCamera->getViewportWidth();
            int depthHeight = m_mainCamera->getViewportHeight();
            
            int depthHalfWidth  = depthWidth / 2;
            int depthHalfHeight = depthHeight / 2;

            int depthQuarterWidth  = depthHalfWidth / 2;
            int depthQuarterHeight = depthHalfHeight / 2;

            m_depthHalf = m_renderBufferPool.getRTTBuffer( PIXFMT_32F, depthHalfWidth, depthHalfHeight );
            m_depthQuarter = m_renderBufferPool.getRTTBuffer( PIXFMT_32F, depthQuarterWidth, depthQuarterHeight );
        }

        // 派发主视角数据
        m_currCamera = m_mainCamera;
        m_renderPassStage = MP_DEFPRE;
        g_generalRender->beginAdd( m_renderPassStage );

        KHAOS_FOR_EACH_CONST( RenderableSceneNodeList, m_renderResultsMain, it )
        {
            RenderableSceneNode* node = *it;
            node->_addToRenderer(); // 每个节点将可渲染体加入所属的渲染器
        }

        g_generalRender->endAdd();

        // 渲染
        params.clearAndStampGlobal();
        m_gBuf.beginRender( m_currCamera );
        g_renderDevice->enableStencil( true );
        g_generalRender->renderSolidWS( params );
        g_renderDevice->enableStencil( false );
        m_gBuf.endRender( params );
    }

    void RenderSystem::_generateLitBuffers( EffSetterParams& params )
    {
        if ( m_renderMode == RM_FORWARD )
            return;

        // 得到灯光队列
        m_currCamera = m_mainCamera;
        m_renderPassStage = MP_LITACC;
        g_lightRender->clear();
        g_volumeProbeRender->clear();

        KHAOS_FOR_EACH( LightNodeList, m_litResultsMain, it )
        {
            g_lightRender->addLight(*it);
        }

        KHAOS_FOR_EACH( VolumeProbeNodeList, m_volpResultsMain, it )
        {
            VolumeProbeNode* node = *it;
            if ( node->getProbe()->isPrepared() )
                g_volumeProbeRender->addVolumeProbe( node );
        }

        // 累加各光线贡献
        params.clearAndStampGlobal();

        _setSceneBuffer();
        _setDrawOnly( RK_OPACITY );
        
        // 清理buffer,NB: 以后不在这里，defer pre就清
        //g_renderDevice->clear( RCF_TARGET, &Color::ZERO, 1.0f, 0 );

        g_lightRender->renderLights( params );

        g_volumeProbeRender->renderVolumeProbes( params );

        _unsetDrawOnly();
        _unsetSceneBuffer();
    }

    void RenderSystem::_generateComposite( EffSetterParams& params )
    {
        if ( m_renderMode == RM_FORWARD )
            return;

        _setDrawOnly( RK_OPACITY );
        g_imageProcessManager->getProcessComposite()->setOutput( m_sceneBuf );
        g_imageProcessManager->getProcessComposite()->process( params );
        _unsetDrawOnly();
    }
}

