#include "KhaosPreHeaders.h"
#include "KhaosRenderSystem.h"
#include "KhaosSceneGraph.h"
#include "KhaosGeneralRender.h"
#include "KhaosRenderDevice.h"
#include "KhaosLightRender.h"
#include "KhaosImageProcess.h"
#include "KhaosImageProcessShadow.h"
#include "KhaosImageProcessHDR.h"
#include "KhaosImageProcessAA.h"

namespace Khaos
{
    void RenderSystem::_processPin( EffSetterParams& params, ImagePin* pin )
    {
        // 设置
        m_currCamera = m_mainCamera; // 目前就使用main camera
        m_renderPassStage = MP_ETC;
        params.clearAndStampGlobal();

        if ( pin->isMainRTTUsed() ) // 使用当前主camera的rtt设置
        {
            m_currCamera->_setViewport(0);
            g_renderDevice->restoreMainRTT(); // only main window current
        }
        else // 使用pin的设置
        {
            // 请求临时tex资源
            pin->requestRttRes();

            RenderTarget* rtt = pin->getOutput();
            int rttWidth = rtt->getWidth(pin->getOutRTTLevel());
            int rttHeight = rtt->getHeight(pin->getOutRTTLevel());

            // 绑定临时depth
            RenderBufferPool::ItemTemp depthTmp;

            if ( pin->isMainDepthUsed() ) // 使用主深度
            {
                khaosAssert(rttWidth == g_renderDevice->getMainDepthStencil()->getWidth() );
                khaosAssert(rttHeight == g_renderDevice->getMainDepthStencil()->getHeight() );
                rtt->linkDepth( g_renderDevice->getMainDepthStencil()->getSurface(0) );
            }
            else // 从池里面取，不过也有可能取到主深度
            {
                depthTmp.attach( m_renderBufferPool.getDepthBufferTemp( rttWidth, rttHeight ) );
                rtt->linkDepth( depthTmp );
            }

            // 渲染准备
            rtt->beginRender(pin->getOutRTTLevel());

            Viewport* vpMain = rtt->getViewport(0);
            vpMain->setRectByRtt(pin->getOutRTTLevel());
            vpMain->linkCamera( m_currCamera );

            uint32 clearFlagOld = vpMain->getClearFlag(); // 临时保存
            if ( !pin->isClearFlagEnabled() ) // 需要临时禁用
                vpMain->setClearFlag( 0 );

            vpMain->apply();
            vpMain->setClearFlag( clearFlagOld ); // 还原
        }

        // 设置为不需要深度，关闭混合
        g_renderDevice->setDepthStateSet( DepthStateSet::ALL_DISABLED );
        g_renderDevice->enableBlendState( pin->isBlendModeEnabled() );

        // 画全屏
        int effTempId = pin->getEffectTempId();
        Material* mtr = pin->getMaterial();

        FullScreenDSRenderable raObj;
        raObj.setMaterial( mtr );
        raObj.setData( pin );

        if ( pin->isWPOSRender() )
            raObj.setCamera( m_currCamera );
        else
            raObj.setCamera( 0 );

        EffectContext* effContext = g_effectTemplateManager->getEffectTemplate(effTempId)->getEffectContext( &raObj );
        g_renderDevice->setEffect( effContext->getEffect() );

        params.ra = &raObj;
        effContext->doSet( &params );
        raObj.render();

        // 渲染完毕
        if ( !pin->isMainRTTUsed() ) 
            pin->getOutput()->endRender();

        // must be reset zero for main camera
        m_currCamera->_setViewport(0);
    }

    void RenderSystem::_generateAOBuffer( EffSetterParams& params )
    {
        if ( !m_currSSAOEnabled )
            return;

        m_aoBuf = m_renderBufferPool.getRTTBuffer( PIXFMT_16F, 0, 0 );
        g_imageProcessManager->getProcessSSAO()->process( params );
    }

