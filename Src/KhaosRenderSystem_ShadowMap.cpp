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
        // ��������Դ��Ӱ
        _generateDirLitShadowMapDepth( params );
    }

    void RenderSystem::_prepareDirLitShadowMap()
    {
        // �Ƿ���ƽ�й�
        if ( !m_currDirLit )
            return;

        // ������Ӱ
        m_currDirLit->updateShadowInfo( m_mainCamera );

        // �Ƿ�
        if ( !m_currDirLit->_isShadowCurrActive() )
            return;

        // ����׼����ɸѡͶ������
        m_renderPassStage = MP_SHADOW;
        bool hasResult = false; // �Ƿ�����Ӱ�������

        ShadowInfo* si = m_currDirLit->_getShadowInfo();
        int cascadesCount = si->cascadesCount;

        // ����ÿ����
        for ( int splitIdx = 0; splitIdx < cascadesCount; ++splitIdx )
        {
            // ��ѯ��Ӱ�ӽ�caster���ݣ����ֱ�ӵ�����
            m_currCamera = si->getRenderCaster(splitIdx); // ʹ�������޳������, ����:����ʹ��render
            g_generalRender->beginAdd( m_renderPassStage, splitIdx );

            _SGFindShadowCasterByCamera finder;
            finder.setFinder( m_currCamera );
            finder.find( m_sceneGraph );

            g_generalRender->endAdd();

            if ( finder.m_hasResults ) // �н��
            {
                hasResult = true;   
            }
        }

        // ���統ǰû���κ�Ͷ��������Ż���
        si->isCurrentActive = hasResult;
    }

    PixelFormat RenderSystem::_getShadowMapDummyColorFormat() const
    {
        // ��Ӱʹ��Ӳ���Ļ����õ�һ����͵����õ���ɫ��ʽ
        PixelFormat fmts[] = { PIXFMT_L8, PIXFMT_A8, PIXFMT_16F };

        for ( int i = 0; i < KHAOS_ARRAY_SIZE(fmts); ++i )
        {
            if ( g_renderDevice->isTextureFormatSupported( TEXUSA_RENDERTARGET, TEXTYPE_2D, fmts[i] ) )
                return fmts[i];
        }

        khaosAssert(0); // 16f��֧�֣�
        return PIXFMT_A8R8G8B8;
    }

    void RenderSystem::_generateDirLitShadowMapDepth( EffSetterParams& params )
    {
        // �Ƿ���ƽ�й�
        if ( !m_currDirLit )
            return;

        // ��ǰ���
        if ( !m_currDirLit->_isShadowCurrActive() )
            return;

        // ����shadowmap
        ShadowInfo* si = m_currDirLit->_getShadowInfo();
        int rttSize = si->needRttSize;
        int cascadesCount = si->cascadesCount;

        // ��ʱclr rtt������
        RenderBufferPool::ItemTemp clrTmp = m_renderBufferPool.getRTTBufferTemp( 
            _getShadowMapDummyColorFormat(), rttSize, rttSize );

#if MERGE_ONEMAP
        RenderBufferPool::Item* rttDepth = 
            m_renderBufferPool.getRTTBufferTemp( PIXFMT_D24S8, rttSize, rttSize ); // �ϲ�һ��
#else
        RenderBufferPool::Item* rttDepths[MAX_PSSM_CASCADES] = {};
#endif
        // ����״̬����
        m_renderPassStage = MP_SHADOW;

        g_renderDevice->setSolidState( true );
        g_renderDevice->enableColorWriteenable( 0, false, false, false, false ); // ����Ҫдcolor

        params.clear();

        // ����ÿ����
        for ( int splitIdx = 0; splitIdx < cascadesCount; ++splitIdx )
        {
            // ����Ӳ��sm��Ҫ��d24s8
#if !MERGE_ONEMAP
            rttDepths[splitIdx] = m_renderBufferPool.getRTTBufferTemp( PIXFMT_D24S8, rttSize, rttSize );
            RenderBufferPool::Item* rttDepth = rttDepths[splitIdx];
#endif
            si->linkResPre( splitIdx, clrTmp, rttDepth->rtt->getSurface(0) ); // clrTmp������ʱ����

            // �ü���rt����sm
            RenderTarget* renderTarget = si->getRenderTarget(splitIdx);
            renderTarget->beginRender(0);

            Viewport* vp = renderTarget->getViewport(0); // ע�⣬vp���Ѿ�����������Ⱦ�����
            vp->apply(); // ����û��Ͷ������ҲҪ��1����Զ��

            if ( g_generalRender->hasRenderables( m_renderPassStage, splitIdx ) ) // �н��
            {   
                params.stampGlobal();
                m_currCamera = vp->getCamera(); // vp�ڵ����������Ⱦcaster����һ�����޳�caster

                g_renderDevice->setDepthBias( si->slopeBias[splitIdx], si->constBias[splitIdx] ); // depth bias
                g_generalRender->renderShadowDirect( splitIdx, params );
            }

            renderTarget->endRender();

            // Ӳ��sm�����Ϊrtt
            si->linkResPost( splitIdx, rttDepth->rtt );
        }
        
        // ��ԭbias
        g_renderDevice->setDepthBias( 0, 0 );

        // ƽ�й��ӳ���Ӱ
        if ( m_renderMode != RM_FORWARD )
        {
            g_renderDevice->enableColorWriteenable( 0, false, false, false, true ); // ����Ҫдrgb

            _setDrawOnly( RK_SHADOW );

            ImgProcShadowMask* proc = g_imageProcessManager->getProcessSMMask();
            proc->setOutput( m_gBuf.getNormalBuffer() ); // д��normal g-buffer��aͨ��
            proc->process( params );

            _unsetDrawOnly();

            // �ӳ��������ֱ�ӹ黹��ȣ�����ǰ����Ҫ����
#if MERGE_ONEMAP
            rttDepth->setUnused();
#else
            for ( int splitIdx = 0; splitIdx < cascadesCount; ++splitIdx )
                rttDepths[splitIdx]->setUnused();
#endif
        }
 
        // ״̬��ԭ
        g_renderDevice->enableColorWriteenable( 0, true, true, true, true );
    }
}

