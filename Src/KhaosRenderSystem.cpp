#include "KhaosPreHeaders.h"
#include "KhaosRenderSystem.h"
#include "KhaosSceneGraph.h"
#include "KhaosGeneralRender.h"
#include "KhaosLightRender.h"
#include "KhaosSkyRender.h"
#include "KhaosRenderFeature.h"
#include "KhaosRenderDevice.h"
#include "KhaosTextureManager.h"
#include "KhaosRenderTarget.h"
#include "KhaosSysResManager.h"
#include "KhaosEffectBuildStrategy.h"
#include "KhaosImageProcess.h"
#include "KhaosMaterialManager.h"
#include "KhaosImageProcessAA.h"
#include "KhaosLightMgr.h"
#include "KhaosEnvLitBuilder.h"
#include "KhaosVolumeProbeRender.h"

namespace Khaos
{
    Color g_backColor( 130/255.0f, 157/255.0f, 184/255.0f );

    //////////////////////////////////////////////////////////////////////////
    // RenderSystem
    RenderSystem* g_renderSystem = 0;

    RenderSystem::RenderSystem() :
		m_litMgr(KHAOS_NEW LightManager),
        m_sceneGraph(0), m_mainCamera(0), m_currCamera(0),
        m_renderMode(RM_DEFERRED_SHADING), m_enableNormalMap(false), m_currSSAOEnabled(false), 
        m_currGammaSetting(0), m_currHDRSetting(0), m_currAntiAliasSetting(0),
        m_currDirLit(0),
        m_depthHalf(0), m_depthQuarter(0),
        m_sceneBuf(0), m_sceneLastBuf(0), m_backBuff(0),
        m_renderPassStage(MP_MATERIAL), 
        m_currFrame(0), m_effectFrame(0)
    {
        khaosAssert(!g_renderSystem);
        g_renderSystem = this;
    }

    RenderSystem::~RenderSystem()
    {
		KHAOS_DELETE m_litMgr;
        g_renderSystem = 0;
    }

    void RenderSystem::init()
    {
        // 渲染配置管理
        KHAOS_NEW RenderSettingManager;

        // 模版管理器
        KHAOS_NEW EffectTemplateManager;

        // 后期管理
        KHAOS_NEW ImageProcessManager;

        // g-buffer
        m_gBuf.init();

        // 创建渲染器
        KHAOS_NEW GeneralRender;
        KHAOS_NEW LightRender;
        KHAOS_NEW VolumeProbeRender;
        KHAOS_NEW SkyRender;

        // 初始化各个渲染器
        KHAOS_FOR_EACH( RendererList, m_rendererList, it )
        {
            RenderBase* base = *it;
            base->init();
        }

        // 其他
        g_effectTemplateManager->registerEffectTemplateVSPS( ET_UI, "uirect", KHAOS_NEW UIBuildStrategy );

        buildEnvLUTMap( "/System/EnvLUT.dds", 128 );
    }

    void RenderSystem::shutdown()
    {
        // 关闭各个渲染器
        KHAOS_FOR_EACH( RendererList, m_rendererList, it )
        {
            RenderBase* base = *it;
            base->shutdown();
            KHAOS_DELETE base;
        }

        m_rendererList.clear();

        // g-buffer
        m_gBuf.clean();

        // 销毁管理器
        KHAOS_DELETE g_imageProcessManager;
        KHAOS_DELETE g_effectTemplateManager;
        KHAOS_DELETE g_renderSettingManager;

        KHAOS_SAFE_DELETE( m_sceneBuf );
        KHAOS_SAFE_DELETE( m_sceneLastBuf );
    }

    void RenderSystem::_registerRenderer( RenderBase* base )
    {
        m_rendererList.push_back( base );
    }

