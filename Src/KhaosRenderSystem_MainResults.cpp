#include "KhaosPreHeaders.h"
#include "KhaosRenderSystem.h"
#include "KhaosSceneGraph.h"
#include "KhaosGeneralRender.h"
#include "KhaosRenderDevice.h"
#include "KhaosSkyRender.h"
#include "KhaosImageProcessHDR.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class _SGFindMainResultsByCamera : public SGFindBase
    {
    public:
        _SGFindMainResultsByCamera(
            RenderableSceneNodeList* renderNodes,
            LightNodeList*           litNodes,
            EnvProbeNodeList*        envProbeNodes,
            VolumeProbeNodeList*     volProbeNodes
            ) 
            : m_renderNodes(renderNodes), m_litNodes(litNodes), m_envProbeNodes(envProbeNodes), m_volProbeNodes(volProbeNodes)
        {
            renderNodes->clear();
            litNodes->clear();
            envProbeNodes->clear();
            volProbeNodes->clear();

            KHAOS_CLEAR_ARRAY(m_litCnt);

            KHAOS_SFG_SET_RESULT( _SGFindMainResultsByCamera, NT_RENDER,    _onFindRenderNode );
            KHAOS_SFG_SET_RESULT( _SGFindMainResultsByCamera, NT_LIGHT,     _onFindLightNode );
            KHAOS_SFG_SET_RESULT( _SGFindMainResultsByCamera, NT_ENVPROBE,  _onFindEnvProbeNode );
            KHAOS_SFG_SET_RESULT( _SGFindMainResultsByCamera, NT_VOLPROBE,  _onFindVolProbeNode );
        }

    public:
        void _onFindRenderNode( SceneNode* node )
        {
            m_renderNodes->push_back( static_cast<RenderableSceneNode*>(node) );
        }

        void _onFindLightNode( SceneNode* node )
        {
            LightNode* ln    = static_cast<LightNode*>(node);
            LightType  ltype = ln->getLight()->getLightType();
            int&       cnt   = m_litCnt[ltype];

            if ( cnt < 255 ) // 1个字节能放完,预留255
            {
                ln->getLight()->_setTempId( cnt );
                m_litNodes->push_back( ln );
                ++cnt;
            }
        }

        void _onFindEnvProbeNode( SceneNode* node )
        {
            m_envProbeNodes->push_back( static_cast<EnvProbeNode*>(node) );
        }

        void _onFindVolProbeNode( SceneNode* node )
        {
            m_volProbeNodes->push_back( static_cast<VolumeProbeNode*>(node) );
        }

    private:
        RenderableSceneNodeList* m_renderNodes;
        LightNodeList*           m_litNodes;
        EnvProbeNodeList*        m_envProbeNodes;
        VolumeProbeNodeList*     m_volProbeNodes;
        int                      m_litCnt[LT_MAX];
    };

    //////////////////////////////////////////////////////////////////////////
    void RenderSystem::_calcMainResults()
    {
        // 查询主视角数据，结果保存在ResultsMain
        _SGFindMainResultsByCamera finder(&m_renderResultsMain, &m_litResultsMain, &m_envpResultsMain, &m_volpResultsMain);
        finder.setFinder( m_mainCamera );
        finder.find( m_sceneGraph );

        // 决定1个平行光为主光源
        m_currDirLit = _fetchMainDirLit( m_litResultsMain );
    }

    Light* RenderSystem::_fetchMainDirLit( const LightNodeList& litList ) const
    {
        Light* litMax = 0;

        KHAOS_FOR_EACH_CONST(LightNodeList, litList, it )
        {
            LightNode* node = (*it);
            Light* lit = node->getLight();
            if ( lit->getLightType() == LT_DIRECTIONAL && lit > litMax )
                litMax = lit;
        }

        return litMax;
    }

    void RenderSystem::_updateLitInfos()
    {
        // 用主相机信息更新阴影计算的信息
        _prepareDirLitShadowMap();
    }

    void RenderSystem::_prepareSceneBuffers()
    {
        // 我们需要一个线性的buffer来保存颜色场景
        if ( !m_sceneBuf )
            m_sceneBuf = m_renderBufferPool.createRTTBuffer( PIXFMT_A16B16G16R16F, 0, 0 );

        if ( _isAATempEnabled() ) // 开启了taa
        {
            if ( !m_sceneLastBuf ) // 需要另一个场景颜色buffer
                m_sceneLastBuf = m_renderBufferPool.createRTTBuffer( PIXFMT_A16B16G16R16F, 0, 0 );

            // 有aa后，上一次sceneBuf变成sceneLastBuf，而当前画在了上一次lastBufer上。
            // 也就是两者交换下。
            swapVal( m_sceneBuf, m_sceneLastBuf );
        }
        else // 无taa
        {
            // 不需要多余的buffer
            KHAOS_SAFE_DELETE( m_sceneLastBuf );
        }

        m_backBuff = 0; // for aa
    }

    void RenderSystem::_prepareBackBufferOnce()
    {
        // 要确保1帧只调用1次
        if ( !m_backBuff )
            m_backBuff = m_renderBufferPool.getRTTBuffer( PIXFMT_A8R8G8B8, 0, 0 );
    }

    void RenderSystem::_setSceneBuffer()
    {
        m_currCamera->_setViewport(0);
        g_renderDevice->setRenderTarget( 0, m_sceneBuf->getSurface(0) );
        g_renderDevice->restoreMainDepthStencil();
        g_renderDevice->restoreMainViewport();
    }

    void RenderSystem::_unsetSceneBuffer()
    {
        g_renderDevice->restoreMainRenderTarget(0);
    }

    void RenderSystem::_renderMainResults( EffSetterParams& params )
    {
        // 延迟阶段，画完不透明物体后画天空背景层，然后后面接着画半透明物体
        if ( m_renderMode != RM_FORWARD )
        {
            g_skyRender->renderBackground( params );
        }

        // 派发主视角数据
        m_currCamera = m_mainCamera;
        m_renderPassStage = MP_MATERIAL;

        g_generalRender->beginAdd( m_renderPassStage );

        KHAOS_FOR_EACH_CONST( RenderableSceneNodeList, m_renderResultsMain, it )
        {
            RenderableSceneNode* node = *it;
            node->_addToRenderer(); // 每个节点将可渲染体加入所属的渲染器
        }

        g_generalRender->endAdd();

        // 渲染
        params.clearAndStampGlobal();

        _setSceneBuffer();

        if ( m_renderMode == RM_FORWARD ) // 前向模式画完整不透明物体或者透明物体
        {
            g_renderDevice->clear( RCF_DEPTH|RCF_STENCIL, 0, 1.0f, 0 ); // 颜色由天空填充
            
            // 画不透明物体
            _buildDrawFlag( RK_OPACITY );
            g_generalRender->renderSolidWS(params);
            _unbuildDrawFlag();

            // 画天空背景
            g_skyRender->renderBackground( params );

            // 画透明物体
            m_currCamera = m_mainCamera;
            m_renderPassStage = MP_MATERIAL;
            params.clearAndStampGlobal();
            _setSceneBuffer();

            g_generalRender->renderTransWS( params );

        }
        else // 延迟只要再画透明即可，不透明在延迟阶段已经画好了
        {
            g_generalRender->renderTransWS(params);
        }
        
        _unsetSceneBuffer();
    }

    void RenderSystem::_renderDebugTmp( EffSetterParams& params )
    {
        return;

        m_currCamera = m_mainCamera;
        m_renderPassStage = MP_ETC;

        m_currCamera->_setViewport(0);
        g_renderDevice->restoreMainRTT(); // only main window current

        params.clearAndStampGlobal();
       
        g_renderDevice->setDepthStateSet( DepthStateSet::ALL_DISABLED );
        g_renderDevice->enableBlendState( false );

        _beginGammaCorrect();

#if 0
        // g-buf
        TextureObj* bufA = m_gBuf.getDiffuseBuffer();
        TextureObj* bufB = m_gBuf.getSpecularBuffer();
		TextureObj* bufC = m_gBuf.getNormalBuffer();
		TextureObj* bufD = m_gBuf.getDepthBuffer();

		const int showsize = 150;
		const int spacesize = showsize+10;
		int startx = 10;
        _drawDebugTexture( params, startx, 470, showsize, showsize, bufA );
        _drawDebugTexture( params, startx+=spacesize, 470, showsize, showsize, bufB );
		_drawDebugTexture( params, startx+=spacesize, 470, showsize, showsize, bufC );
		_drawDebugTexture( params, startx+=spacesize, 470, showsize, showsize, bufD );
#endif

#if 0
        // shadow map
        ShadowInfo* si = m_currDirLit->_getShadowInfo();
        TextureObj* bufShadow = si->texture->getTextureObj();
        _drawDebugTexture( params, 10, 470, 200, 200, bufShadow );
#endif

#if 0
        // lit-buf
        TextureObj* bufLit = m_litBuf.getRTT();
        _drawDebugTexture( params, 0, 0, bufLit->getWidth(), bufLit->getHeight(), bufLit );
        //_drawDebugTexture( params, 430, 470, 200, 200, bufLit );
#endif

#if 0
        // ao-buf
        TextureObj* bufAo = m_aoBuf.getRTT();
        _drawDebugTexture( params, 0, 0, bufAo->getWidth(), bufAo->getHeight(), bufAo );

        //TextureObj* bufAoH = m_renderBufferPool.getItem("testh")->rtt;
        //_drawDebugTexture( params, 640, 260, 200, 200, bufAoH );
#endif

#if 0
        // hdr buf
        if ( m_currHDRSetting )
        {
            ImageProcessHDR* process = g_imageProcessManager->getProcessHDR();

            // 1/2 target
            int offsetx = 0, offsety = 0;

            TextureObj* bufHalf = process->_getTargetHalf();
            //_drawDebugTexture( params, offsetx, offsety, 
            //    bufHalf->getWidth(), bufHalf->getHeight(), bufHalf );

            // 1/4 target
            offsetx = bufHalf->getWidth() + 1;

            TextureObj* bufQuarter = process->_getTargetQuarter();
            //_drawDebugTexture( params, offsetx, offsety, 
            //    bufQuarter->getWidth(), bufQuarter->getHeight(), bufQuarter );

            // 64x64 tonemap
            offsetx += bufQuarter->getWidth() + 1;

            for ( int i = 3; i >= 1; --i )
            {
                TextureObj* toneMap = process->_getToneMap(i);
                //_drawDebugTexture( params, offsetx, offsety, 
                //    toneMap->getWidth(), toneMap->getHeight(), toneMap );
                offsetx += toneMap->getWidth() + 1;
            }

            TextureObj* toneMap0 = process->_getToneMap(0);
            _drawDebugTexture( params, offsetx, offsety, 32, 32, toneMap0 );
            offsetx += 32+1;

            TextureObj* currLum = process->_getCurrLumin();
            _drawDebugTexture( params, offsetx, offsety, 32, 32, currLum );

            TextureObj* bloom4 = process->_getTargetQuarterPost();
            offsetx = 0; offsety = bufHalf->getHeight();
            _drawDebugTexture( params, offsetx, offsety, bloom4->getWidth(), bloom4->getHeight(), bloom4 );
            offsetx += bloom4->getWidth();

            TextureObj* bloom8 = process->_getTargetEighth();
            //_drawDebugTexture( params, offsetx, offsety, bloom8->getWidth(), bloom8->getHeight(), bloom8 );
            offsetx += bloom8->getWidth();

            TextureObj* bloom16 = process->_getTargetSixteenth();
            //_drawDebugTexture( params, offsetx, offsety, bloom16->getWidth(), bloom16->getHeight(), bloom16 );
            offsetx += bloom16->getWidth();

            TextureObj* bloomF = process->_getFinalBloomQuarter();
            offsetx = 0; offsety = bufHalf->getHeight() + bloom4->getHeight();
            //_drawDebugTexture( params, offsetx, offsety, bloomF->getWidth(), bloomF->getHeight(), bloomF );
            offsetx += bloomF->getWidth();
        }
#endif
        _endGammaCorrect();
    }
}

