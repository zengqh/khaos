#include "KhaosPreHeaders.h"
#include "KhaosMesh.h"
#include "KhaosRenderDevice.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    SubMesh::SubMesh() : m_primType(PT_TRIANGLELIST) 
    {
        m_bvh = KHAOS_NEW AABBBVH;
        m_bvh->_init(this); 
    }

    SubMesh::~SubMesh()
    {
        KHAOS_DELETE m_bvh;
    }

    int SubMesh::getPrimitiveCount() const
    {
        return m_ib->getIndexCount() / getPrimitiveTypeVertexCount(m_primType);
    }

    int SubMesh::getVertexCount() const
    {
        return m_vb->getVertexCount();
    }

    VertexBuffer* SubMesh::createVertexBuffer()
    {
        if ( m_vb )
            return m_vb.get();

        m_vb.attach( g_renderDevice->createVertexBuffer() );
        return m_vb.get();
    }

    IndexBuffer* SubMesh::createIndexBuffer()
    {
        if ( m_ib )
            return m_ib.get();

        m_ib.attach( g_renderDevice->createIndexBuffer() );
        return m_ib.get();
    }

    void SubMesh::_expandVB( int maskAdd )
    {
        khaosAssert( m_vb );
        VertexDeclaration* vd = m_vb->getDeclaration();

        int newID = vd->getID() | maskAdd;
        if ( vd->getID() == newID ) // 已经是了
            return;

        // 创建并复制
        VertexDeclaration* vdNew = g_vertexDeclarationManager->getDeclaration(newID);
        VertexBuffer* vbNew = g_renderDevice->createVertexBuffer();

        vbNew->setDeclaration( vdNew );
        vbNew->copyFrom( m_vb.get() );

        // 设为新的
        m_vb.reset( vbNew );
    }

    void SubMesh::expandNormal()
    {
        _expandVB( VFM_NOR );
    }

    void SubMesh::expandTangent()
    {
        _expandVB( VFM_TAN );
    }

    void SubMesh::expandSH( int order )
    {
        khaosAssert( order == 2 || order == 3 || order == 4 );

        if ( order == 2 )
            _expandVB( VFM_SH0 | VFM_SH1 );
        else if ( order == 3 )
            _expandVB( VFM_SH0 | VFM_SH1 | VFM_SH2 );
        else if ( order == 4 )
            _expandVB( VFM_SH0 | VFM_SH1 | VFM_SH2 | VFM_SH3 );
    }

    void SubMesh::expandTex2()
    {
        _expandVB( VFM_TEX2 );
    }

    void SubMesh::_getVertexAdjList( IntListList& adjList )
    {
        // 获取每个点的邻接面列表
        adjList.resize( getVertexCount() );

        int primCount = getPrimitiveCount();

        for ( int f = 0; f < primCount; ++f )
        {
            int v0, v1, v2;
            m_ib->getCacheTriIndices( f, v0, v1, v2 );

            adjList[v0].push_back(f);
            adjList[v1].push_back(f);
            adjList[v2].push_back(f);
        }
    }

    Vector3 SubMesh::_getFaceNormal( int face ) const
    {
        // 得到第f个面的法线

        // 面的3个点
        Vector3* v0;
        Vector3* v1;
        Vector3* v2;

        const_cast<SubMesh*>(this)->getTriVertices( face, v0, v1, v2 );

        // 求法线，注意这里面是逆时针顺序
        Vector3 va = *v1 - *v0;
        Vector3 vb = *v2 - *v1;
        Vector3 vc = va.crossProduct( vb );

        return vc; //.normalisedCopy(); // 这里考虑面积因素不单位化
    }

    void SubMesh::generateNormals( bool forceUpdate, bool refundImm )
    {
        khaosAssert( m_vb );

        // 是否已经有法线了
        if ( m_vb->hasElement(VFM_NOR) && !forceUpdate )
            return;

        // 确保法线
        expandNormal();

        // 确保缓存
        cacheLocalData( true );

        // 获取每个点的邻接面列表
        IntListList adjList;
        _getVertexAdjList( adjList );

        // 遍历每个点
        int vtxCnt = getVertexCount();

        for ( int i = 0; i < vtxCnt; ++i )
        {
            // 计算该点的所有邻接面的法线和
            Vector3 n(Vector3::ZERO);

            const IntList& adjs = adjList[i];
            int adjCnt = (int)adjs.size();

            for ( int j = 0; j < adjCnt; ++j )
            {
                int faceIdx = adjs[j];
                n += _getFaceNormal( faceIdx );
            }

            *(m_vb->getCacheNormal(i)) = n.normalisedCopy();
        }

        // 立即上传到gpu?
        if ( refundImm )
            m_vb->refundLocalData();
    }

    void SubMesh::generateTangents( bool forceUpdate, bool refundImm )
    {
        khaosAssert( m_vb );

        // 如果没有法线，我们先创建法线
        generateNormals( false, false );

        // 是否已经有切线了
        if ( m_vb->hasElement(VFM_TAN) && !forceUpdate )
            return;

        // 确保切线
        expandTangent();

        // 确保缓存
        cacheLocalData( true );

        // 开始计算切线~~~~~~~~~~~~~
        // 计算每个面的切线
        int primCount   = getPrimitiveCount();
        int vertexCount = getVertexCount();
        
        vector<Vector3>::type faceTangents( primCount ); // 每个面的切线
        vector<Vector3>::type faceBinormals( primCount ); // 每个面的次法线
        IntListList vtxFaces( vertexCount ); // 每个顶点对应的邻接面列表

        for ( int i = 0; i < primCount; ++i )
        {
            // 该面3点索引
            int i0, i1, i2;
            m_ib->getCacheTriIndices( i, i0, i1, i2 );

            // 该面位置
            const Vector3& v0 = *(m_vb->getCachePos(i0));
            const Vector3& v1 = *(m_vb->getCachePos(i1));
            const Vector3& v2 = *(m_vb->getCachePos(i2));

            // 该面uv
            const Vector2& uv0 = *(m_vb->getCacheTex(i0)); // 在第一套uv
            const Vector2& uv1 = *(m_vb->getCacheTex(i1));
            const Vector2& uv2 = *(m_vb->getCacheTex(i2));

            // 第i个面的切线
            Math::calcTangent( v0, v1, v2, uv0, uv1, uv2, faceTangents[i], faceBinormals[i] );

            // 记录顶点使用的面
            vtxFaces[i0].push_back( i );
            vtxFaces[i1].push_back( i );
            vtxFaces[i2].push_back( i );
        }

        // 计算每个顶点的平均切线
        for ( int i = 0; i < vertexCount; ++i )
        {
            IntList& faces = vtxFaces[i]; // 该点邻接面
            const Vector3& vnormal = *(m_vb->getCacheNormal(i)); // 该点法线

            Vector3 tanget(Vector3::ZERO);
            Vector3 binormal(Vector3::ZERO);

            // 该点的所有邻接面的切线统计
            for ( size_t j = 0; j < faces.size(); ++j )
            {
                int fi = faces[j];
                tanget   += faceTangents[fi];
                binormal += faceBinormals[fi];
            }

            // 正交矫正
            *(m_vb->getCacheTanget(i)) = Math::gramSchmidtOrthogonalize( tanget, binormal, vnormal );
        }

        // 立即上传到gpu?
        if ( refundImm )
            m_vb->refundLocalData();
    }

    void SubMesh::updateAABB()
    {
        m_aabb.setNull();
     
        if ( !m_vb )
            return;

        if ( uint8* vb = (uint8*)m_vb->lock( HBA_READ ) )
        {
            //int posOffset = m_vb->getDeclaration()->findElement(VFM_POS)->offset;
            Vector3* pos = (Vector3*)(vb /*+ posOffset*/); // always 0
            m_aabb.merge( pos, m_vb->getVertexCount(), m_vb->getDeclaration()->getStride() );
            m_vb->unlock();
        }
    }

    void SubMesh::setAABB( const AxisAlignedBox& aabb )
    {
        m_aabb = aabb;
    }

    void SubMesh::draw()
    {
        g_renderDevice->setVertexBuffer( getVertexBuffer() );

        if ( IndexBuffer* ib = getIndexBuffer() )
        {
            g_renderDevice->setIndexBuffer( ib );
            g_renderDevice->drawIndexedPrimitive( getPrimitiveType(), 0, getPrimitiveCount() );
        }
        else
        {
            g_renderDevice->drawPrimitive( getPrimitiveType(), 0, getPrimitiveCount() );
        }
    }

    void SubMesh::cacheLocalData(  bool forceUpdate )
    {
        m_vb->cacheLocalData(forceUpdate);
        m_ib->cacheLocalData(forceUpdate);
    }

    void SubMesh::freeLocalData()
    {
        m_vb->freeLocalData();
        m_ib->freeLocalData();
    }

    void SubMesh::refundLocalData( bool vb, bool ib )
    {
        if ( vb )
            m_vb->refundLocalData();

        if ( ib )
            m_ib->refundLocalData();
    }

    void SubMesh::getTriVertices( int face, Vector3*& v0, Vector3*& v1, Vector3*& v2 )
    {
        int i0, i1, i2;
        m_ib->getCacheTriIndices( face, i0, i1, i2 );

        v0 = m_vb->getCachePos(i0);
        v1 = m_vb->getCachePos(i1);
        v2 = m_vb->getCachePos(i2);
    }

    Vector2 SubMesh::getTriUV( int face, const Vector3& gravity ) const
    {
        int i0, i1, i2;
        m_ib->getCacheTriIndices( face, i0, i1, i2 );

        const Vector2& uv0 = *(m_vb->getCacheTex(i0));
        const Vector2& uv1 = *(m_vb->getCacheTex(i1));
        const Vector2& uv2 = *(m_vb->getCacheTex(i2));

        return uv0 * gravity.x + uv1 * gravity.y + uv2 * gravity.z; 
    }

    Vector2 SubMesh::getTriUV2( int face, const Vector3& gravity ) const
    {
        int i0, i1, i2;
        m_ib->getCacheTriIndices( face, i0, i1, i2 );

        const Vector2& uv0 = *(m_vb->getCacheTex2(i0));
        const Vector2& uv1 = *(m_vb->getCacheTex2(i1));
        const Vector2& uv2 = *(m_vb->getCacheTex2(i2));

        return uv0 * gravity.x + uv1 * gravity.y + uv2 * gravity.z;
    }

    Vector3 SubMesh::getTriNormal( int face, const Vector3& gravity ) const
    {
        int i0, i1, i2;
        m_ib->getCacheTriIndices( face, i0, i1, i2 );

        const Vector3& norm0 = *(m_vb->getCacheNormal(i0));
        const Vector3& norm1 = *(m_vb->getCacheNormal(i1));
        const Vector3& norm2 = *(m_vb->getCacheNormal(i2));

        return norm0 * gravity.x + norm1 * gravity.y + norm2 * gravity.z;
    }

    void SubMesh::buildBVH( bool forceUpdate )
    {
        cacheLocalData( false ); // 最低限度调用
        m_bvh->build( forceUpdate );
    }

    void SubMesh::clearBVH()
    {
        m_bvh->clear();
    }

    AABBBVH::Result SubMesh::intersectDetail( const Ray& ray ) const
    {
        return m_bvh->intersect( ray );
    }

    AABBBVH::Result SubMesh::intersectDetail( const LimitRay& ray ) const
    {
        return m_bvh->intersect( ray );
    }

    void SubMesh::copyFrom( const SubMesh* rhs )
    {
        this->m_primType = rhs->m_primType;

        // copy vb
        this->m_vb.release();
        if ( rhs->m_vb )
        {
            VertexBuffer* vb = this->createVertexBuffer();
            vb->copyFrom( rhs->m_vb.get() );
        }
        
        // copy ib
        this->m_ib.release();
        if ( rhs->m_ib )
        {
            IndexBuffer* ib = this->createIndexBuffer();
            ib->copyFrom( rhs->m_ib.get() );
        }
        
        // misc
        this->m_aabb = rhs->m_aabb;
        this->m_materialName = rhs->m_materialName;

        this->m_bvh->clear(); // 不复制，总清空
    }

    //////////////////////////////////////////////////////////////////////////
    Mesh::~Mesh()
    {
        _destructResImpl();
    }

    SubMesh* Mesh::createSubMesh()
    {
        SubMesh* sm = KHAOS_NEW SubMesh;
        m_subMeshList.push_back( sm );
        return sm;
    }

    void Mesh::setMaterialName( const String& mtrName )
    {
        KHAOS_FOR_EACH( SubMeshList, m_subMeshList, it )
        {
            (*it)->setMaterialName( mtrName );
        }
    }

    void Mesh::updateAABB( bool forceAll )
    {
        m_aabb.setNull();

        for ( SubMeshList::iterator it = m_subMeshList.begin(), ite = m_subMeshList.end(); it != ite; ++it )
        {
            SubMesh* sm = *it;
            if ( forceAll )
                sm->updateAABB();
            m_aabb.merge( sm->getAABB() );
        }
    }

    void Mesh::setAABB( const AxisAlignedBox& aabb )
    {
        m_aabb = aabb;
    }

    void Mesh::expandSH( int order )
    {
        for ( SubMeshList::iterator it = m_subMeshList.begin(), ite = m_subMeshList.end(); it != ite; ++it )
        {
            SubMesh* sm = *it;
            sm->expandSH( order );
        }
    }

    void Mesh::expandTex2()
    {
        for ( SubMeshList::iterator it = m_subMeshList.begin(), ite = m_subMeshList.end(); it != ite; ++it )
        {
            SubMesh* sm = *it;
            sm->expandTex2();
        }
    }

    void Mesh::generateNormals( bool forceUpdate, bool refundImm )
    {
        for ( SubMeshList::iterator it = m_subMeshList.begin(), ite = m_subMeshList.end(); it != ite; ++it )
        {
            SubMesh* sm = *it;
            sm->generateNormals( forceUpdate, refundImm );
        }
    }

    void Mesh::generateTangents(  bool forceUpdate, bool refundImm )
    {
        for ( SubMeshList::iterator it = m_subMeshList.begin(), ite = m_subMeshList.end(); it != ite; ++it )
        {
            SubMesh* sm = *it;
            sm->generateTangents( forceUpdate, refundImm );
        }
    }

    void Mesh::cacheLocalData( bool forceUpdate )
    {
        for ( SubMeshList::iterator it = m_subMeshList.begin(), ite = m_subMeshList.end(); it != ite; ++it )
        {
            SubMesh* sm = *it;
            sm->cacheLocalData( forceUpdate );
        }
    }

    int Mesh::getVertexCount() const
    {
        int cnt = 0;

        for ( SubMeshList::const_iterator it = m_subMeshList.begin(), ite = m_subMeshList.end(); it != ite; ++it )
        {
            const SubMesh* sm = *it;
            cnt += sm->getVertexCount();
        }

        return cnt;
    }

    void Mesh::freeLocalData()
    {
        for ( SubMeshList::iterator it = m_subMeshList.begin(), ite = m_subMeshList.end(); it != ite; ++it )
        {
            SubMesh* sm = *it;
            sm->freeLocalData();
        }
    }

    void Mesh::buildBVH( bool forceUpdate )
    {
        for ( SubMeshList::iterator it = m_subMeshList.begin(), ite = m_subMeshList.end(); it != ite; ++it )
        {
            SubMesh* sm = *it;
            sm->buildBVH( forceUpdate );
        }
    }

    void Mesh::clearBVH()
    {
        for ( SubMeshList::iterator it = m_subMeshList.begin(), ite = m_subMeshList.end(); it != ite; ++it )
        {
            SubMesh* sm = *it;
            sm->clearBVH();
        }
    }

    template<class T>
    bool Mesh::_intersectBoundImpl( const T& ray, float* t ) const
    {
        std::pair<bool, float> ret = ray.intersects( m_aabb );
        if ( t && ret.first )
            *t = ret.second;
        return ret.first;
    }

    template<class T>
    bool Mesh::_intersectBoundMoreImpl( const T& ray, int* subIdx, float* t ) const
    {
        int cnt = (int)m_subMeshList.size();
        if ( cnt == 1 ) // 只有1个情况，优化成intersectBound
        {
            bool ret = _intersectBoundImpl( ray, t );
            if ( subIdx && ret )
                *subIdx = 0; // 1个肯定是0啦
            return ret;
        }

        if ( !ray.intersects(m_aabb).first )
            return false;

        int   sub  = -1;
        float dist = Math::POS_INFINITY;
        
        for ( int i = 0; i < cnt; ++i )
        {
            const SubMesh* sm = m_subMeshList[i];
            std::pair<bool, float> curr = ray.intersects( sm->getAABB() );

            if ( curr.first && curr.second < dist )
            {
                sub  = i;
                dist = curr.second;
            }
        }

        if ( sub == -1 )
            return false;

        if ( subIdx )  *subIdx = sub;
        if ( t )       *t = dist;

        return true;
    }

    template<class T>
    bool Mesh::_intersectDetailImpl( const T& ray, int* subIdx, int* faceIdx, float* t, Vector3* gravity ) const
    {
        if ( !ray.intersects(m_aabb).first )
            return false;

        AABBBVH::Result ret;
        int sub = -1;

        int cnt = (int)m_subMeshList.size();
        for ( int i = 0; i < cnt; ++i )
        {
            const SubMesh* sm = m_subMeshList[i];
            AABBBVH::Result curr = sm->intersectDetail( ray );

            if ( curr.face != -1 && curr.distance < ret.distance )
            {
                ret = curr;
                sub = i;
            }
        }

        if ( sub == -1 )
            return false;

        if ( subIdx )  *subIdx = sub;
        if ( faceIdx ) *faceIdx = ret.face;
        if ( t )       *t = ret.distance;
        if ( gravity ) *gravity = ret.gravity;

        return true;
    }

    bool Mesh::intersectBound( const Ray& ray, float* t ) const
    {
        return _intersectBoundImpl( ray, t );
    }

    bool Mesh::intersectBound( const LimitRay& ray, float* t ) const
    {
        return _intersectBoundImpl( ray, t );
    }

    bool Mesh::intersectBoundMore( const Ray& ray, int* subIdx, float* t ) const
    {
        return _intersectBoundMoreImpl( ray, subIdx, t );
    }

    bool Mesh::intersectBoundMore( const LimitRay& ray, int* subIdx, float* t ) const
    {
        return _intersectBoundMoreImpl( ray, subIdx, t );
    }

    bool Mesh::intersectDetail( const Ray& ray, int* subIdx, int* faceIdx, float* t, Vector3* gravity ) const
    {
        return _intersectDetailImpl( ray, subIdx, faceIdx, t, gravity );
    }

    bool Mesh::intersectDetail( const LimitRay& ray, int* subIdx, int* faceIdx, float* t, Vector3* gravity ) const
    {
        return _intersectDetailImpl( ray, subIdx, faceIdx, t, gravity );
    }

    void Mesh::_clearSubMesh()
    {
        for ( SubMeshList::iterator it = m_subMeshList.begin(), ite = m_subMeshList.end(); it != ite; ++it )
        {
            SubMesh* sm = *it;
            KHAOS_DELETE sm;
        }

        m_subMeshList.clear();
    }

    void Mesh::copyFrom( const Resource* rhs )
    {
        const Mesh* meshOth = static_cast<const Mesh*>(rhs);

        this->_clearSubMesh();

        for ( int subIdx = 0; subIdx < meshOth->getSubMeshCount(); ++subIdx )
        {
            SubMesh* sm = this->createSubMesh();
            SubMesh* smOth = meshOth->getSubMesh( subIdx );

            sm->copyFrom( smOth );
        }

        m_aabb = meshOth->m_aabb;
    }

    void Mesh::_destructResImpl()
    {
        _clearSubMesh();
    }

    void Mesh::drawSub( int i )
    {
        getSubMesh(i)->draw();
    }
}

