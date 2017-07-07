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
        // ��Ⱦ��ע��
        void _registerRenderer( RenderBase* base );

        // ���ص�ǰ��Ⱦ�׶�״̬
        MaterialPass _getRenderPassStage() const { return m_renderPassStage; }

        // ���ؼ������µĵ�ǰ֡��
        uint32 _getCurrFrame() const { return m_currFrame; }

        int _getEffectFrame() const { return m_effectFrame; }

        // ���ص�ǰ������״��
        RenderMode _getRenderMode() const{ return m_renderMode; }

        bool _isNormalMapEnabled() const { return m_enableNormalMap; }

        GammaSetting*     _getCurrGammaSetting() const { return m_currGammaSetting; }
        HDRSetting*       _getCurrHDRSetting() const { return m_currHDRSetting; }
        //AntiAliasSetting* _getCurrAntiAliasSetting() const { return m_currAntiAliasSetting; }

        bool _isAAMainEnabled() const;
        bool _isAATempEnabled() const;
        bool _isSSAOEnabled() const { return m_currSSAOEnabled; }

        // ���س���
        SceneGraph* _getCurrSceneGraph() const { return m_sceneGraph; }

        // �������
        Camera* _getMainCamera() const { return m_mainCamera; }
        Camera* _getCurrCamera() const { return m_currCamera; }

        // ��ǰ��Ӱ��ƽ�й�
        Light* _getCurrDirLit() const { return m_currDirLit; }

        // ��ʱ�û���
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
        // ��Ⱦ
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

        TextureObj*     m_sceneBuf;  // ������ɫbuffer���������ԣ�
        TextureObj*     m_sceneLastBuf; // ��һ֡�ĳ�����ɫbuffer(post effect֮ǰ)��TAA��

        TextureObj*     m_backBuff;  // ������AA�Ļ�,��ʱ������˻��棬����ֱ�������ϵͳbuffer

        // rs cmd
        RsCmdPtr        m_postEffCmd; // ��������

        // state
        MaterialPass    m_renderPassStage;
        uint32          m_currFrame;
        int             m_effectFrame;
    };

    extern RenderSystem* g_renderSystem;
}

