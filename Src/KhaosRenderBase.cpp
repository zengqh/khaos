#include "KhaosPreHeaders.h"
#include "KhaosRenderBase.h"
#include "KhaosRenderSystem.h"
#include "KhaosRenderDevice.h"
#include "KhaosCameraNode.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    RenderableContextList::RenderableContextList() : m_size(0)
    {
        m_list.push_back( m_pool.requestObject() ); // 至少保留1个，保证m_list[0]合法
    }

    RenderableContextList::~RenderableContextList()
    {
        KHAOS_FOR_EACH(RcList, m_list, it)
        {
            m_pool.freeObject( *it );
        }
    }

    RenderableContextList::RcPtr RenderableContextList::pushBack()
    {
        RcPtr rc;

        if ( (int)m_list.size() > m_size )
        {
            rc = m_list[m_size];
        }
        else
        {
            rc = m_pool.requestObject();
            m_list.push_back( rc );
        }
       
        ++m_size;
        return rc;
    }

    void RenderableContextList::clear()
    {
        m_size = 0;
    }

    static bool _sortF2B( RenderableContext* const lhs, RenderableContext* const rhs )
    {
        return lhs->dist < rhs->dist;
    }

    static bool _sortB2F( RenderableContext* const lhs, RenderableContext* const rhs )
    {
        return lhs->dist > rhs->dist;
    }

    void RenderableContextList::sortBackToFront()
    {
        std::sort( m_list.begin(), m_list.begin()+m_size, _sortB2F );
    }

    void RenderableContextList::sortFrontToBack()
    {
        std::sort( m_list.begin(), m_list.begin()+m_size, _sortF2B );
    }

    const RenderableContextList::RcPtr* RenderableContextList::begin() const
    {
        return &m_list[0];
    }

    const RenderableContextList::RcPtr* RenderableContextList::end() const
    {
        return &m_list[0] + m_size;
    }

    RenderableContextList::RcPtr* RenderableContextList::begin()
    {
        return &m_list[0];
    }

    RenderableContextList::RcPtr* RenderableContextList::end()
    {
        return &m_list[0] + m_size;
    }

    RenderableContextList::RcPtr RenderableContextList::at( int idx ) const
    {
        khaosAssert( 0 <= idx && idx < m_size );
        return m_list[idx];
    }

    int RenderableContextList::getSize() const
    {
        return m_size;
    }

    //////////////////////////////////////////////////////////////////////////
    uint32 KindRenderableList::_makeSortKey( EffectContext* ec, Renderable* raObj )
    {
        // 13bit effect(8192 max) | 19bit material(524288 max)
        return (ec->getRID() << 19) | raObj->getImmMaterial()->getRID();
    }

    RenderableContext* KindRenderableList::pushBack( EffectContext* ec, Renderable* raObj )
    {
        uint32 key = _makeSortKey( ec, raObj );
        RenderableContextList*& rcList = m_listMap[key];

        if ( !rcList )
            rcList = m_listPool.requestObject();

        return rcList->pushBack();
    }

    void KindRenderableList::clear()
    {
        KHAOS_FOR_EACH( RcListMap, m_listMap, it )
        {
            RenderableContextList* rcList = it->second;
            rcList->clear();
            m_listPool.freeObject( rcList );
        }

        m_listMap.clear();
    }

    void KindRenderableList::sortBackToFront()
    {
        KHAOS_FOR_EACH( RcListMap, m_listMap, it )
        {
            RenderableContextList* rcList = it->second;
            rcList->sortBackToFront();
        }
    }

    void KindRenderableList::sortFrontToBack()
    {
        KHAOS_FOR_EACH( RcListMap, m_listMap, it )
        {
            RenderableContextList* rcList = it->second;
            rcList->sortFrontToBack();
        }
    }

    bool KindRenderableList::hasSize() const
    {
        KHAOS_FOR_EACH_CONST( RcListMap, m_listMap, it )
        {
            RenderableContextList* rcList = it->second;
            if ( rcList->getSize() > 0 )
                return true;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    RenderBase::RenderBase()
    {
        g_renderSystem->_registerRenderer( this ); 
    }

    void RenderBase::_renderList( KindRenderableList& raList, EffSetterParams& params )
    {
        EffectContext* currEffContext = (EffectContext*)-1;

        // 渲染每个分组
        KHAOS_FOR_EACH( KindRenderableList::RcListMap, raList.getRenderables(), itGroup )
        {
            // 渲染该组
            RenderableContextList* subList = itGroup->second;
            int cnt = subList->getSize();

            // 设置该组的effect
            RenderableContext* rc = subList->at(0);
            if ( currEffContext != rc->effContext )
            {
                currEffContext = rc->effContext;
                g_renderDevice->setEffect( currEffContext->getEffect() );
            }

            // 渲染该组中物体
            for ( int i = 0; i < cnt; ++i )
            {
                RenderableContext* rc = subList->at(i);
                khaosAssert( rc->effContext == currEffContext );

                Renderable* ra = rc->raObj;
                params.ra = ra;

                currEffContext->doSet( &params );
                ra->render();
            }
        }
    }

    void RenderBase::_renderList( RenderableContextList& raList, EffSetterParams& params )
    {
        int cnt = raList.getSize();

        for ( int i = 0; i < cnt; ++i )
        {
            RenderableContext* rc = raList.at(i);
            EffectContext* effContext = rc->effContext;
            g_renderDevice->setEffect( effContext->getEffect() );

            Renderable* ra = rc->raObj;
            params.ra = ra;
            effContext->doSet( &params );
            ra->render();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    CommRenderBase::CommRenderBase() : m_currPass(MP_MATERIAL), m_currIdx(0)
    {
    }

    void CommRenderBase::beginAdd( MaterialPass pass, int i )
    {
        m_currPass = pass;
        m_currIdx  = i;

        switch ( m_currPass )
        {
        case MP_MATERIAL:
        case MP_REFLECT:
        case MP_DEFPRE:
            m_solidGroup.clear();
            m_transParentGroup.clear();
            break;

        case MP_SHADOW:
            m_shadowGroup[i].clear();
            break;
        }
    }

    void CommRenderBase::endAdd()
    {
        switch ( m_currPass )
        {
        case MP_MATERIAL:
        case MP_REFLECT:
        case MP_DEFPRE:
            m_solidGroup.sortFrontToBack();
            m_transParentGroup.sortBackToFront();
            break;

        case MP_SHADOW:
            m_shadowGroup[m_currIdx].sortFrontToBack();
            break;
        }
    }

    bool CommRenderBase::hasRenderables( MaterialPass pass, int i ) const
    {
        switch ( pass )
        {
        case MP_SHADOW:
            return m_shadowGroup[i].hasSize();

        default:
            khaosAssert(0);
            return false;
        }
    }

    void CommRenderBase::addRenderable( Renderable* ra )
    {
        Material* mtr = ra->getImmMaterial();

        // 阴影/延迟阶段提早剔除
        if ( m_currPass == MP_SHADOW || m_currPass == MP_DEFPRE )
        {
            if ( mtr->isBlendEnabled() ) // blend不需要
                return;
        }

        // 材质绘制阶段提早剔除
        if ( m_currPass == MP_MATERIAL )
        {
            if ( g_renderSystem->_getRenderMode() == RM_DEFERRED_SHADING )
            {
                // 延迟渲染模式那么只画透明的即可
                if ( !mtr->isBlendEnabled() )
                    return;
            }
        }

        // 获取effect模板
        EffectTemplate* effTemp = g_effectTemplateManager->getEffectTemplate( mtr->getEffTempId(m_currPass) );

        // 得到effect上下文
        EffectContext* effContext = effTemp->getEffectContext( ra );

        // 渲染上下文
        RenderableContext* rc = 0;

        switch ( m_currPass )
        {
        case MP_MATERIAL:
        case MP_REFLECT:
        case MP_DEFPRE:
            {
                if ( mtr->isBlendEnabled() ) // 混合
                    rc = m_transParentGroup.pushBack();
                else // 实体
                    rc = m_solidGroup.pushBack( effContext, ra );
            }
            break;

        case MP_SHADOW:
            {
                rc = m_shadowGroup[m_currIdx].pushBack( effContext, ra );
            }
            break;
        }

        rc->raObj = ra;
        rc->effContext = effContext;
        rc->dist = g_renderSystem->_getCurrCamera()->getEyePos().squaredDistance( 
            ra->getImmAABBWorld().getCenter() );
    }

    void CommRenderBase::renderSolidWS( EffSetterParams& params )
    {
        g_renderDevice->setSolidState( true );
        renderSolidDirect( params );
    }

    void CommRenderBase::renderTransWS( EffSetterParams& params )
    {
        g_renderDevice->setSolidState( false );
        renderTransDirect( params );
    }

    void CommRenderBase::renderSolidTransWS( EffSetterParams& params )
    {
        renderSolidWS( params );
        renderTransWS( params );
    }

    void CommRenderBase::renderSolidDirect( EffSetterParams& params )
    {
        _renderList( m_solidGroup, params );
    }

    void CommRenderBase::renderTransDirect( EffSetterParams& params )
    {
        _renderList( m_transParentGroup, params );
    }

    void CommRenderBase::renderShadowDirect( int i, EffSetterParams& params )
    {
        _renderList( m_shadowGroup[i], params );
    }
}

