#pragma once
#include "KhaosAxisAlignedBox.h"
#include "KhaosRenderBufferPool.h"
#include "KhaosGBuffer.h"
#include "KhaosRenderSetting.h"
#include "KhaosRSCmd.h"

namespace Khaos
{
    class Texture;
    class Viewport;
    class Camera;
    class Light;
    class LightsInfo;
    class SceneNode;
    class RenderableSceneNode;
    class CameraNode;
    class LightNode;
    class EnvProbeNode;
    class VolumeProbeNode;
    class SceneGraph;
    class RenderBase;
    class ImagePin;
	class LightManager;
    struct EffSetterParams;
    enum  MaterialPass;

    typedef vector<RenderableSceneNode*>::type  RenderableSceneNodeList;
    typedef vector<LightNode*>::type            LightNodeList;
    typedef vector<EnvProbeNode*>::type         EnvProbeNodeList;
    typedef vector<VolumeProbeNode*>::type      VolumeProbeNodeList;

    extern Color g_backColor;

    //////////////////////////////////////////////////////////////////////////
    // RenderSystem
    class RenderSystem : public AllocatedObject
    {
        typedef vector<RenderBase*>::type RendererList;

    public:
        enum
        {
            RK_SHADOW  = 0x1,
            RK_OPACITY = 0x2,
        };

    public:
        RenderSystem();
        ~RenderSystem();

    public:
        void init();
        void shutdown();

        void update();

        void beginRender();
        void renderScene( CameraNode* camera );
        void endRender();

        void queryLightsInfo( const AxisAlignedBox& aabb, LightsInfo& litsInfo, bool checkSM );

        void addPostEffRSCmd( RsCmdPtr& cmd );

    public:
        // 渲染器注册
        void _registerRenderer( RenderBase* base );

        // 返回当前渲染阶段状态
        MaterialPass _getRenderPassStage() const { return m_renderPassStage; }

        // 返回即将更新的当前帧号
        uint32 _getCurrFrame() const { return m_currFrame; }

        int _getEffectFrame() const { return m_effectFrame; }

        // 返回当前的配置状况
        RenderMode _getRenderMode() const{ return m_renderMode; }

        bool _isNormalMapEnabled() const { return m_enableNormalMap; }

        GammaSetting*     _getCurrGammaSetting() const { return m_currGammaSetting; }
        HDRSetting*       _getCurrHDRSetting() const { return m_currHDRSetting; }
        //AntiAliasSetting* _getCurrAntiAliasSetting() const { return m_currAntiAliasSetting; }

        bool _isAAMainEnabled() const;
        bool _isAATempEnabled() const;
        bool _isSSAOEnabled() const { return m_currSSAOEnabled; }

        // 返回场景
        SceneGraph* _getCurrSceneGraph() const { return m_sceneGraph; }

        // 返回相机
        Camera* _getMainCamera() const { return m_mainCamera; }
        Camera* _getCurrCamera() const { return m_currCamera; }

        // 当前阴影的平行光
        Light* _getCurrDirLit() const { return m_currDirLit; }

        // 临时用缓存
        RenderBufferPool::Item* _getRTTBufferTemp( PixelFormat fmt, int width, int height, const String* name = 0 );
        TextureObj*             _getRTTBuffer( PixelFormat fmt, int width, int height, const String* name = 0 );
        TextureObj*             _createRTTBuffer( PixelFormat fmt, int width, int height );
        RenderBufferPool::Item* _getRenderBuffItem( const String& name ) const;

        GBuffer&      _getGBuf() { return m_gBuf; }
        TextureObj*   _getDepthHalf() { return m_depthHalf; }
        TextureObj*   _getDepthQuarter() { return m_depthQuarter; }

        TextureObj* _getAoBuf() const { return m_aoBuf; }

        TextureObj* _getCurrSceneBuf() const { return m_sceneBuf; }
        //TextureObj* _getSceneLastBuf() const { return m_sceneLastBuf; }
        //TextureObj* _getBackBuf() const { return m_backBuff; }

        // Pin
        void _processPin( EffSetterParams& params, ImagePin* pin );

        void _beginGammaCorrect();
        void _endGammaCorrect();

        // draw flag
        void _buildDrawFlag( int kind );
        void _unbuildDrawFlag();

        void _setDrawOnly( int kind );
        void _setDrawNot( int kind );
        void _unsetDrawOnly();

		// LitMgr
		LightManager* _getLitMgr() const { return m_litMgr; }

    private:
        // 渲染
        void _calcMainResults();
        void _updateLitInfos();
        void _prepareDirLitShadowMap();
        void _generateEnvMaps( EffSetterParams& params );
        void _generateGBuffers( EffSetterParams& params );
        void _generateAOBuffer( EffSetterParams& params );
        void _generateLitBuffers( EffSetterParams& params );
        void _generateShadowMaps( EffSetterParams& params );
        void _generateComposite( EffSetterParams& params );
        void _renderMainResults( EffSetterParams& params );
        void _renderPostEffects( EffSetterParams& params );
        void _renderDebugTmp( EffSetterParams& params );
        void _postRender();

        Light* _fetchMainDirLit( const LightNodeList& litList ) const;
        void   _generateEnvProbe( EffSetterParams& params, EnvProbeNode* envProbeNode );
        void   _generateDirLitShadowMapDepth( EffSetterParams& params );

        void  _setDefaultRS();
        void  _drawDebugTexture( EffSetterParams& params, int x, int y, int width, int height, TextureObj* tex );
        void  _setMainCamera( CameraNode* camera );

        PixelFormat _getShadowMapDummyColorFormat() const;

        void _prepareSceneBuffers();
        void _prepareBackBufferOnce();
        void _setSceneBuffer();
        void _unsetSceneBuffer();

    private:
        // manager
        RendererList     m_rendererList;
        RenderBufferPool m_renderBufferPool;
        LightManager*    m_litMgr;

        // for rendering at one frame
        SceneGraph*    m_sceneGraph;
        Camera*        m_mainCamera;
        Camera*        m_currCamera;

        RenderMode     m_renderMode;
        bool           m_enableNormalMap;
        bool           m_currSSAOEnabled;

        GammaSetting*       m_currGammaSetting;
        HDRSetting*         m_currHDRSetting;
        AntiAliasSetting*   m_currAntiAliasSetting;

        RenderableSceneNodeList m_renderResultsMain;
        LightNodeList           m_litResultsMain;
        EnvProbeNodeList        m_envpResultsMain;
        VolumeProbeNodeList     m_volpResultsMain;
        Light*                  m_currDirLit;

        // render buffer
        GBuffer         m_gBuf;
        TextureObj*     m_depthHalf;
        TextureObj*     m_depthQuarter; 

        TextureObj*     m_aoBuf;

        TextureObj*     m_sceneBuf;  // 场景颜色buffer，总是线性！
        TextureObj*     m_sceneLastBuf; // 上一帧的场景颜色buffer(post effect之前)，TAA用

        TextureObj*     m_backBuff;  // 假如有AA的话,临时输出到此缓存，不是直接输出到系统buffer

        // rs cmd
        RsCmdPtr        m_postEffCmd; // 后期命令

        // state
        MaterialPass    m_renderPassStage;
        uint32          m_currFrame;
        int             m_effectFrame;
    };

    extern RenderSystem* g_renderSystem;
}

