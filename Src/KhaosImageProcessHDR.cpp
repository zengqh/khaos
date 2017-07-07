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
        m_pin->setRttFormat( PIXFMT_INVALID ); // 用外部输出
        m_pin->setInputFilter( false ); // 输入线性采样
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
        m_pin->setRttFormat( PIXFMT_INVALID ); // 用外部输出
        m_pin->setInputFilter( false ); // 输入线性采样
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

        // 在支持线性采样float纹理的情况下，我们这样干，取4点即可，从而将16x16取样到1x1
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
        // MtrCommState::BIT0标记是否最后次迭代
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
        m_pin->setRttFormat( PIXFMT_INVALID ); // 用外部输出
        m_pin->setInputFilter( true ); // 输入点采样
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
        // 渐进调整
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
        m_pin->setRttFormat( PIXFMT_INVALID ); // 用外部输出
        m_pin->setInputFilter( true ); // 输入点采样
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
        m_pin->setRttFormat( PIXFMT_INVALID ); // 用外部输出
        m_pin->setInputFilter( true ); // 输入点采样
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
        // 直接输出到系统缓存
        m_pinToMainBuf = _createImagePin( ET_HDR_FINALPASS );
        m_pinToMainBuf->setInputFilter( true ); // 输入点采样
        m_pinToMainBuf->useMainRTT( true ); // 使用主相机rtt
        m_pinToMainBuf->useWPOSRender( true );

        // 输出到临时缓存
        m_pinToTmpBuf = _createImagePin( ET_HDR_FINALPASS );
        m_pinToTmpBuf->setOwnOutput();
        m_pinToTmpBuf->setInputFilter( true ); // 输入点采样
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

        // 检查gamma是否2.2，不是的话，hdr略做调整（hdr输出后已经做了gamma矫正2.2）
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
        bool needAdjGamma = !Math::realEqual( adjVal, 1.0f, 0.01f ); // 如果和1.0一样，那就不用调节了
        m_material->enableCommState( MtrCommState::BIT0, needAdjGamma );

        //bool useTempAA = g_renderSystem->_isAATempEnabled();
        //m_material->enableCommState( MtrCommState::BIT1, useTempAA );
        m_material->enableCommState( MtrCommState::BIT1, false ); // hdr内的tempaa永远没用了

        // 检查输出
        TextureObj* outBuf = g_imageProcessManager->getProcessHDR()->_getOutBuf();
        if ( outBuf ) // 有临时输出
        {
            m_pinToTmpBuf->getOutput()->linkRTT( outBuf );
            _setRoot( m_pinToTmpBuf );
        }
        else // 直接输出到系统缓存
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
        // 原图的缩小图
        m_scaleTargetHalf.setBigDownsample( false );
        m_scaleTargetQuarter.setBigDownsample( false );

        // 统计lumin用图 1,4,16,64
        KHAOS_CLEAR_ARRAY( m_texToneMaps );

        // 计算当前调整过的lumin，1x1大小
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

        // 更新参数
        _updateParams();

        // 降采样几张目标缩略图
        _downsampleHDRTarget( params );

        // 检测场景亮度，调节眼亮度
        int adaptCache = Math::maxVal(1, g_renderSystem->_getCurrHDRSetting()->getEyeAdaptionCache());

        if ( g_renderSystem->_getEffectFrame() % adaptCache == 0 )
        {  
            _measureLuminance( params );
            _eyeAdaptation( params );
        }

        // bloom闪烁
        _bloomAndFlares( params );

        // 最终渲染
        m_finalScene.process(params);
    }

    void ImageProcessHDR::_downsampleHDRTarget( EffSetterParams& params )
    {
        // 分配资源
        Camera* cam = g_renderSystem->_getMainCamera(); // 这里应该是依据主相机
        int vpWidth = cam->getViewportWidth();
        int vpHeight = cam->getViewportHeight();

        const PixelFormat targetFmt = PIXFMT_A16B16G16R16F;

        m_texTargetHalf = g_renderSystem->_getRTTBuffer( targetFmt, vpWidth/2, vpHeight/2 );
        m_texTargetQuarter = g_renderSystem->_getRTTBuffer( targetFmt, vpWidth/4, vpHeight/4 );
        m_texTargetQuarterPost = g_renderSystem->_getRTTBuffer( targetFmt, vpWidth/4, vpHeight/4 );
        m_texTargetEighth = g_renderSystem->_getRTTBuffer( targetFmt, vpWidth/8, vpHeight/8 );
        m_texTargetSixteenth = g_renderSystem->_getRTTBuffer( targetFmt, vpWidth/16, vpHeight/16 );

        m_texFinalBloomQuarter = g_renderSystem->_getRTTBuffer( /*targetFmt*/PIXFMT_A8R8G8B8, vpWidth/4, vpHeight/4 );

        // 缩小到一半
        m_scaleTargetHalf.setInput( m_inBuffer );
        m_scaleTargetHalf.setOutput( m_texTargetHalf );
        m_scaleTargetHalf.process( params );

        // 缩小到四分之一
        m_scaleTargetQuarter.setInput( m_texTargetHalf );
        m_scaleTargetQuarter.setOutput( m_texTargetQuarter );
        m_scaleTargetQuarter.process( params );
    }

    void ImageProcessHDR::_buildToneMaps()
    {
        if ( m_texToneMaps[0] )
            return;

        const PixelFormat fmt = PIXFMT_A16B16G16R16F; // PIXFMT_A16B16G16R16F PIXFMT_16F

        // 由于这些图可能保留几个帧的生命周期，我们直接创建他们
        // 统计lumin用图 1,4,16,64
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
        // 建立tone map资源
        _buildToneMaps();

        int curTexture = NUM_TONEMAP_TEXTURES-1;

        // 1/4原图作为输入，线性采样，统计初始化到64x64 tonemap
        m_initLumin.setInput( m_texTargetQuarter );
        m_initLumin.setOutput( m_texToneMaps[curTexture] );
        m_initLumin.process( params );
        
        // 将64x64迭代采样到1x1 
        --curTexture;

        while ( curTexture >= 0 )
        {
            m_luminIter.setInput( m_texToneMaps[curTexture+1] );
            m_luminIter.setOutput( m_texToneMaps[curTexture] );

            bool isLastIter = ( curTexture == 0 ); // 最后一次，我们要exp还原
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

        // 由于这些图可能保留几个帧的生命周期，我们直接创建他们
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

        // 当前的lum图索引
        int curLumIdx = m_curLumTextureTick;
        ++m_curLumTextureTick;
        curLumIdx &= MASK_CURRLUMIN_MAX;

        // 上一个lum图索引
        int lastLumIdx = (curLumIdx - 1) & MASK_CURRLUMIN_MAX;

        // 得到上一次图和本次要处理的图
        // texCurr的结果会在texPrev和m_texToneMaps[0]间插值
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

        // 生成最终bloom
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

