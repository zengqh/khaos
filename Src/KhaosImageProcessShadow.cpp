#include "KhaosPreHeaders.h"
#include "KhaosImageProcessShadow.h"
#include "KhaosRenderSystem.h"
#include "KhaosEffectID.h"
#include "KhaosMath.h"
#include "KhaosVector2.h"
#include "KhaosMaterialManager.h"
#include "KhaosRenderSystem.h"
#include "KhaosCamera.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    ImageProcessDownscaleZ::ImageProcessDownscaleZ()
    {
        m_pin = _createImagePin( ET_DOWNSCALEZ );
        m_pin->setOwnOutput();
        m_pin->setRttFormat( PIXFMT_INVALID ); // ���ⲿ���
        m_pin->setInputFilter( true ); // ��������
        _setRoot( m_pin );
    }

    void ImageProcessDownscaleZ::setInput( TextureObj* texIn )
    {
        m_pin->setInput( texIn );
        _updateParas();
    }

    void ImageProcessDownscaleZ::_updateParas()
    {
        // Set samples position
        float s1 = 0.5f / (float) m_pin->getInput()->getWidth(); 
        float t1 = 0.5f / (float) m_pin->getInput()->getHeight(); 

        // Use rotated grid
        m_texToTexParams0 = Vector4( s1,  t1, -s1,  t1); 
        m_texToTexParams1 = Vector4(-s1, -t1,  s1, -t1);
    }

    void ImageProcessDownscaleZ::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcessGeneralAO::ImageProcessGeneralAO()
    {
        m_pin = _createImagePin( ET_SSAO );
        m_pin->setOwnOutput();
        m_pin->setRttFormat( PIXFMT_INVALID ); // ���ⲿ���
        m_pin->setInputFilter( true ); // ��������
        m_pin->useMainDepth( true ); // Ҫ��ģ���޳��Ż�
        _setRoot( m_pin );
    }

    void ImageProcessGeneralAO::process( EffSetterParams& params )
    {
        // ���ⲿ����õ�
        m_pin->getOutput()->linkRTT( g_renderSystem->_getAoBuf() );

        // ���²���
        _checkSettings();

        _updateParams();

        // ����
        m_pinRoot->process( params );
    }

    void ImageProcessGeneralAO::_updateParams()
    {
        SSAORenderSetting* ssaoSetting = m_settings->getSetting<SSAORenderSetting>();

        // SSAO_VOParams
        Camera* cam = g_renderSystem->_getMainCamera();

        const Vector3* vCoords = cam->getCorners();
        const float fAspectRatio = 1.0f / cam->getAspect();
        const float fZFar = cam->getZFar();

        const float fDXView = (vCoords[7] - vCoords[6]).length(); // [Far-Right-Bottom] - [Far-Left-Bottom]
        const float fDYView = (vCoords[4] - vCoords[6]).length() * fAspectRatio; // [Far-Right-Top] - [Far-Left-Bottom]
       
        const float fRadius = ssaoSetting->getDiskRadius() * 0.03f; // CV_r_SSAO_radius = 1.5

        m_param1.x = fRadius;
        m_param1.y = fRadius * fAspectRatio;
        m_param1.z = fRadius * 0.707107f * (fDXView + fDYView) / Math::maxVal(1.0f, fZFar);
        m_param1.w = 0; // no used

        // SSAO_MultiRadiiParams
        m_param2.x = ssaoSetting->getSmallRadiusRatio();
        m_param2.y = ssaoSetting->getLargeRadiusRatio();
        m_param2.z = ssaoSetting->getBrighteningMargin();
        m_param2.w = 0;

        // SSAO_params
        const float fDetailRadius = 10.f / cam->getViewportHeight();
        const float fAOContrast = ssaoSetting->getContrast();

        m_param3.x = ssaoSetting->getAmount();
        m_param3.y = 1.f / fAOContrast;
        m_param3.z = fDetailRadius;
        m_param3.w = ssaoSetting->getDiskRadius();
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcessBlurAO::ImageProcessBlurAO() : m_pinBlurH(0), m_pinBlurV(0), m_dirty(true)
    {
        m_pinBlurH = _createImagePin( ET_SSAO_BLUR );
        m_pinBlurH->setInputFilter( false ); // ���Թ���
        m_pinBlurH->setOwnOutput(); // ʹ���ڲ�rtt������clear
        m_pinBlurH->setFlag((void*)0);

        m_pinBlurV = _createImagePin( ET_SSAO_BLUR );
        m_pinBlurV->setInputFilter( false ); // ���Թ���
        m_pinBlurV->setOwnOutput(); // ʹ���ڲ�rtt������clear
        m_pinBlurV->setRttFormat( PIXFMT_INVALID ); // �����ٴ���tex�ˣ�������ظ�ԭʼͼ
        m_pinBlurV->setFlag((void*)1);

        m_pinBlurH->useMainDepth( true ); // ģ���Ż�
        m_pinBlurV->useMainDepth( true );

        // ���ӣ�blurH -> blurV
        m_pinBlurV->addChild( m_pinBlurH );
        _setRoot( m_pinBlurV );
    }

    void ImageProcessBlurAO::setInput( TextureObj* texIn )
    {
        m_pinBlurH->setInput( texIn );
        m_pinBlurH->setRequestRttSize( IntVector2(texIn->getWidth(), texIn->getHeight()) );
        m_pinBlurH->setRttFormat( texIn->getFormat() );

        m_pinBlurV->getOutput()->linkRTT( texIn );

        m_dirty = true;
    }

    void ImageProcessBlurAO::_checkSettings()
    {
        if ( !m_dirty )
            return;

        m_dirty = false;

        const int nSrcSizeX = m_pinBlurH->getInput()->getWidth();
        const int nSrcSizeY = m_pinBlurH->getInput()->getHeight();
        const int nDstSizeX = nSrcSizeX;
        const int nDstSizeY = nSrcSizeY;

        ////////////////////
        // BlurOffset
        m_param1[0] = 0.5f / (float)nDstSizeX;
        m_param1[1] = 0.5f / (float)nDstSizeY;
        m_param1[2] = 1.f / (float)nSrcSizeX;
        m_param1[3] = 1.f / (float)nSrcSizeY;

        //////////////////
        // SSAO_BlurKernel
        m_param2H[0] = 2.f / nSrcSizeX;
        m_param2H[1] = 0;
        m_param2H[2] = 0;
        m_param2H[3] = 35.f; // Weight coef

        m_param2V[0] = 0;
        m_param2V[1] = 2.f / nSrcSizeY;
        m_param2V[2] = 0;
        m_param2V[3] = 35.f; // Weight coef
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcessSSAO::ImageProcessSSAO()
    {
    }

    ImageProcessSSAO::~ImageProcessSSAO()
    {
    }

    void ImageProcessSSAO::process( EffSetterParams& params )
    {
        // downscale depth
        TextureObj* depthTarget = g_renderSystem->_getGBuf().getDepthBuffer();
        TextureObj* depthTargetHalf = g_renderSystem->_getDepthHalf();
        TextureObj* depthTargetQuarter = g_renderSystem->_getDepthQuarter();

        m_downScaleZHalf.setInput( depthTarget );
        m_downScaleZHalf.setOutput( depthTargetHalf );
        m_downScaleZHalf.process( params );

        m_downScaleZQuarter.setInput( depthTargetHalf );
        m_downScaleZQuarter.setOutput( depthTargetQuarter );
        m_downScaleZQuarter.process( params );

        ////////////////////////////////////
        g_renderSystem->_setDrawOnly( RenderSystem::RK_OPACITY ); // �Ż���

        // general ao
        m_generalAO.process( params );

        // blur ao
        m_blurAO.setInput( g_renderSystem->_getAoBuf() );
        m_blurAO.process( params );

        g_renderSystem->_unsetDrawOnly();
    }

    //////////////////////////////////////////////////////////////////////////
    ImgProcShadowMask::ImgProcShadowMask()
    {
        m_pin = _createImagePin( ET_SHADOWMASK );
        m_pin->setOwnOutput( /*RCF_TARGET, &Color::WHITE*/ ); // �������rt,�������ɫ�����
        //m_pin->enableClearFlag( true );
        m_pin->setRttFormat( PIXFMT_INVALID ); // ���ⲿ�ṩ�����Լ�������
        m_pin->useMainDepth( true ); // ������Ҫģ���Ż�
        m_pin->useWPOSRender( true );
        _setRoot( m_pin );
    }

    void ImgProcShadowMask::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
    }
}

