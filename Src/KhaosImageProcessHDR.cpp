#include "KhaosPreHeaders.h"
#include "KhaosImageProcessHDR.h"
#include "KhaosImageProcessUtil.h"

#include "KhaosRenderSystem.h"
#include "KhaosCamera.h"
#include "KhaosTextureObj.h"
#include "KhaosMaterialManager.h"
#include "KhaosTimer.h"

#include "KhaosEffectID.h"
#include "KhaosMath.h"
#include "KhaosVector2.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    ImageProcessInitLumin::ImageProcessInitLumin()
    {
        m_pin = _createImagePin( ET_HDR_INITLUMIN );
        m_pin->setOwnOutput();
        m_pin->setRttFormat( PIXFMT_INVALID ); // ���ⲿ���
        m_pin->setInputFilter( false ); // �������Բ���
        _setRoot( m_pin );
    }

    void ImageProcessInitLumin::setInput( TextureObj* texIn )
    {
        m_pin->setInput( texIn );
        _updateParas();
    }

    void ImageProcessInitLumin::_updateParas()
    {
        float s1 = 1.0f / (float) m_pin->getInput()->getWidth();
        float t1 = 1.0f / (float) m_pin->getInput()->getHeight();     

        // Use rotated grid
        m_sampleLumOffsets0 = Vector4(s1*0.95f, t1*0.25f, -s1*0.25f, t1*0.96f); 
        m_sampleLumOffsets1 = Vector4(-s1*0.96f, -t1*0.25f, s1*0.25f, -t1*0.96f);  
    }

    void ImageProcessInitLumin::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcessLuminIter::ImageProcessLuminIter()
    {
        m_pin = _createImagePin( ET_HDR_LUMINITER );
        m_pin->setOwnOutput();
        m_pin->setRttFormat( PIXFMT_INVALID ); // ���ⲿ���
        m_pin->setInputFilter( false ); // �������Բ���
        _setRoot( m_pin );
    }

    void ImageProcessLuminIter::setInput( TextureObj* texIn )
    {
        m_pin->setInput( texIn );
        _updateParas();
    }

    void ImageProcessLuminIter::_updateParas()
    {
        float tU = 1.0f / (float) m_pin->getInput()->getWidth();
        float tV = 1.0f / (float) m_pin->getInput()->getHeight();

        // ��֧�����Բ���float���������£����������ɣ�ȡ4�㼴�ɣ��Ӷ���16x16ȡ����1x1
        int index = 0;

        for ( int y = 0; y < 4; y += 2 )
        {
            for ( int x = 0; x < 4; x += 2, ++index )
            {
                m_sampleLumOffsets[index].x = (x - 1.f) * tU;
                m_sampleLumOffsets[index].y = (y - 1.f) * tV;
                m_sampleLumOffsets[index].z = 0;
                m_sampleLumOffsets[index].w = 1;
            }
        }
    }

    void ImageProcessLuminIter::setLastIter( bool isLast )
    {
        // MtrCommState::BIT0����Ƿ����ε���
        if ( !m_material )
        {
            m_material.attach( MaterialManager::createMaterial("$LuminIterMtr") );
            m_material->setNullCreator();
            m_pin->setMaterial( m_material.get() );
        }

        m_material->enableCommState( MtrCommState::BIT0, isLast );
    }

    void ImageProcessLuminIter::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcessAdaptedLum::ImageProcessAdaptedLum() :
        m_texPrevLum(0), m_texCurToneMap(0)
    {
        m_pin = _createImagePin( ET_HDR_ADAPTEDLUM );
        m_pin->setOwnOutput();
        m_pin->setRttFormat( PIXFMT_INVALID ); // ���ⲿ���
        m_pin->setInputFilter( true ); // ��������
        _setRoot( m_pin );
    }

    void ImageProcessAdaptedLum::setPrevLum( TextureObj* texIn )
    {
        m_texPrevLum = texIn;
    }

    void ImageProcessAdaptedLum::setCurToneMap( TextureObj* texIn )
    {
        m_texCurToneMap = texIn;
    }

    void ImageProcessAdaptedLum::update()
    {
        // ��������
        float elapsedTime = g_timerSystem.getElapsedTime();

        m_param[0] = elapsedTime;
        m_param[1] = 1 - Math::pow(0.98f, g_renderSystem->_getCurrHDRSetting()->getEyeAdaptationSpeed() * 33.0f * elapsedTime);
        m_param[2] = 0;
        m_param[3] = 0;
    }

    void ImageProcessAdaptedLum::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcessBrightPass::ImageProcessBrightPass() :
        m_pin(0), m_texSrc(0), m_texCurLum(0)
    {
        m_pin = _createImagePin( ET_HDR_BRIGHTPASS );
        m_pin->setOwnOutput();
        m_pin->setRttFormat( PIXFMT_INVALID ); // ���ⲿ���
        m_pin->setInputFilter( true ); // ��������
        _setRoot( m_pin );
    }

    void ImageProcessBrightPass::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcessFlaresPass::ImageProcessFlaresPass()
    {
        m_pin = _createImagePin( ET_HDR_FLARESPASS );
        m_pin->setOwnOutput();
        m_pin->setRttFormat( PIXFMT_INVALID ); // ���ⲿ���
        m_pin->setInputFilter( true ); // ��������
        _setRoot( m_pin );

        KHAOS_CLEAR_ARRAY(m_texBlooms);
    }

    void ImageProcessFlaresPass::setBlooms( TextureObj* texIn[3] )
    {
        m_texBlooms[0] = texIn[0];
        m_texBlooms[1] = texIn[1];
        m_texBlooms[2] = texIn[2];
    }

    void ImageProcessFlaresPass::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcessFinalScene::ImageProcessFinalScene()
    {
        // ֱ�������ϵͳ����
        m_pinToMainBuf = _createImagePin( ET_HDR_FINALPASS );
        m_pinToMainBuf->setInputFilter( true ); // ��������
        m_pinToMainBuf->useMainRTT( true ); // ʹ�������rtt
        m_pinToMainBuf->useWPOSRender( true );

        // �������ʱ����
        m_pinToTmpBuf = _createImagePin( ET_HDR_FINALPASS );
        m_pinToTmpBuf->setOwnOutput();
        m_pinToTmpBuf->setInputFilter( true ); // ��������
        m_pinToTmpBuf->useWPOSRender( true );

        //_setRoot( m_pin );
    }

    ImageProcessFinalScene::~ImageProcessFinalScene()
    {
        _setRoot(0); // delete at here, not in base class!

        KHAOS_DELETE m_pinToMainBuf;
        KHAOS_DELETE m_pinToTmpBuf;
    }

    float ImageProcessFinalScene::getGammaAdjVal() const
    {
        GammaSetting* gs = g_renderSystem->_getCurrGammaSetting();
        float curGamma = gs ? gs->getGammaValue() : 1.0f; 
        return 2.2f / curGamma;
    }

    void ImageProcessFinalScene::_checkSettings()
    {
        ImageProcess::_checkSettings();

        // ���gamma�Ƿ�2.2�����ǵĻ���hdr����������hdr������Ѿ�����gamma����2.2��
        // hdr_x = pow(x, 1/2.2)
        // output = pow( hdr_x, 2.2 / gamma )

        if ( !m_material )
        {
            m_material.attach( MaterialManager::createMaterial("$HDRFinalGamma") );
            m_material->setNullCreator();
            m_pinToMainBuf->setMaterial( m_material.get() );
            m_pinToTmpBuf->setMaterial( m_material.get() );
        }

        float adjVal = getGammaAdjVal();
        bool needAdjGamma = !Math::realEqual( adjVal, 1.0f, 0.01f ); // �����1.0һ�����ǾͲ��õ�����
        m_material->enableCommState( MtrCommState::BIT0, needAdjGamma );

        //bool useTempAA = g_renderSystem->_isAATempEnabled();
        //m_material->enableCommState( MtrCommState::BIT1, useTempAA );
        m_material->enableCommState( MtrCommState::BIT1, false ); // hdr�ڵ�tempaa��Զû����

        // ������
        TextureObj* outBuf = g_imageProcessManager->getProcessHDR()->_getOutBuf();
        if ( outBuf ) // ����ʱ���
        {
            m_pinToTmpBuf->getOutput()->linkRTT( outBuf );
            _setRoot( m_pinToTmpBuf );
        }
        else // ֱ�������ϵͳ����
        {
            _setRoot( m_pinToMainBuf );
        }
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcessHDR::ImageProcessHDR() :
        m_inBuffer(0), m_outBuffer(0),
        m_texTargetHalf(0), m_texTargetQuarter(0), m_texTargetQuarterPost(0),
        m_texTargetEighth(0), m_texTargetSixteenth(0), m_texFinalBloomQuarter(0),
        m_curLumTexture(0), m_curLumTextureTick(0),
        m_isWriteLuma(false)
    {
        // ԭͼ����Сͼ
        m_scaleTargetHalf.setBigDownsample( false );
        m_scaleTargetQuarter.setBigDownsample( false );

        // ͳ��lumin��ͼ 1,4,16,64
        KHAOS_CLEAR_ARRAY( m_texToneMaps );

        // ���㵱ǰ��������lumin��1x1��С
        KHAOS_CLEAR_ARRAY( m_texAdaptedLuminCur );

        m_scaleBloom_8.setBigDownsample( false );
        m_scaleBloom_16.setBigDownsample( false );

        m_brightpass.setUserData( this );
        m_finalScene.setUserData( this );
    }

    ImageProcessHDR::~ImageProcessHDR()
    {
        _cleanToneMaps();
        _cleanAdaptedLuminMaps();
    }

    void ImageProcessHDR::process( EffSetterParams& params, TextureObj* inBuffer, TextureObj* outBuffer, bool isPriorToFXAA )
    {
        m_inBuffer = inBuffer;
        m_outBuffer = outBuffer;
        m_isWriteLuma = isPriorToFXAA;

        // ���²���
        _updateParams();

        // ����������Ŀ������ͼ
        _downsampleHDRTarget( params );

        // ��ⳡ�����ȣ�����������
        int adaptCache = Math::maxVal(1, g_renderSystem->_getCurrHDRSetting()->getEyeAdaptionCache());

        if ( g_renderSystem->_getEffectFrame() % adaptCache == 0 )
        {  
            _measureLuminance( params );
            _eyeAdaptation( params );
        }

        // bloom��˸
        _bloomAndFlares( params );

        // ������Ⱦ
        m_finalScene.process(params);
    }

    void ImageProcessHDR::_downsampleHDRTarget( EffSetterParams& params )
    {
        // ������Դ
        Camera* cam = g_renderSystem->_getMainCamera(); // ����Ӧ�������������
        int vpWidth = cam->getViewportWidth();
        int vpHeight = cam->getViewportHeight();

        const PixelFormat targetFmt = PIXFMT_A16B16G16R16F;

        m_texTargetHalf = g_renderSystem->_getRTTBuffer( targetFmt, vpWidth/2, vpHeight/2 );
        m_texTargetQuarter = g_renderSystem->_getRTTBuffer( targetFmt, vpWidth/4, vpHeight/4 );
        m_texTargetQuarterPost = g_renderSystem->_getRTTBuffer( targetFmt, vpWidth/4, vpHeight/4 );
        m_texTargetEighth = g_renderSystem->_getRTTBuffer( targetFmt, vpWidth/8, vpHeight/8 );
        m_texTargetSixteenth = g_renderSystem->_getRTTBuffer( targetFmt, vpWidth/16, vpHeight/16 );

        m_texFinalBloomQuarter = g_renderSystem->_getRTTBuffer( /*targetFmt*/PIXFMT_A8R8G8B8, vpWidth/4, vpHeight/4 );

        // ��С��һ��
        m_scaleTargetHalf.setInput( m_inBuffer );
        m_scaleTargetHalf.setOutput( m_texTargetHalf );
        m_scaleTargetHalf.process( params );

        // ��С���ķ�֮һ
        m_scaleTargetQuarter.setInput( m_texTargetHalf );
        m_scaleTargetQuarter.setOutput( m_texTargetQuarter );
        m_scaleTargetQuarter.process( params );
    }

    void ImageProcessHDR::_buildToneMaps()
    {
        if ( m_texToneMaps[0] )
            return;

        const PixelFormat fmt = PIXFMT_A16B16G16R16F; // PIXFMT_A16B16G16R16F PIXFMT_16F

        // ������Щͼ���ܱ�������֡���������ڣ�����ֱ�Ӵ�������
        // ͳ��lumin��ͼ 1,4,16,64
        for ( int i = 0; i < NUM_TONEMAP_TEXTURES; ++i )
        {
            int iSampleLen = 1 << (2 * i);
            m_texToneMaps[i] = g_renderSystem->_createRTTBuffer( fmt, iSampleLen, iSampleLen );
        }
    }

    void ImageProcessHDR::_cleanToneMaps()
    {
        if ( !m_texToneMaps[0] )
            return;

        for ( int i = 0; i < NUM_TONEMAP_TEXTURES; ++i )
        {
            KHAOS_DELETE m_texToneMaps[i];
            m_texToneMaps[i] = 0;
        }
    }

    void ImageProcessHDR::_measureLuminance( EffSetterParams& params )
    {
        // ����tone map��Դ
        _buildToneMaps();

        int curTexture = NUM_TONEMAP_TEXTURES-1;

        // 1/4ԭͼ��Ϊ���룬���Բ�����ͳ�Ƴ�ʼ����64x64 tonemap
        m_initLumin.setInput( m_texTargetQuarter );
        m_initLumin.setOutput( m_texToneMaps[curTexture] );
        m_initLumin.process( params );
        
        // ��64x64����������1x1 
        --curTexture;

        while ( curTexture >= 0 )
        {
            m_luminIter.setInput( m_texToneMaps[curTexture+1] );
            m_luminIter.setOutput( m_texToneMaps[curTexture] );

            bool isLastIter = ( curTexture == 0 ); // ���һ�Σ�����Ҫexp��ԭ
            m_luminIter.setLastIter( isLastIter );

            m_luminIter.process( params );

            --curTexture;
        }
    }

    void ImageProcessHDR::_buildAdaptedLuminMaps()
    {
        if ( m_texAdaptedLuminCur[0] )
            return;

        const PixelFormat fmt = PIXFMT_A16B16G16R16F; // PIXFMT_A16B16G16R16F PIXFMT_16F

        // ������Щͼ���ܱ�������֡���������ڣ�����ֱ�Ӵ�������
        for ( int i = 0; i < NUM_CURRLUMIN_TEXTURES; ++i )
        {
            m_texAdaptedLuminCur[i] = g_renderSystem->_createRTTBuffer( fmt, 1, 1 );
        }
    }

    void ImageProcessHDR::_cleanAdaptedLuminMaps()
    {
        if ( !m_texAdaptedLuminCur[0] )
            return;

        for ( int i = 0; i < NUM_CURRLUMIN_TEXTURES; ++i )
        {
            KHAOS_DELETE m_texAdaptedLuminCur[i];
            m_texAdaptedLuminCur[i] = 0;
        }
    }

    void ImageProcessHDR::_eyeAdaptation( EffSetterParams& params )
    {
        _buildAdaptedLuminMaps();

        // ��ǰ��lumͼ����
        int curLumIdx = m_curLumTextureTick;
        ++m_curLumTextureTick;
        curLumIdx &= MASK_CURRLUMIN_MAX;

        // ��һ��lumͼ����
        int lastLumIdx = (curLumIdx - 1) & MASK_CURRLUMIN_MAX;

        // �õ���һ��ͼ�ͱ���Ҫ�����ͼ
        // texCurr�Ľ������texPrev��m_texToneMaps[0]���ֵ
        TextureObj* texPrev = m_texAdaptedLuminCur[lastLumIdx];
        TextureObj* texCurr = m_texAdaptedLuminCur[curLumIdx];

        m_curLumTexture = texCurr;

        m_adaptedLum.setPrevLum( texPrev );
        m_adaptedLum.setCurToneMap( m_texToneMaps[0] );
        m_adaptedLum.setOutput( texCurr );
        m_adaptedLum.update();
        m_adaptedLum.process( params );
    }

    void ImageProcessHDR::_bloomAndFlares( EffSetterParams& params )
    {
        //////////////////////////////////////////////////////////////////////////
        // bright pass
        m_brightpass.setSrcTex( m_texTargetQuarter );
        m_brightpass.setCurLumTex( m_curLumTexture );
        m_brightpass.setOutput( m_texTargetQuarterPost );
        m_brightpass.process( params );

        //////////////////////////////////////////////////////////////////////////
        // Bloom/Glow generation: 
        //  - using 3 textures, each with half resolution of previous, 
        //    Gaussian blurred and result summed up at end

        // bright pass and blur pBloomGen0
        m_blurBloom_4.setInput( m_texTargetQuarterPost );
        m_blurBloom_4.setScale( 1.0f );
        m_blurBloom_4.setDistribution( 3.0f );
        m_blurBloom_4.process( params );
   
        // 1/4 => 1/8
        m_scaleBloom_8.setInput( m_texTargetQuarterPost );
        m_scaleBloom_8.setOutput( m_texTargetEighth );
        m_scaleBloom_8.process( params );

        m_blurBloom_8.setInput( m_texTargetEighth );
        m_blurBloom_8.setScale( 1.0f );
        m_blurBloom_8.setDistribution( 3.0f );
        m_blurBloom_8.process( params );
      
        // 1/8 => 1/16
        m_scaleBloom_16.setInput( m_texTargetEighth );
        m_scaleBloom_16.setOutput( m_texTargetSixteenth );
        m_scaleBloom_16.process( params );

        m_blurBloom_16.setInput( m_texTargetSixteenth );
        m_blurBloom_16.setScale( 1.0f );
        m_blurBloom_16.setDistribution( 3.0f );
        m_blurBloom_16.process( params );

        // ��������bloom
        TextureObj* blooms[3] = { m_texTargetQuarterPost, m_texTargetEighth, m_texTargetSixteenth };
        m_flares.setBlooms( blooms );
        m_flares.setOutput( m_texFinalBloomQuarter );
        m_flares.process( params );
    }

    void ImageProcessHDR::_updateParams()
    {
        HDRSetting* setting = g_renderSystem->_getCurrHDRSetting();

        //////////////////////////////////////////////////////////////////////////
        // To maximize precision on bloom we keep original re-scale
        m_params[0].x = setting->getEyeAdaptationFactor();
        m_params[0].y = setting->getBrightOffset();

        // apply gamma correction to bright offset
        if ( g_renderSystem->_getCurrGammaSetting() )
            m_params[0].y *= m_params[0].y;

        m_params[0].z = setting->getBrightThreshold();
        m_params[0].w = setting->getBrightLevel(); 

        //////////////////////////////////////////////////////////////////////////
        m_params[1].x = setting->getEyeAdaptationBase();
        m_params[1].y = setting->getHDRLevel();
        m_params[1].z = setting->getHDROffset();

        // Output eye adaptation into register
        m_params[1].w = setting->getEyeAdaptationBase() * (1.0f - setting->getEyeAdaptationFactor()) + 
            setting->getEyeAdaptationFactor() * 1.0f;

        //////////////////////////////////////////////////////////////////////////
        m_params[2].x = 1.0f;
        m_params[2].y = 1.0f; 
        m_params[2].z = 1.0f;
        m_params[2].w = 1.0f;

        //////////////////////////////////////////////////////////////////////////
        Color clr = setting->getBloomColor() * setting->getBloomMul();
        clr.a = setting->getGrainAmount(); // no used

        m_params[5].x = clr.r;
        m_params[5].y = clr.g;
        m_params[5].z = clr.b;
        m_params[5].w = clr.a;

        //////////////////////////////////////////////////////////////////////////
        m_params[8].x = setting->getShoulderScale();
        m_params[8].y = setting->getMidtonesScale();
        m_params[8].z = setting->getToeScale();
        m_params[8].w = setting->getWhitePoint();
    }
}

