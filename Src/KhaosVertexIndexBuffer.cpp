#include "KhaosPreHeaders.h"
#include "KhaosVertexIndexBuffer.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    // HardwareBuffer
    void HardwareBuffer::destroy()
    {
        freeLocalData();
        _destroyImpl();
    }

    void HardwareBuffer::cacheLocalData( bool forceUpdate )
    {
        if ( forceUpdate )
            freeLocalData();

        if ( m_cache )
            return;

        int needBytes = Math::maxVal( m_size, 1 );
        m_cache = (uint8*) KHAOS_MALLOC(needBytes);
        readData( m_cache );
    }

    void HardwareBuffer::freeLocalData()
    {
        if ( m_cache )
        {
            KHAOS_FREE(m_cache);
            m_cache = 0;
        }
    }

    void HardwareBuffer::refundLocalData()
    {
        if ( m_cache )
            fillData( m_cache );
    }

    void HardwareBuffer::fillData( const void* data )
    {
        void* ptr = lock( HBA_WRITE );
        memcpy( ptr, data, m_size );
        unlock();
    }

    void HardwareBuffer::readData( void* data )
    {
        void* ptr = lock( HBA_READ );
        memcpy( data, ptr, m_size );
        unlock();
    }

    //////////////////////////////////////////////////////////////////////////
    bool VertexBuffer::hasElement( VertexFixedMask mask ) const
    {
        return (m_declaration->getID() & mask) != 0;
    }

    const Vector3* VertexBuffer::getCachePos( int vtxIdx ) const
    {
        return const_cast<VertexBuffer*>(this)->getCachePos( vtxIdx );
    }

    Vector3* VertexBuffer::getCachePos( int vtxIdx )
    {
        khaosAssert( 0 <= vtxIdx && vtxIdx < getVertexCount() );
        int offset = m_declaration->getStride() * vtxIdx;
        return (Vector3*)(m_cache + offset); // pos always at first
    }

    void* VertexBuffer::_getCacheByEle( VertexFixedMask mask, int vtxIdx )
    {
        khaosAssert( 0 <= vtxIdx && vtxIdx < getVertexCount() );
        int shOffset = m_declaration->findElement( mask )->offset;
        int offset = m_declaration->getStride() * vtxIdx + shOffset;
        return (void*)(m_cache + offset);
    }

    const Vector3* VertexBuffer::getCacheNormal( int vtxIdx ) const
    {
        return const_cast<VertexBuffer*>(this)->getCacheNormal( vtxIdx );
    }

    Vector3* VertexBuffer::getCacheNormal( int vtxIdx )
    {
        return (Vector3*)_getCacheByEle(VFM_NOR, vtxIdx);
    }

    const Vector4* VertexBuffer::getCacheTanget( int vtxIdx ) const
    {
        return const_cast<VertexBuffer*>(this)->getCacheTanget( vtxIdx );
    }

    Vector4* VertexBuffer::getCacheTanget( int vtxIdx )
    {
        return (Vector4*)_getCacheByEle(VFM_TAN, vtxIdx);
    }

    const Vector2* VertexBuffer::getCacheTex( int vtxIdx ) const
    {
        return const_cast<VertexBuffer*>(this)->getCacheTex( vtxIdx );
    }

    Vector2* VertexBuffer::getCacheTex( int vtxIdx )
    {
        return (Vector2*)_getCacheByEle(VFM_TEX, vtxIdx);
    }

    const Vector2* VertexBuffer::getCacheTex2( int vtxIdx ) const
    {
        return const_cast<VertexBuffer*>(this)->getCacheTex2( vtxIdx );
    }

    Vector2* VertexBuffer::getCacheTex2( int vtxIdx )
    {
        return (Vector2*)_getCacheByEle(VFM_TEX2, vtxIdx);
    }

    const float* VertexBuffer::getCacheSH( int vtxIdx ) const
    {
        return const_cast<VertexBuffer*>(this)->getCacheSH( vtxIdx );
    }

    float* VertexBuffer::getCacheSH( int vtxIdx )
    {
        return (float*)_getCacheByEle(VFM_SH0, vtxIdx);
    }

    const uint32* VertexBuffer::getCacheColor( int vtxIdx ) const
    {
        return const_cast<VertexBuffer*>(this)->getCacheColor( vtxIdx );
    }

    uint32* VertexBuffer::getCacheColor( int vtxIdx )
    {
        return (uint32*)_getCacheByEle(VFM_CLR, vtxIdx);
    }

    void VertexBuffer::copyFrom( VertexBuffer* src )
    {
        // 本地还没有声明，那么使用源声明
        if ( !this->getDeclaration() )
            this->setDeclaration( src->getDeclaration() );

        // 检查容量
        const int vtxCnt = src->getVertexCount();

        if ( this->getVertexCount() != vtxCnt ) // 顶点数不匹配
        {
            // 释放旧的，并重构
            this->destroy();
            this->create( this->getDeclaration()->getStride() * vtxCnt, HBU_STATIC );
        }

        khaosAssert( vtxCnt == this->getVertexCount() );

        // lock，准备复制
        const uint8* dataSrc = (const uint8*) src->lock(HBA_READ);
        VertexDeclaration* vdSrc = src->getDeclaration();
        int strideSrc  = vdSrc->getStride();

        uint8* dataDest = (uint8*) this->lock( HBA_WRITE );
        VertexDeclaration* vdDest = this->getDeclaration();
        int strideDest = vdDest->getStride();

        // 基本上支持这些够了
        const VertexFixedMask vfmList[] = { VFM_POS, VFM_NOR, VFM_TEX, VFM_CLR, VFM_TAN, VFM_TEX2 };

        for ( int vi = 0; vi < vtxCnt; ++vi )
        {
            for ( int i = 0; i < KHAOS_ARRAY_SIZE(vfmList); ++i )
            {
                VertexFixedMask vfm = vfmList[i];

                const VertexElement* eleSrc  = vdSrc->findElement( vfm );
                const VertexElement* eleDest = vdDest->findElement( vfm );

                if ( eleSrc && eleDest ) // 源和目的都有，则复制
                {
                    const uint8* itemSrc  = dataSrc  + eleSrc->offset;
                    uint8*       itemDest = dataDest + eleDest->offset;

                    memcpy( itemDest, itemSrc, getVertexElementTypeBytes(eleSrc->type) );
                }
            }

            dataSrc  += strideSrc;
            dataDest += strideDest;
        }

        // unlock
        this->unlock();
        src->unlock();
    }

    //////////////////////////////////////////////////////////////////////////
    void IndexBuffer::copyFrom( IndexBuffer* src )
    {
        // 不一致就重建
        if ( this->m_indexType != src->m_indexType || this->getIndexCount() != src->getIndexCount() )
        {
            destroy();
            this->create( src->m_size, HBU_STATIC, src->m_indexType );
        }

        // 读取并填充
        vector<int>::type tmp( src->getIndexCount(), 0 ); // 按int来总是足够的
        src->readData( &tmp[0] );
        this->fillData( &tmp[0] );
    }

    void IndexBuffer::readData( vector<int>::type& data )
    {
        int idxCnt = getIndexCount();
        if ( idxCnt <= 0 )
        {
            data.clear();
            return;
        }

        data.resize( idxCnt );

        if ( m_indexType == IET_INDEX16 )
        {
            vector<ushort>::type data16;
            data16.resize( idxCnt );
            readData( (void*)&data16[0] );

            for ( int i = 0; i < idxCnt; ++i )
                data[i] = data16[i];
        }
        else
        {
            khaosAssert( m_indexType == IET_INDEX32 );
            readData( (void*)&data[0] );
        }
    }

    void IndexBuffer::readData( vector<ushort>::type& data )
    {
        int idxCnt = getIndexCount();

        if ( m_indexType != IET_INDEX16 || idxCnt <= 0 )
        {
            data.clear();
            return;
        }

        data.resize( idxCnt );
        readData( (void*)&data[0] );
    }

    int IndexBuffer::getCacheIndex( int i ) const
    {
        khaosAssert( 0 <= i && i < getIndexCount() );

        if ( m_indexType == IET_INDEX16 )
        {
            return *((uint16*)m_cache + i);
        }
        else 
        {
            khaosAssert( m_indexType == IET_INDEX32 );
            return *((int*)m_cache + i);
        }
    }

    void IndexBuffer::getCacheTriIndices( int face, int& v0, int& v1, int &v2 ) const
    {
        int startIdx = face * 3;

        v0 = getCacheIndex(startIdx);
        ++startIdx;

        v1 = getCacheIndex(startIdx);
        ++startIdx;

        v2 = getCacheIndex(startIdx);
    }

    void IndexBuffer::setCacheIndex( int i, int index )
    {
        khaosAssert( 0 <= i && i < getIndexCount() );

        if ( m_indexType == IET_INDEX16 )
        {
            *((uint16*)m_cache + i) = (uint16)index;
        }
        else 
        {
            khaosAssert( m_indexType == IET_INDEX32 );
            *((int*)m_cache + i) = index;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // VertexDeclaration
    void VertexDeclaration::nextElement( VertexFixedMask mask, VertexElementSemantic semantic, VertexElementType type, int offset, int index )
    {
        VertexElement ve;
        
        ve.semantic = semantic;
        ve.type = type;
        ve.offset = offset;
        ve.index = index;

        m_elements.push_back( ve );
        khaosAssert( m_eleMap.find(mask) == m_eleMap.end() );
        m_eleMap[mask] = (int)m_elements.size() - 1;
    }

    const VertexElement* VertexDeclaration::findElement( VertexFixedMask mask ) const
    {
        ElementMap::const_iterator it = m_eleMap.find( mask );
        if ( it != m_eleMap.end() )
            return &m_elements[it->second];
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    // VertexDeclarationManager
    VertexDeclarationManager* g_vertexDeclarationManager = 0;

    VertexDeclarationManager::VertexDeclarationManager()
    {
        khaosAssert( !g_vertexDeclarationManager );
        g_vertexDeclarationManager = this;
        _init();
    }

    VertexDeclarationManager::~VertexDeclarationManager()
    {
        _clear();
        g_vertexDeclarationManager = 0;
    }

    void VertexDeclarationManager::_init()
    {
        registerDeclaration( VertexPNGTT::ID, createVertexPNGTTDeclaration() );
        registerDeclaration( VertexPNTT::ID, createVertexPNTTDeclaration() );
        registerDeclaration( VertexPNGT::ID, createVertexPNGTDeclaration() );
        registerDeclaration( VertexPNT::ID, createVertexPNTDeclaration() );
        registerDeclaration( VertexPTC::ID, createVertexPTCDeclaration() );
        registerDeclaration( VertexPT::ID, createVertexPTDeclaration() );
        registerDeclaration( VertexP::ID, createVertexPDeclaration() );
        
        registerDeclaration( VertexPNTCSH4::ID, createVertexPNTCSH4Declaration() );
        registerDeclaration( VertexPNTSH4::ID, createVertexPNTSH4Declaration() );
        registerDeclaration( VertexPNTSH3::ID, createVertexPNTSH3Declaration() );
    }

    void VertexDeclarationManager::_clear()
    {
        for ( DeclarationMap::iterator it = m_declMap.begin(), ite = m_declMap.end(); it != ite; ++it )
        {
            VertexDeclaration* vd = it->second;
            KHAOS_DELETE vd;
        }

        m_declMap.clear();
    }

    void VertexDeclarationManager::registerDeclaration( uint32 id, VertexDeclaration* vd )
    {
        bool b = m_declMap.insert( DeclarationMap::value_type(id, vd) ).second;
        khaosAssert(b);
    }

    VertexDeclaration* VertexDeclarationManager::getDeclaration( uint32 id ) const
    {
        DeclarationMap::const_iterator it = m_declMap.find( id );
        if ( it != m_declMap.end() )
            return it->second;
        khaosAssert(0);
        return 0;
    }
}