    void RenderSystem::queryLightsInfo( const AxisAlignedBox& aabb, LightsInfo& litsInfo, bool checkSM )
    {
        litsInfo.clear();
        Vector3 pos = aabb.getCenter();

        // 查询主批次灯光，计算受影响的光
        KHAOS_FOR_EACH_CONST( LightNodeList, m_litResultsMain, it )
        {
            LightNode* node = *it;
            if ( node->intersects( aabb ) )
            {
                float dist = node->squaredDistance( pos );

                if ( checkSM ) // 需要检查阴影距离
                {
                    // 灯光应当已经updateShadowInfo了
                    // 判断是否真正需要接受阴影
                    if ( node->getLight()->_isShadowCurrActive() )
                    {
                        checkSM = node->getLight()->_getShadowInfo()->getCullReceiver()->testVisibility( aabb );
                    }
                    else
                    {
                        checkSM = false;
                    }
                }

                litsInfo.addLight( node->getLight(), dist, checkSM );
            }
        }

        // 按照对当前相机远近做排序
        static const int maxEffectLits[LT_MAX] =
        {
            MAX_DIRECTIONAL_LIGHTS,
            MAX_POINT_LIGHTS,
            MAX_SPOT_LIGHTS
        };

        for ( int i = 0; i < LT_MAX; ++i )
        {
            litsInfo.sortN( (LightType)i, maxEffectLits[i] );
        }
    }

    RenderBufferPool::Item* RenderSystem::_getRTTBufferTemp( PixelFormat fmt, int width, int height, const String* name )
    {
        return m_renderBufferPool.getRTTBufferTemp( fmt, width, height, name );
    }

    TextureObj* RenderSystem::_getRTTBuffer( PixelFormat fmt, int width, int height, const String* name )
    {
        return m_renderBufferPool.getRTTBuffer( fmt, width, height, name );
    }

    TextureObj* RenderSystem::_createRTTBuffer( PixelFormat fmt, int width, int height )
    {
        return m_renderBufferPool.createRTTBuffer( fmt, width, height );
    }

    RenderBufferPool::Item* RenderSystem::_getRenderBuffItem( const String& name ) const
    {
        return m_renderBufferPool.getItem( name );
    }

    void RenderSystem::update()
    {
        MaterialManager::update();
        g_effectTemplateManager->update();
    }

    void RenderSystem::beginRender()
    {
        ++m_currFrame; // 放在开头，比其他模块总保持领先

        g_renderDevice->beginRender();

        // 默认rs设置
        _setDefaultRS();

        // 重置渲染缓存使用
        m_renderBufferPool.resetAllBufers();
    }

    void RenderSystem::_setDefaultRS()
    {
        g_renderDevice->enableStencil( false );
        g_renderDevice->setSolidState( true );
        g_renderDevice->setMaterialStateSet( MaterialStateSet::FRONT_DRAW );
    }

    void RenderSystem::endRender()
    {
        m_effectFrame = (m_effectFrame + 1) % 1000; // 一些效果用frame

        g_renderDevice->endRender();
    }

    void RenderSystem::_drawDebugTexture( EffSetterParams& params, int x, int y, int width, int height, TextureObj* tex )
    {
        params.clearAndStampGlobal();

        MeshRectDebugRenderable ra;
        ra.setRect( x, y, width, height );
        ra.setTexture( tex );

        EffectContext* eff = g_effectTemplateManager->getEffectTemplate(ET_UI)->getEffectContext(&ra);
        g_renderDevice->setEffect( eff->getEffect() );

        params.ra = &ra;
        eff->doSet( &params );

        ra.render();
    }

    void RenderSystem::_setMainCamera( CameraNode* camera )
    {
        m_sceneGraph = camera->getSceneGraph();
        m_mainCamera = camera->getCamera();

        // 检查配置
        RenderSettings* settings = m_mainCamera->getRenderSettings();
        khaosAssert( settings );

        // 渲染模式
        m_renderMode = settings->getRenderMode();
        m_enableNormalMap = settings->getEnableNormalMap();

        // 后期开启
        m_currSSAOEnabled = false;
        m_currHDRSetting = 0;
        m_currAntiAliasSetting = 0;

        if ( m_renderMode != RM_FORWARD )
        {
            m_currSSAOEnabled = settings->isSettingEnabled<SSAORenderSetting>();
        }

        m_currHDRSetting        = settings->getEnabledSetting<HDRSetting>();
        m_currAntiAliasSetting  = settings->getEnabledSetting<AntiAliasSetting>();

        // gamma矫正
        m_currGammaSetting = settings->getEnabledSetting<GammaSetting>();
        g_renderDevice->enableSRGB( m_currGammaSetting != 0 );

        // aa的camera jitter
        g_imageProcessManager->getProcessPostAA()->applyJitter( m_mainCamera );
    }

