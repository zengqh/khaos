#pragma once
#include "KhaosRenderable.h"
#include "KhaosEffectContext.h"
#include "KhaosObjectRecycle.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    // RenderableContext
    struct RenderableContext : public AllocatedObject
    {
        RenderableContext() : raObj(0), effContext(0), dist(0) {}

        Renderable*     raObj;
        EffectContext*  effContext;
        float           dist;
    };

    class RenderableContextList : public AllocatedObject
    {
    private:
        typedef RenderableContext* RcPtr;
        typedef vector<RcPtr>::type RcList;
        typedef ObjectRecycle<RenderableContext> RenderableContextRecycle;

    public:
        RenderableContextList();
        ~RenderableContextList();

        RcPtr pushBack();
        void  clear();

        void sortBackToFront();
        void sortFrontToBack();

        const RcPtr* begin() const;
        const RcPtr* end() const;

        RcPtr* begin();
        RcPtr* end();

        RcPtr at( int idx ) const;
        int   getSize() const;

    private:
        RcList m_list;
        RenderableContextRecycle m_pool;
        int m_size;
    };

    //////////////////////////////////////////////////////////////////////////
    class KindRenderableList : public AllocatedObject
    {
    public:
        typedef map<uint32, RenderableContextList*>::type RcListMap;
        typedef ObjectRecycle<RenderableContextList, SPFM_DELETE_T> RcListRecycle;

    public:
        ~KindRenderableList() { clear(); }

        RenderableContext* pushBack( EffectContext* ec, Renderable* raObj );
        void clear();

        const RcListMap& getRenderables() const { return m_listMap; }
        RcListMap& getRenderables() { return m_listMap; }

        void sortBackToFront();
        void sortFrontToBack();

        bool hasSize() const;

    private:
        uint32 _makeSortKey( EffectContext* ec, Renderable* raObj );

    private:
        RcListMap       m_listMap;
        RcListRecycle   m_listPool;
    };

    //////////////////////////////////////////////////////////////////////////
    class RenderBase : public AllocatedObject
    {
    public:
        RenderBase();
        virtual ~RenderBase() {}

    public:
        virtual void init() = 0;
        virtual void shutdown() {}

    protected:
        void _renderList( KindRenderableList& raList, EffSetterParams& params );
        void _renderList( RenderableContextList& raList, EffSetterParams& params );
    };

    class CommRenderBase : public RenderBase
    {
    public:
        CommRenderBase();

        void beginAdd( MaterialPass pass, int i = 0 );
        void addRenderable( Renderable* ra );
        void endAdd();

        void renderSolidDirect( EffSetterParams& params );
        void renderTransDirect( EffSetterParams& params );
        void renderShadowDirect( int i, EffSetterParams& params );

        void renderSolidWS( EffSetterParams& params );
        void renderTransWS( EffSetterParams& params );
        void renderSolidTransWS( EffSetterParams& params );

        bool hasRenderables( MaterialPass pass, int i = 0 ) const;

    protected:
        MaterialPass          m_currPass;
        KindRenderableList    m_solidGroup;       // solid
        RenderableContextList m_transParentGroup; // alpha
        KindRenderableList    m_shadowGroup[4];   // shadow 4 == MAX_PSSM_CASCADES
        int                   m_currIdx;
    };
}

