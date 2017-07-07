#include "KhaosPreHeaders.h"
#include "KhaosImageProcess.h"
#include "KhaosEffectID.h"
#include "KhaosRenderTarget.h"
#include "KhaosRenderSystem.h"
#include "KhaosEffectBuildStrategy.h"
#include "KhaosEffectContext.h"
#include "KhaosCamera.h"
#include "KhaosSysResManager.h"
#include "KhaosImageProcessComposite.h"
#include "KhaosImageProcessShadow.h"
#include "KhaosImageProcessHDR.h"
#include "KhaosImageProcessAA.h"
#include "KhaosImageProcessUtil.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    ImagePin::ImagePin( ImageProcess* owner ) : m_owner(owner), m_effTempId(0), 
        m_inputTex(0), m_output(0), m_mtr(0), m_tmpRtt(0), 
        m_rttFormat(PIXFMT_INVALID), m_resolutionSize(RESSIZE_FULL), 
        m_flag(0), m_texFilterType(TextureFilterSet::NEAREST), m_outRTTLevel(0),
        m_ownOutput(false), m_ownRttSize(false), m_holdRtt(false), 
        m_clearFlagEnabled(false), m_useMainRTT(false), m_useMainDepth(false),
        m_isWPOSRender(false), m_isBlendEnabled(false)
    {
        m_mtr = g_sysResManager->getDefaultMaterial();
    }

    ImagePin::~ImagePin()
    {
        KHAOS_FOR_EACH(ImagePinList, m_children, it )
        {
            ImagePin* child = *it;
            KHAOS_DELETE child;
        }

        freeRttRes();

        if ( m_ownOutput )
            KHAOS_DELETE m_output;
    }

    void ImagePin::setInput( TextureObj* obj )
    {
        m_inputTex = obj;
    }

    TextureObj* ImagePin::getInput() const
    {
        // 返回该pin的自动化输入
        if ( m_inputTex ) // 有自定义输入
            return m_inputTex;

        // 只返回第一个孩子的输出，也就是孩子的输出作为我的输入，适用于链接
        khaosAssert( m_children.size() == 1 );
        return m_children.front()->getOutput()->getRTT();
    }

    void ImagePin::setInputFilter( bool filterPoint ) 
    {
        if ( filterPoint )
            m_texFilterType = TextureFilterSet::NEAREST;
        else
            m_texFilterType = TextureFilterSet::BILINEAR;
    }

    void ImagePin::setOwnOutput()
    {
        setOwnOutputEx(0);
    }

    void ImagePin::setOwnOutputEx( uint32 clearFlag, const Color& clrClear, float z, int s )
    {
        // 设定自主输出
        if ( !m_output )
        {
            m_output = KHAOS_NEW RenderTarget;
            m_output->createViewport(0);
        }
        
        m_output->getViewport(0)->setClearFlag( clearFlag );
        m_output->getViewport(0)->setClearColor( clrClear );

        m_clearFlagEnabled = (clearFlag != 0);
        m_ownOutput = true;
    }

    void ImagePin::clearOwnOutput()
    {
        KHAOS_SAFE_DELETE(m_output);
        m_clearFlagEnabled = false;
        m_ownOutput = false;
    }

    void ImagePin::setRequestRttSize( const IntVector2& sz )
    {
        m_rttSize = sz;
        m_ownRttSize = true;
    }

    IntVector2 ImagePin::getRequestRttSize() const
    {
        // 获取需要的rtt尺寸

        // 有自定义设置优先
        if ( m_ownRttSize )
            return m_rttSize;

        // 自动根据viewport获取
        Camera* cam = g_renderSystem->_getMainCamera(); // 这里应该是依据主相机

        IntVector2 size( cam->getViewportWidth(), cam->getViewportHeight() );

        if ( m_resolutionSize == RESSIZE_THREE_QUARTER )
        {
            size.x = size.x * 3 / 4;
            size.y = size.y * 3 / 4;
        }
        else if ( m_resolutionSize == RESSIZE_HALF )
        {
            size.x /= 2;
            size.y /= 2;
        }

        return size;
    }

    void ImagePin::requestRttRes()
    {
        // 请求分配临时的rtt资源，并绑定到输出
        if ( m_rttFormat == PIXFMT_INVALID )
            return;

        IntVector2 rttSize = getRequestRttSize();
        m_tmpRtt = g_renderSystem->_getRTTBufferTemp( m_rttFormat, rttSize.x, rttSize.y );
        m_output->linkRTT( m_tmpRtt->rtt );
    }

    void ImagePin::freeRttRes()
    {
        if ( m_tmpRtt && !m_holdRtt ) // 不需要持有临时资源则删除
        {
            m_tmpRtt->setUnused();
            m_tmpRtt = 0;
        }
    }

    ImagePin* ImagePin::addChild( ImagePin* child )
    {
        m_children.push_back(child);
        return child;
    }

    void ImagePin::setResolutionSizeDerived( ResolutionSize rs )
    {
        // 递归设定解析度
        KHAOS_FOR_EACH( ImagePinList, m_children, it )
        {
            ImagePin* pin = *it;
            pin->setResolutionSizeDerived( rs );
        }

        setResolutionSize( rs );
    }

    void ImagePin::process( EffSetterParams& params )
    {
        _processDerived( params );
        freeRttRes(); // 最后释放自己资源
    }

    void ImagePin::_processDerived( EffSetterParams& params )
    {
        // 孩子先做
        KHAOS_FOR_EACH( ImagePinList, m_children, it )
        {
            ImagePin* pin = *it;
            pin->_processDerived( params );
        }

        // 然后自己做  
        g_renderSystem->_processPin( params, this );

        // 做完后释放孩子rtt资源
        KHAOS_FOR_EACH( ImagePinList, m_children, it )
        {
            ImagePin* pin = *it;
            pin->freeRttRes();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcess::ImageProcess() : m_pinRoot(0), m_settings(0), m_userData(0)
    {
    }

    ImageProcess::~ImageProcess()
    {
        KHAOS_DELETE m_pinRoot;
    }

    void ImageProcess::process( EffSetterParams& params )
    {
        // 首先检查更新一些信息
        _checkSettings();

        // 然后从pin的根结点开始执行
        m_pinRoot->process( params );
    }

    ImagePin* ImageProcess::_createImagePin( int templateId )
    {
        // 创建一个给定效果模版id的pin
        ImagePin* pin = KHAOS_NEW ImagePin(this);
        pin->setEffectTempId( templateId );
        return pin;
    }

    void ImageProcess::_setRoot( ImagePin* pin )
    {
        // 设定根pin
        m_pinRoot = pin;

        if ( m_pinRoot )
            m_pinRoot->setHoldRtt(); // 一般最终输出保留rtt资源
    }

    void ImageProcess::_checkSettings()
    {
        // 主配置有变化
        RenderSettings* curSettings = g_renderSystem->_getMainCamera()->getRenderSettings();
        if ( curSettings != m_settings )
        {
            m_settings = curSettings;
            _onDirtySettings();
            return;
        }

        // 检查使用到的配置
        vector<ClassType>::type types;
        _getNeedSettingTypes( types );

        for ( size_t i = 0; i < types.size(); ++i )
        {
            RenderSetting* setting = m_settings->getSetting( types[i] );
            if ( setting->_isDirty() )
            {
                _onDirtySettings();
                break;
            }
        }
    }

    void ImageProcess::_setResolutionSize( ResolutionSize rs )
    {
        // 设定后期解析度
        m_pinRoot->setResolutionSizeDerived( rs );
    }

    //////////////////////////////////////////////////////////////////////////
    ImageProcessManager* g_imageProcessManager = 0;

    ImageProcessManager::ImageProcessManager()
    {
        khaosAssert( !g_imageProcessManager );
        g_imageProcessManager = this;
        _init();
    }

    ImageProcessManager::~ImageProcessManager()
    {
        _clear();
        g_imageProcessManager = 0;
    }

    void ImageProcessManager::_init()
    {
        _initEffect();
        _initProcess();
    }

    void ImageProcessManager::_clear()
    {
        KHAOS_DELETE m_shadowMask;
        KHAOS_DELETE m_ssao;
        KHAOS_DELETE m_hdr;
        KHAOS_DELETE m_postAA;
        KHAOS_DELETE m_copy;
        KHAOS_DELETE m_deferComposite;
    }

    void ImageProcessManager::_initEffect()
    {
        _registerEffectVSPS2HS( ET_COMPOSITE, "deferComposite", "deferComposite", 
            ("pbrUtil", "materialUtil", "deferUtil"), DeferCompositeBuildStrategy );

        _registerEffectVSPS2HS( ET_SHADOWMASK, "shadowMask", "shadowMask", 
            ("deferUtil", "shadowUtil"), ShadowMaskBuildStrategy );

        _registerEffectVSPS( ET_DOWNSCALEZ, "downscaleZ", DownscaleZBuildStrategy );

        _registerEffectVSPS( ET_SSAO, "ssao", SSAOBuildStrategy );
        _registerEffectVSPS( ET_SSAO_BLUR, "ssaoFilter", SSAOFilterBuildStrategy );

        _registerEffectVSPS( ET_COMM_SCALE, "commScale", CommScaleBuildStrategy );
        _registerEffectVSPS( ET_COMM_BLUR, "commBlur", CommBlurBuildStrategy );
        _registerEffectVSPS2( ET_COMM_COPY, "commDrawScreen", "commCopy", CommCopyBuildStrategy );

        _registerEffectVSPS2( ET_HDR_INITLUMIN, "commDrawScreen", "hdrInitLumin", HDRInitLuminBuildStrategy );
        _registerEffectVSPS2( ET_HDR_LUMINITER, "commDrawScreen", "hdrLumIterative", HDRLuminIterativeBuildStrategy );
        _registerEffectVSPS2( ET_HDR_ADAPTEDLUM, "commDrawScreen", "hdrAdaptedLum", HDRAdaptedLumBuildStrategy );
        _registerEffectVSPS2( ET_HDR_BRIGHTPASS, "commDrawScreen", "hdrBrightPass", HDRBrightPassBuildStrategy );
        _registerEffectVSPS2( ET_HDR_FLARESPASS, "commDrawScreen", "hdrFlaresPass", HDRFlaresPassBuildStrategy );
        _registerEffectVSPS2H( ET_HDR_FINALPASS, "commDrawScreenWPOS", "hdrFinalScene", 
            "deferUtil", HDRFinalPassBuildStrategy );

        // post aa
        _registerEffectVSPS2H( ET_POSTAA_EDGE_DETECTION, "AAEdgeDetection", "AAEdgeDetection", 
            "smaaUtil", AAEdgeDetectionBuildStrategy );
        _registerEffectVSPS2H( ET_POSTAA_BLENDING_WEIGHT_CALC, "AABlendingWeightsCalc", "AABlendingWeightsCalc", 
            "smaaUtil", AABlendingWeightsCalcBuildStrategy );
        _registerEffectVSPS2H( ET_POSTAA_FINAL, "AAFinal", "AAFinal", 
            "smaaUtil", AAFinalBuildStrategy );

        _registerEffectVSPS2H( ET_POSTAA_FINAL2, "AAFinal2", "AAFinal2", 
            "fxaaUtil", AAFinal2BuildStrategy );

        _registerEffectVSPS2H( ET_POSTAA_TEMP, "commDrawScreenWPOS", "AATemp", 
            "deferUtil", AATempBuildStrategy );
    }

    void ImageProcessManager::_initProcess()
    {
        m_deferComposite = KHAOS_NEW ImgProcComposite;
        m_copy = KHAOS_NEW ImageCopyProcess;
        m_shadowMask = KHAOS_NEW ImgProcShadowMask;
        m_ssao = KHAOS_NEW ImageProcessSSAO;
        m_hdr  = KHAOS_NEW ImageProcessHDR; 
        m_postAA = KHAOS_NEW ImageProcessPostAA;
    }
    
}

