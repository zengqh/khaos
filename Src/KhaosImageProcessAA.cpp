#include "KhaosPreHeaders.h"
#include "KhaosImageProcessAA.h"
#include "KhaosImageProcessUtil.h"

#include "KhaosRenderSystem.h"
#include "KhaosCamera.h"
#include "KhaosTextureObj.h"
#include "KhaosMaterialManager.h"
#include "KhaosTimer.h"

#include "KhaosEffectID.h"
#include "KhaosMath.h"
#include "KhaosVector2.h"

#include "KhaosRenderDevice.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    ImgProcAAEdgeDetection::ImgProcAAEdgeDetection()
    {
        m_pin = _createImagePin( ET_POSTAA_EDGE_DETECTION );
        m_pin->setOwnOutputEx( RCF_TARGET | RCF_STENCIL, Color::ZERO, 1.0f, 0 ); // ��Ҫ����ģ���Ǳ�Ե
        m_pin->setInputFilterEx( TextureFilterSet::TRILINEAR ); // Ϊ��������?��demo���������ðɡ�
        _setRoot( m_pin );
    }

    void ImgProcAAEdgeDetection::setColorBuffer( TextureObj* texClrGamma )
    {
        // �������gamma���������ɫ����Ŷ������Ҫ������Ĵ��ɷ���demo����˵�ģ�
        m_pin->setInput( texClrGamma );
    }

    void ImgProcAAEdgeDetection::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
    }

    void ImgProcAAEdgeDetection::process( EffSetterParams& params )
    {
        // ����ģ���ǣ�ֻҪû��discard������ô���Ǳ��1����ʾ��Ե
        g_renderDevice->enableStencil( true );

        g_renderDevice->setStencilStateSet( 
            StencilStateSet(1, CMP_ALWAYS, -1, -1, 
            STENCILOP_KEEP, STENCILOP_KEEP, STENCILOP_REPLACE) );

        ImageProcess::process( params );

        //g_renderDevice->enableStencil( false ); // next pass is blend, also need stencil
    }

    //////////////////////////////////////////////////////////////////////////
    ImgProcAABlendingWeightsCalculation::ImgProcAABlendingWeightsCalculation()
    {
        m_pin = _createImagePin( ET_POSTAA_BLENDING_WEIGHT_CALC );
        m_pin->setOwnOutputEx( RCF_TARGET, Color::ZERO ); 
        m_pin->setInputFilterEx( TextureFilterSet::TRILINEAR ); 
        _setRoot( m_pin );
    }

    void ImgProcAABlendingWeightsCalculation::setEdgesBuffer( TextureObj* texEdges )
    {
        m_pin->setInput( texEdges );
    }

    void ImgProcAABlendingWeightsCalculation::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
    }
   
    void ImgProcAABlendingWeightsCalculation::process( EffSetterParams& params )
    {
        // ����ģ���ǣ�ֻ������1������
        //g_renderDevice->enableStencil( true );

        g_renderDevice->setStencilStateSet( 
            StencilStateSet(1, CMP_EQUAL, -1, -1, 
            STENCILOP_KEEP, STENCILOP_KEEP, STENCILOP_KEEP) );

        ImageProcess::process( params );

        g_renderDevice->enableStencil( false );
    }

    //////////////////////////////////////////////////////////////////////////
    ImgProcAAFinal::ImgProcAAFinal()
    {
        m_pin = _createImagePin( ET_POSTAA_FINAL );
        m_pin->useMainRTT( true ); // ʹ�������rtt
        _setRoot( m_pin );
    }

    void ImgProcAAFinal::process( EffSetterParams& params )
    {
        g_renderSystem->_beginGammaCorrect();
        ImageProcess::process( params );
        g_renderSystem->_endGammaCorrect();
    }

    //////////////////////////////////////////////////////////////////////////
    ImgProcAAFinal2::ImgProcAAFinal2()
    {
        m_pin = _createImagePin( ET_POSTAA_FINAL2 );
        m_pin->useMainRTT( true ); // ʹ�������rtt
        _setRoot( m_pin );
    }

    void ImgProcAAFinal2::process( EffSetterParams& params )
    {
        ImageProcess::process( params );
    }

    //////////////////////////////////////////////////////////////////////////
    ImgProcAATemporal::ImgProcAATemporal() : m_inSceneBuff(0), m_inSceneLastBuff(0), m_isWriteLuma(false)
    {
        m_pin = _createImagePin( ET_POSTAA_TEMP );
        m_pin->useMainRTT( true ); // ʹ�������rtt
        m_pin->useWPOSRender( true );

        m_pinTemp = _createImagePin( ET_POSTAA_TEMP );
        m_pinTemp->setOwnOutput(); // ������м�buffer
        m_pinTemp->useWPOSRender( true );
    }

    ImgProcAATemporal::~ImgProcAATemporal()
    {
        _setRoot(0); // delete at here, not in base class!

        KHAOS_DELETE m_pin;
        KHAOS_DELETE m_pinTemp;
    }

    void ImgProcAATemporal::process( EffSetterParams& params, TextureObj* inSceneBuff, 
        TextureObj* inSceneLastBuff, TextureObj* outSceneBuff, bool needSRGB, bool isPriorToFXAA )
    {
        m_inSceneBuff = inSceneBuff;
        m_inSceneLastBuff = inSceneLastBuff;
        m_isWriteLuma = isPriorToFXAA;

        if ( outSceneBuff ) // �������ʱ
        {
            m_pinTemp->getOutput()->linkRTT( outSceneBuff );
            _setRoot( m_pinTemp );
        }
        else // ֱ�����
        {
            _setRoot( m_pin );
        }

        if ( needSRGB )
            g_renderSystem->_beginGammaCorrect();

        ImageProcess::process( params );
        
        if ( needSRGB )
            g_renderSystem->_endGammaCorrect();
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcessPostAA::ImageProcessPostAA() : m_colorBuf(0), m_edgesAABuf(0), m_blendAABuf(0), m_inited(false)
    {
    }

    ImageProcessPostAA::~ImageProcessPostAA()
    {
    }

    void ImageProcessPostAA::applyJitter( Camera* cam )
    {
        if ( !g_renderSystem->_isAATempEnabled() )
        {
            m_inited = false;
            return;
        }

        // camera jitter
        cam->applyJitter( _getJitter(cam) );

        // ����reproject matrix
        Matrix4 matProjToTex;
        g_renderDevice->makeMatrixProjToTex( matProjToTex, 0, 
            cam->getViewportWidth(), cam->getViewportHeight(), true );

        if ( m_inited )
        {
            Matrix4 transJitter;
            Vector2 jitter = _getJitter(cam);
            transJitter.makeTrans( jitter.x, jitter.y, 0 ); // ��Ȼ����һ֡��viewproj�������������ñ��ε�

            m_matLastProj = transJitter * m_matLastProj;
            g_renderDevice->toDeviceProjMatrix( m_matLastProj );
            m_matViewProjPre = matProjToTex * m_matLastProj * m_matLastView;
        }
        else
        {
            m_matViewProjPre = matProjToTex * g_renderSystem->_getMainCamera()->getViewProjMatrixRD();
            //m_inited = true;
        }

        // ��ǰ��Ϊ��һ֡ no jitter here
        m_matLastView = g_renderSystem->_getMainCamera()->getViewMatrixRD();
        m_matLastProj = g_renderSystem->_getMainCamera()->getProjMatrix();
    }

    const float* ImageProcessPostAA::getSubIdx() const
    {
        // ����û��taa����
        //if ( !g_renderSystem->_isAATempEnabled() )
        {
            static const float zeros[4] = {};
            return zeros;
        }

        static const float indices[][4] = 
        {
            { 1, 1, 1, 0 }, // S0
            { 2, 2, 2, 0 }  // S1
            // (it's 1 for the horizontal slot of S0 because horizontal
            //  blending is reversed: positive numbers point to the right)
        };

        
        return indices[_getCurrID()];
    }

    int ImageProcessPostAA::_getCurrID() const
    {
        return (g_renderSystem->_getCurrFrame()) % 2;
    }

    Vector2 ImageProcessPostAA::_getJitter( Camera* cam )
    {
        static const Vector2 vSSAA2x[2] =
        {
            Vector2(-0.25f,  0.25f),
            Vector2( 0.25f, -0.25f)
        };

        Vector2 jitter = vSSAA2x[_getCurrID()];

        jitter.x = jitter.x * 2.0f / cam->getViewportWidth(); // project���ƶ�2dΪʵ���ƶ�d
        jitter.y = jitter.y * 2.0f / cam->getViewportHeight();

        return jitter;
    }

    Vector2 ImageProcessPostAA::getJitterUV( Camera* cam )
    {
        return _getJitter(cam) * Vector2(0.5f, -0.5f);
    }

    Vector3 ImageProcessPostAA::getJitterWorld( Camera* cam )
    {
        const Vector3* corners = cam->getCorners();

        Vector3 xdir = corners[7] - corners[6];
        Vector3 ydir = corners[4] - corners[7]; // far

        Vector2 jitter = _getJitter(cam) * 0.5f;

        return (xdir * jitter.x + ydir * jitter.y); // jitter @ far plane
    }

    TextureObj* ImageProcessPostAA::getFrame( bool curr ) const
    {
        if ( m_inited )
        {
            return curr ? m_temporal.getCurrSceneBuff() : m_temporal.getLastSceneBuff();
        }

        return m_temporal.getCurrSceneBuff();
    }

    void ImageProcessPostAA::processTempAA( EffSetterParams& params, TextureObj* inSceneBuff, 
        TextureObj* inSceneLastBuff, TextureObj* outSceneBuff, bool needSRGB, bool isPriorToFXAA )
    {
        // pass3: temporal
        khaosAssert ( g_renderSystem->_isAATempEnabled() );

        m_temporal.process( params, inSceneBuff, inSceneLastBuff, outSceneBuff, needSRGB, isPriorToFXAA );
        m_inited = true;
    }

    void ImageProcessPostAA::processMainAA( EffSetterParams& params, TextureObj* inSceneBuff )
    {
        m_colorBuf = inSceneBuff;

        // pass0: ��Ǳ�Ե
        RenderBufferPool::ItemTemp tmpEdgeAABuff = g_renderSystem->_getRTTBufferTemp( PIXFMT_A8R8G8B8, 0, 0 );
        m_edgesAABuf = tmpEdgeAABuff->rtt;
        
        m_passEdgeDetection.setColorBuffer( inSceneBuff );
        m_passEdgeDetection.setOutput( m_edgesAABuf );
        m_passEdgeDetection.process( params );

        // pass1: ���Ȩ�ؼ���
        RenderBufferPool::ItemTemp tmpBlendAABuff = g_renderSystem->_getRTTBufferTemp( PIXFMT_A8R8G8B8, 0, 0 );
        m_blendAABuf = tmpBlendAABuff->rtt;

        m_passBlendingWeights.setEdgesBuffer( m_edgesAABuf );
        m_passBlendingWeights.setOutput( m_blendAABuf );
        m_passBlendingWeights.process( params );

        tmpEdgeAABuff.clear(); // ������ҪedgeAA��

        // pass2: ���ս��
        m_final.process( params );
    }

    void ImageProcessPostAA::processMainAA2( EffSetterParams& params, TextureObj* inSceneBuff )
    {
        m_colorBuf = inSceneBuff;
        m_final2.process( params );
    }
}

