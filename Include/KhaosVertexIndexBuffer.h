#pragma once
#include "KhaosVertexDef.h"
#include "KhaosDebug.h"
#include "KhaosUtil.h"
#include "KhaosScopedPtr.h"
#include "KhaosRenderDeviceDef.h"
#include "KhaosVector4.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    // HardwareBuffer
    class HardwareBuffer : public AllocatedObject, public Noncopyable
    {
    public:
        HardwareBuffer() : m_size(0), m_usage(HBU_STATIC), m_cache(0) {}
        virtual ~HardwareBuffer() {}

    public:
        void destroy();

        virtual void* lock( HardwareBufferAccess access, int offset = 0, int size = 0 ) = 0;
        virtual void  unlock() = 0;

        void fillData( const void* data );
        void readData( void* data );

        void cacheLocalData( bool forceUpdate );
        void freeLocalData();
        void refundLocalData();

    public:
        int getSize() const { return m_size; }

    protected:
        virtual void _destroyImpl() = 0;

    protected:
        int                  m_size;
        HardwareBufferUsage  m_usage;
        uint8*               m_cache;
    };

    //////////////////////////////////////////////////////////////////////////
    // VertexElement
    inline int getVertexElementTypeBytes( VertexElementType type )
    {
        const int b[] = { 4, 8, 12, 16, 4, 4 };
        khaosAssert( 0 <= type && type < KHAOS_ARRAY_SIZE(b) );
        return b[type];
    }

    struct VertexElement
    {
    public:
        VertexElement() : semantic(VES_POSITION), type(VET_FLOAT1), offset(0), index(0) {}

    public:
        VertexElementSemantic semantic;
        VertexElementType     type;
        int                   offset;
        int                   index;
    };

    //////////////////////////////////////////////////////////////////////////
    // VertexDeclaration
    class VertexDeclaration : public AllocatedObject
    {
    protected:
        typedef vector<VertexElement>::type     ElementList;
        typedef map<int, int>::type             ElementMap;

    public:
        VertexDeclaration() : m_id(0), m_stride(0) {}
        virtual ~VertexDeclaration() {}

    public:
        void _setID( uint32 id ) { m_id = id; }
        uint32 getID() const { return m_id; }

        void nextElement( VertexFixedMask mask, VertexElementSemantic semantic, VertexElementType type, int offset, int index );

        const VertexElement& getElement( int i ) const { return m_elements[i]; }
        int getElementCount() const { return (int)m_elements.size(); }

        const VertexElement* findElement( VertexFixedMask mask ) const;

        void setStride( int stride ) { m_stride = stride; }
        int getStride() const { return m_stride; }

    protected:
        uint32      m_id;
        ElementList m_elements;
        ElementMap  m_eleMap;
        int         m_stride;
    };

    VertexDeclaration* _createVertexDeclaration( uint32 id );

    // 默认的几种顶点格式
    KHAOS_MAKE_VERTEX_STRUCT_DECL_5( VertexPNGTT, POS, NOR, TAN, TEX, TEX2 )
    KHAOS_MAKE_VERTEX_STRUCT_DECL_4( VertexPNTT, POS, NOR, TEX, TEX2 )
    KHAOS_MAKE_VERTEX_STRUCT_DECL_4( VertexPNGT, POS, NOR, TAN, TEX )
    KHAOS_MAKE_VERTEX_STRUCT_DECL_3( VertexPNT, POS, NOR, TEX )
    KHAOS_MAKE_VERTEX_STRUCT_DECL_3( VertexPTC, POS, TEX, CLR )
    KHAOS_MAKE_VERTEX_STRUCT_DECL_2( VertexPT, POS, TEX )
    KHAOS_MAKE_VERTEX_STRUCT_DECL_1( VertexP, POS )

    KHAOS_MAKE_VERTEX_STRUCT_DECL_8( VertexPNTCSH4, POS, NOR, TEX, CLR, SH0, SH1, SH2, SH3 )
    KHAOS_MAKE_VERTEX_STRUCT_DECL_7( VertexPNTSH4, POS, NOR, TEX, SH0, SH1, SH2, SH3 )
    KHAOS_MAKE_VERTEX_STRUCT_DECL_6( VertexPNTSH3, POS, NOR, TEX, SH0, SH1, SH2 )

    //////////////////////////////////////////////////////////////////////////
    // VertexBuffer
    class VertexBuffer : public HardwareBuffer
    {
    public:
        VertexBuffer() : m_declaration(0) {}
        virtual ~VertexBuffer() {}

    public:
        virtual bool create( int size, HardwareBufferUsage usage ) = 0;

    public:
        void setDeclaration( VertexDeclaration* vd ) { m_declaration = vd; }
        VertexDeclaration* getDeclaration() const { return m_declaration; }

        int getVertexCount() const { return m_size / m_declaration->getStride(); }

        bool hasElement( VertexFixedMask mask ) const;

        const Vector3* getCachePos( int vtxIdx ) const;
        Vector3* getCachePos( int vtxIdx );

        const Vector3* getCacheNormal( int vtxIdx ) const;
        Vector3* getCacheNormal( int vtxIdx );

        const Vector4* getCacheTanget( int vtxIdx ) const;
        Vector4* getCacheTanget( int vtxIdx );

        const Vector2* getCacheTex( int vtxIdx ) const;
        Vector2* getCacheTex( int vtxIdx );

        const Vector2* getCacheTex2( int vtxIdx ) const;
        Vector2* getCacheTex2( int vtxIdx );

        const float* getCacheSH( int vtxIdx ) const;
        float* getCacheSH( int vtxIdx );

        const uint32* getCacheColor( int vtxIdx ) const;
        uint32* getCacheColor( int vtxIdx );

        void copyFrom( VertexBuffer* src );

    protected:
        void* _getCacheByEle( VertexFixedMask mask, int vtxIdx );

    protected:
        VertexDeclaration* m_declaration;
    };

    typedef ScopedPtr<VertexBuffer, SPFM_DELETE> VertexBufferScpPtr;

    //////////////////////////////////////////////////////////////////////////
    // IndexBuffer
    inline int getIndexElementTypeBytes( IndexElementType type )
    {
        int b[] = { 2, 4 };
        khaosAssert( 0 <= type && type < KHAOS_ARRAY_SIZE(b) );
        return b[type];
    }

    class IndexBuffer : public HardwareBuffer
    {
    public:
        using HardwareBuffer::readData;

    public:
        IndexBuffer() : m_indexType(IET_INDEX16) {}
        virtual ~IndexBuffer() {}

    public:
        virtual bool create( int size, HardwareBufferUsage usage, IndexElementType type ) = 0;

        void readData( vector<int>::type& data );
        void readData( vector<ushort>::type& data );

    public:
        IndexElementType getIndexType() const { return m_indexType; }
        int getIndexCount() const { return m_size / getIndexElementTypeBytes(m_indexType); }

        int  getCacheIndex( int i ) const;
        void getCacheTriIndices( int face, int& v0, int& v1, int &v2 ) const;

        void setCacheIndex( int i, int index );

        void copyFrom( IndexBuffer* src );

    protected:
        IndexElementType m_indexType;
    };

    typedef ScopedPtr<IndexBuffer, SPFM_DELETE> IndexBufferScpPtr;

    //////////////////////////////////////////////////////////////////////////
    // VertexDeclarationManager
    class VertexDeclarationManager : public AllocatedObject
    {
    public:
        typedef unordered_map<uint32, VertexDeclaration*>::type DeclarationMap;

    public:
        VertexDeclarationManager();
        ~VertexDeclarationManager();

    public:
        void registerDeclaration( uint32 id, VertexDeclaration* vd );
        VertexDeclaration* getDeclaration( uint32 id ) const;

    private:
        void _init();
        void _clear();

    private:
        DeclarationMap m_declMap;
    };

    extern VertexDeclarationManager* g_vertexDeclarationManager;

    //////////////////////////////////////////////////////////////////////////
    // PrimitiveType
    inline int getPrimitiveTypeVertexCount( PrimitiveType type )
    {
        int cnt[] = { 3, 2 };
        khaosAssert( 0 <= type && type < KHAOS_ARRAY_SIZE(cnt) );
        return cnt[type];
    }
}

