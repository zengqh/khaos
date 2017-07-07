#include "KhaosPreHeaders.h"
#include "KhaosRenderSystem.h"
#include "KhaosSceneGraph.h"
#include "KhaosGeneralRender.h"
#include "KhaosRenderDevice.h"
#include "KhaosImageProcessShadow.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class _SGFindShadowCasterByCamera : public SGFindBase
    {
    public:
        _SGFindShadowCasterByCamera() : m_hasResults(false) 
        {
            KHAOS_SFG_SET_PRECULL( _SGFindShadowCasterByCamera, _onPreCullNode );
            KHAOS_SFG_SET_RESULT( _SGFindShadowCasterByCamera, NT_RENDER, _onFindRenderNode );
        }

    public:
        bool _onPreCullNode( SceneNode* node )
        {
            return !static_cast<RenderableSceneNode*>(node)->isCastShadow();
        }

        void _onFindRenderNode( SceneNode* node )
        {
            m_hasResults = true;
            static_cast<RenderableSceneNode*>(node)->_addToRenderer();
        }

    public:
        bool m_hasResults;
    };

    //////////////////////////////////////////////////////////////////////////
    void RenderSystem::_generateShadowMaps( EffSetterParams& params )
    {
        // 生成主光源阴影
        _generateDirLitShadowMapDepth( params );
    }

    void RenderSystem::_prepareDirLitShadowMap()
    {
        // 是否有平行光
        if ( !m_currDirLit )
            return;

        // 更新阴影
        m_currDirLit->updateShadowInfo( m_mainCamera );

        // 是否活动
        if ( !m_currDirLit->_isShadowCurrActive() )
            return;

        // 进行准备，筛选投射物体
        m_renderPassStage = MP_SHADOW;
        bool hasResult = false; // 是否有阴影产生结果

        ShadowInfo* si = m_currDirLit->_getShadowInfo();
        int cascadesCount = si->cascadesCount;

        // 对于每个级
        for ( int splitIdx = 0; splitIdx < cascadesCount; ++splitIdx )
        {
            // 查询阴影视角caster数据，结果直接到队列
            m_currCamera = si->getRenderCaster(splitIdx); // 使用用来剔除的相机, 修正:还是使用render
            g_generalRender->beginAdd( m_renderPassStage, splitIdx );

            _SGFindShadowCasterByCamera finder;
            finder.setFinder( m_currCamera );
            finder.find( m_sceneGraph );

            g_generalRender->endAdd();

            if ( finder.m_hasResults ) // 有结果
            {
                hasResult = true;   
            }
        }

        // 假如当前没有任何投射物，可能优化掉
        si->isCurrentActive = hasResult;
    }

    PixelFormat RenderSystem::_getShadowMapDummyColorFormat() const
    {
        // 阴影使用硬件的话，得到一个最低的无用的颜色格式
        PixelFormat fmts[] = { PIXFMT_L8, PIXFMT_A8, PIXFMT_16F };

        for ( int i = 0; i < KHAOS_ARRAY_SIZE(fmts); ++i )
        {
            if ( g_renderDevice->isTextureFormatSupported( TEXUSA_RENDERTARGET, TEXTYPE_2D, fmts[i] ) )
                return fmts[i];
        }

        khaosAssert(0); // 16f不支持？
        return PIXFMT_A8R8G8B8;
    }

    void RenderSystem::_generateDirLitShadowMapDepth( EffSetterParams& params )
    {
        // 是否有平行光
        if ( !m_currDirLit )
            return;

        // 当前不活动
        if ( !m_currDirLit->_isShadowCurrActive() )
            return;

        // 分配shadowmap
        ShadowInfo* si = m_currDirLit->_getShadowInfo();
        int rttSize = si->needRttSize;
        int cascadesCount = si->cascadesCount;

        // 临时clr rtt，无用
        RenderBufferPool::ItemTemp clrTmp = m_renderBufferPool.getRTTBufferTemp( 
            _getShadowMapDummyColorFormat(), rttSize, rttSize );

#if MERGE_ONEMAP
        RenderBufferPool::Item* rttDepth = 
            m_renderBufferPool.getRTTBufferTemp( PIXFMT_D24S8, rttSize, rttSize ); // 合并一张
#else
        RenderBufferPool::Item* rttDepths[MAX_PSSM_CASCADES] = {};
#endif
        // 公共状态设置
        m_renderPassStage = MP_SHADOW;

        g_renderDevice->setSolidState( true );
        g_renderDevice->enableColorWriteenable( 0, false, false, false, false ); // 不需要写color

        params.clear();

        // 对于每个级
        for ( int splitIdx = 0; splitIdx < cascadesCount; ++splitIdx )
        {
            // 分配硬件sm需要的d24s8
#if !MERGE_ONEMAP
            rttDepths[splitIdx] = m_renderBufferPool.getRTTBufferTemp( PIXFMT_D24S8, rttSize, rttSize );
            RenderBufferPool::Item* rttDepth = rttDepths[splitIdx];
#endif
            si->linkResPre( splitIdx, clrTmp, rttDepth->rtt->getSurface(0) ); // clrTmp可以临时共用

            // 该级别rt生成sm
            RenderTarget* renderTarget = si->getRenderTarget(splitIdx);
            renderTarget->beginRender(0);

            Viewport* vp = renderTarget->getViewport(0); // 注意，vp内已经绑定了用来渲染的相机
            vp->apply(); // 即便没有投射物，深度也要清1（最远）

            if ( g_generalRender->hasRenderables( m_renderPassStage, splitIdx ) ) // 有结果
            {   
                params.stampGlobal();
                m_currCamera = vp->getCamera(); // vp内的相机用于渲染caster，不一定是剔除caster

                g_renderDevice->setDepthBias( si->slopeBias[splitIdx], si->constBias[splitIdx] ); // depth bias
                g_generalRender->renderShadowDirect( splitIdx, params );
            }

            renderTarget->endRender();

            // 硬件sm深度作为rtt
            si->linkResPost( splitIdx, rttDepth->rtt );
        }
        
        // 还原bias
        g_renderDevice->setDepthBias( 0, 0 );

        // 平行光延迟阴影
        if ( m_renderMode != RM_FORWARD )
        {
            g_renderDevice->enableColorWriteenable( 0, false, false, false, true ); // 不需要写rgb

            _setDrawOnly( RK_SHADOW );

            ImgProcShadowMask* proc = g_imageProcessManager->getProcessSMMask();
            proc->setOutput( m_gBuf.getNormalBuffer() ); // 写到normal g-buffer的a通道
            proc->process( params );

            _unsetDrawOnly();

            // 延迟用完可以直接归还深度，但是前向还需要保持
#if MERGE_ONEMAP
            rttDepth->setUnused();
#else
            for ( int splitIdx = 0; splitIdx < cascadesCount; ++splitIdx )
                rttDepths[splitIdx]->setUnused();
#endif
        }
 
        // 状态还原
        g_renderDevice->enableColorWriteenable( 0, true, true, true, true );
    }
}