    void RenderSystem::_renderPostEffects( EffSetterParams& params )
    {
        TextureObj* outSceneBuff = m_sceneBuf;
        bool needSRGBWrite = false;

        bool isFinalFxAA = // 检查最终aa是否为fxaa
            m_currAntiAliasSetting ? m_currAntiAliasSetting->getPostAAMethod() == POSTAA_FXAA : false;

        //////////////////////////////////////////////////////////////////////////
        // taa
        if ( _isAATempEnabled() )
        {
            bool isPriorToFXAA = false; // 是否是fxaa的前一个步骤，如果是，我们需要输出luma

            if ( m_currHDRSetting ) // 后续有HDR等特效输出到16F格式且依然保持线性输出
            {
                outSceneBuff = _getRTTBuffer( m_sceneBuf->getFormat(), 0, 0 ); // 开一张新的16F属于浪费
                needSRGBWrite = false;
            }
            else if ( _isAAMainEnabled() ) // 后续直接postAA，那么应当输出到rgb8格式且srgb空间(smaa的要求)
            {
                _prepareBackBufferOnce(); // 需要一个backBuffer,分配一次
                outSceneBuff = m_backBuff;
                needSRGBWrite = true;

                isPriorToFXAA = isFinalFxAA;
            }
            else // 后续直接输出到系统缓存
            {
                outSceneBuff = 0;
                needSRGBWrite = true; // sRGB空间输出
            }

            g_imageProcessManager->getProcessPostAA()->processTempAA( params, 
                m_sceneBuf, m_sceneLastBuf, outSceneBuff, needSRGBWrite, isPriorToFXAA );
        }
        else // 无taa
        {
            // 保持原始不变，交给后续流程处理
        }

        //////////////////////////////////////////////////////////////////////////
        // hdr
        if ( m_currHDRSetting )
        {
            TextureObj* inBuffer = outSceneBuff; // 上一次结果的输出作为这次的输入
            bool isPriorToFXAA = false; 

            if ( _isAAMainEnabled() ) // 后续是postAA，分配back buffer给AA作为输入,也就是这次hdr的输出
            {
                _prepareBackBufferOnce();
                outSceneBuff = m_backBuff;
                isPriorToFXAA = isFinalFxAA;
            }
            else // 后续直接输出到系统缓存
            {
                outSceneBuff = 0;
            }

            needSRGBWrite = true; // 最终输出在srgb空间下
            g_imageProcessManager->getProcessHDR()->process( params, inBuffer, outSceneBuff, isPriorToFXAA );
        }
        else // 无hdr
        {
            // 后续流程总是srgb(无论输出给post aa或者是系统缓存)，
            // 当前上一步输出如果不是srgb，那么我们矫正
            if ( !needSRGBWrite )
            {
                needSRGBWrite = true;

                bool isPriorToFXAA = false; 

                // 将当前outSceneBuff转换到gamma space
                _beginGammaCorrect();
                g_imageProcessManager->getProcessCopy()->resetParams();
                g_imageProcessManager->getProcessCopy()->setInput( outSceneBuff );

                if ( _isAAMainEnabled() ) // 后续是postAA，分配back buffer作为输出
                {
                    _prepareBackBufferOnce();
                    outSceneBuff = m_backBuff;
                    isPriorToFXAA = isFinalFxAA;
                }
                else // 后续直接输出到系统缓存
                {
                    outSceneBuff = 0;
                }

                g_imageProcessManager->getProcessCopy()->setOutput( outSceneBuff );
                g_imageProcessManager->getProcessCopy()->writeLuma( isPriorToFXAA );
                g_imageProcessManager->getProcessCopy()->process( params );
                _endGammaCorrect();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // post aa
        if ( _isAAMainEnabled() )
        {
            // 无论如何，假如有post aa，那么输入一定是backbuff,格式rgba8，位于srgb空间
            khaosAssert( outSceneBuff == m_backBuff );
            khaosAssert( needSRGBWrite == true );

            if ( m_currAntiAliasSetting->getPostAAMethod() == POSTAA_SMAA ) // smaa
                g_imageProcessManager->getProcessPostAA()->processMainAA( params, outSceneBuff );
            else // fxaa
                g_imageProcessManager->getProcessPostAA()->processMainAA2( params, outSceneBuff );
        }
      
        //////////////////////////////////////////////////////////////////////////
        // post eff cmd
        if ( m_postEffCmd ) // 工具用...
        {
            m_postEffCmd->doCmd();
            m_postEffCmd.release();
        }
    }
}