    bool RenderSystem::_isAATempEnabled() const
    {
        if ( m_renderMode != RM_FORWARD ) // temp 需要 depth
        {
            if ( m_currAntiAliasSetting )
                return m_currAntiAliasSetting->getEnableTemporal();
        }

        return false;
    }

    bool RenderSystem::_isAAMainEnabled() const
    {
        if ( m_currAntiAliasSetting )
            return m_currAntiAliasSetting->getEnableMain();
        return false;
    }

    void RenderSystem::_postRender()
    {
        m_mainCamera->getRenderSettings()->_clearAllDirty();
    }

    void RenderSystem::_beginGammaCorrect()
    {
        // 开启gamma矫正，启用srgb write状态
        if ( m_currGammaSetting )
            g_renderDevice->enableSRGBWrite( true );
    }

    void RenderSystem::_endGammaCorrect()
    {
        if ( m_currGammaSetting )
            g_renderDevice->enableSRGBWrite( false );
    }

    void RenderSystem::renderScene( CameraNode* camera )
    {
        // 记录当前活动的场景图和主相机
        _setMainCamera( camera );
        
        // 初始化setter params
        EffSetterParams params;
        params.stampExtern();

        // 计算主视角数据
        _calcMainResults();

        // 更新当前灯光的一些信息
        _updateLitInfos();

        // 生成环境反射贴图
        _generateEnvMaps( params ); // 在g-buffer前，可利用临时深度

        // 延迟渲染生成G-Buffers
        _generateGBuffers( params );

        // 生成ao
        _generateAOBuffer( params );

        // 延迟渲染生成Lit-Buffers
        _generateLitBuffers( params );

        // 生成阴影贴图
        _generateShadowMaps( params );

        // 合成pass
        _generateComposite( params );

        // 渲染主视角数据
        _renderMainResults( params );

        // 渲染后期的一些效果
        _renderPostEffects( params );

        // 调试临时
        _renderDebugTmp( params );

        // 渲染之后处理
        _postRender();
    }

    void RenderSystem::_buildDrawFlag( int kind )
    {
        // 为此种类型建立模板标记
        g_renderDevice->enableStencil( true );

        static StencilStateSet sss( 0, 
            CMP_ALWAYS, -1, -1, STENCILOP_KEEP, STENCILOP_KEEP, STENCILOP_REPLACE );

        sss.refVal = kind;

        g_renderDevice->setStencilStateSet( sss );
    }

    void RenderSystem::_unbuildDrawFlag()
    {
        g_renderDevice->enableStencil( false );
    }

    void RenderSystem::_setDrawOnly( int kind )
    {
        // 只画此种类型的东西
        g_renderDevice->enableStencil( true );

        static StencilStateSet sss(0, CMP_LESS, 0, -1, 
            STENCILOP_KEEP, STENCILOP_KEEP, STENCILOP_KEEP);

        sss.cmpMask = kind;

        g_renderDevice->setStencilStateSet( sss );
    }

    void RenderSystem::_setDrawNot( int kind )
    {
        // 不画此种类型的东西
        g_renderDevice->enableStencil( true );

        static StencilStateSet sss(0, CMP_EQUAL, 0, -1, 
            STENCILOP_KEEP, STENCILOP_KEEP, STENCILOP_KEEP);

        sss.cmpMask = kind;

        g_renderDevice->setStencilStateSet( sss );
    }

    void RenderSystem::_unsetDrawOnly()
    {
        g_renderDevice->enableStencil( false );
    }

    void RenderSystem::addPostEffRSCmd( RsCmdPtr& cmd )
    {
        m_postEffCmd = cmd;
    }
}

