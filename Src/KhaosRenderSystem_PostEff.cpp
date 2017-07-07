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
        // ����
        m_currCamera = m_mainCamera; // Ŀǰ��ʹ��main camera
        m_renderPassStage = MP_ETC;
        params.clearAndStampGlobal();

        if ( pin->isMainRTTUsed() ) // ʹ�õ�ǰ��camera��rtt����
        {
            m_currCamera->_setViewport(0);
            g_renderDevice->restoreMainRTT(); // only main window current
        }
        else // ʹ��pin������
        {
            // ������ʱtex��Դ
            pin->requestRttRes();

            RenderTarget* rtt = pin->getOutput();
            int rttWidth = rtt->getWidth(pin->getOutRTTLevel());
            int rttHeight = rtt->getHeight(pin->getOutRTTLevel());

            // ����ʱdepth
            RenderBufferPool::ItemTemp depthTmp;

            if ( pin->isMainDepthUsed() ) // ʹ�������
            {
                khaosAssert(rttWidth == g_renderDevice->getMainDepthStencil()->getWidth() );
                khaosAssert(rttHeight == g_renderDevice->getMainDepthStencil()->getHeight() );
                rtt->linkDepth( g_renderDevice->getMainDepthStencil()->getSurface(0) );
            }
            else // �ӳ�����ȡ������Ҳ�п���ȡ�������
            {
                depthTmp.attach( m_renderBufferPool.getDepthBufferTemp( rttWidth, rttHeight ) );
                rtt->linkDepth( depthTmp );
            }

            // ��Ⱦ׼��
            rtt->beginRender(pin->getOutRTTLevel());

            Viewport* vpMain = rtt->getViewport(0);
            vpMain->setRectByRtt(pin->getOutRTTLevel());
            vpMain->linkCamera( m_currCamera );

            uint32 clearFlagOld = vpMain->getClearFlag(); // ��ʱ����
            if ( !pin->isClearFlagEnabled() ) // ��Ҫ��ʱ����
                vpMain->setClearFlag( 0 );

            vpMain->apply();
            vpMain->setClearFlag( clearFlagOld ); // ��ԭ
        }

        // ����Ϊ����Ҫ��ȣ��رջ��
        g_renderDevice->setDepthStateSet( DepthStateSet::ALL_DISABLED );
        g_renderDevice->enableBlendState( pin->isBlendModeEnabled() );

        // ��ȫ��
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

        // ��Ⱦ���
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

        bool isFinalFxAA = // �������aa�Ƿ�Ϊfxaa
            m_currAntiAliasSetting ? m_currAntiAliasSetting->getPostAAMethod() == POSTAA_FXAA : false;

        //////////////////////////////////////////////////////////////////////////
        // taa
        if ( _isAATempEnabled() )
        {
            bool isPriorToFXAA = false; // �Ƿ���fxaa��ǰһ�����裬����ǣ�������Ҫ���luma

            if ( m_currHDRSetting ) // ������HDR����Ч�����16F��ʽ����Ȼ�����������
            {
                outSceneBuff = _getRTTBuffer( m_sceneBuf->getFormat(), 0, 0 ); // ��һ���µ�16F�����˷�
                needSRGBWrite = false;
            }
            else if ( _isAAMainEnabled() ) // ����ֱ��postAA����ôӦ�������rgb8��ʽ��srgb�ռ�(smaa��Ҫ��)
            {
                _prepareBackBufferOnce(); // ��Ҫһ��backBuffer,����һ��
                outSceneBuff = m_backBuff;
                needSRGBWrite = true;

                isPriorToFXAA = isFinalFxAA;
            }
            else // ����ֱ�������ϵͳ����
            {
                outSceneBuff = 0;
                needSRGBWrite = true; // sRGB�ռ����
            }

            g_imageProcessManager->getProcessPostAA()->processTempAA( params, 
                m_sceneBuf, m_sceneLastBuf, outSceneBuff, needSRGBWrite, isPriorToFXAA );
        }
        else // ��taa
        {
            // ����ԭʼ���䣬�����������̴���
        }

        //////////////////////////////////////////////////////////////////////////
        // hdr
        if ( m_currHDRSetting )
        {
            TextureObj* inBuffer = outSceneBuff; // ��һ�ν���������Ϊ��ε�����
            bool isPriorToFXAA = false; 

            if ( _isAAMainEnabled() ) // ������postAA������back buffer��AA��Ϊ����,Ҳ�������hdr�����
            {
                _prepareBackBufferOnce();
                outSceneBuff = m_backBuff;
                isPriorToFXAA = isFinalFxAA;
            }
            else // ����ֱ�������ϵͳ����
            {
                outSceneBuff = 0;
            }

            needSRGBWrite = true; // ���������srgb�ռ���
            g_imageProcessManager->getProcessHDR()->process( params, inBuffer, outSceneBuff, isPriorToFXAA );
        }
        else // ��hdr
        {
            // ������������srgb(���������post aa������ϵͳ����)��
            // ��ǰ��һ������������srgb����ô���ǽ���
            if ( !needSRGBWrite )
            {
                needSRGBWrite = true;

                bool isPriorToFXAA = false; 

                // ����ǰoutSceneBuffת����gamma space
                _beginGammaCorrect();
                g_imageProcessManager->getProcessCopy()->resetParams();
                g_imageProcessManager->getProcessCopy()->setInput( outSceneBuff );

                if ( _isAAMainEnabled() ) // ������postAA������back buffer��Ϊ���
                {
                    _prepareBackBufferOnce();
                    outSceneBuff = m_backBuff;
                    isPriorToFXAA = isFinalFxAA;
                }
                else // ����ֱ�������ϵͳ����
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
            // ������Σ�������post aa����ô����һ����backbuff,��ʽrgba8��λ��srgb�ռ�
            khaosAssert( outSceneBuff == m_backBuff );
            khaosAssert( needSRGBWrite == true );

            if ( m_currAntiAliasSetting->getPostAAMethod() == POSTAA_SMAA ) // smaa
                g_imageProcessManager->getProcessPostAA()->processMainAA( params, outSceneBuff );
            else // fxaa
                g_imageProcessManager->getProcessPostAA()->processMainAA2( params, outSceneBuff );
        }
      
        //////////////////////////////////////////////////////////////////////////
        // post eff cmd
        if ( m_postEffCmd ) // ������...
        {
            m_postEffCmd->doCmd();
            m_postEffCmd.release();
        }
    }
}

