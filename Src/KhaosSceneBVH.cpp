#include "KhaosPreHeaders.h"
#include "KhaosSceneBVH.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void SBVHBox::mergeFirstPoint( const SBVHVec3& pt )
    {
        minPt = maxPt = pt;
    }

    void SBVHBox::mergeNextPoint( const SBVHVec3& pt )
    {
        minPt.makeFloor(pt);
        maxPt.makeCeil(pt);
    }

    bool SBVHBox::isEmpty() const
    {
        return minPt.x > maxPt.x || minPt.y > maxPt.y || minPt.z > maxPt.z;
    }

    SBVHBox SBVHBox::operator+( const SBVHBox& rhs ) const
    {
        if ( this->isEmpty() )
            return rhs;

        if ( rhs.isEmpty() )
            return *this;

        SBVHBox boxtmp(minPt, maxPt);

        boxtmp.minPt.makeFloor(rhs.minPt);
        boxtmp.maxPt.makeCeil(rhs.maxPt);

        return boxtmp;
    }

    void SBVHFaceInfo::buildOtherInfo()
    {
        // �����е�
        worldCenter = (worldVtx0 + worldVtx1 + worldVtx2) / 3.0f;

        // Ϊ���򻯵ķ���
#if 0
        worldEdge1 = worldVtx1 - worldVtx0;
        worldEdge2 = worldVtx2 - worldVtx0;
        worldNorm  = worldEdge1.crossProduct( worldEdge2 );
#else
        #define _vrset(v)   vectorSet((v).x, (v).y, (v).z, 0)

        vrVtx0 = _vrset( worldVtx0 );
        vrVtx1 = _vrset( worldVtx1 );
        vrVtx2 = _vrset( worldVtx2 );

        vrEdge1 = _vrset( worldVtx1 - worldVtx0 );
        vrEdge2 = _vrset( worldVtx2 - worldVtx0 );
        vrNorm  = vectorCross( vrEdge1, vrEdge2 );

        #undef _vrset
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    SceneBVHNode* SceneBVHNode::createNode()
    {
        return new SceneBVHNode;
    }

    void SceneBVHNode::deleteDerived( SceneBVHNode* root )
    {
        if ( root->leftChild )
            deleteDerived( root->leftChild );

        if ( root->rightChild )
            deleteDerived( root->rightChild );

        delete root;
    }

    SceneBVHNode::SceneBVHNode() : 
        boxGroup(KHAOS_NEW_T_ALIGN_16(SBVHAABBGroup)), leftChild(0), rightChild(0), faceGroup(0), beginFaceIndex(0), endFaceIndex(0) 
    {

    }

    SceneBVHNode::~SceneBVHNode()
    {
        if ( faceGroup )
            delete faceGroup;

        KHAOS_DELETE_T_ALIGN_16(boxGroup);
    }

    void SceneBVHNode::makeLeaf()
    {
        khaosAssert( !faceGroup );
        khaosAssert( !leftChild && !rightChild );
        faceGroup = new SBVHFaceGroupInfo;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    SceneBVH::QueryContext<T>::QueryContext( const T& ray_ ) : ray(ray_) 
    {
        #define _vrset(v)   vectorSet((v).x, (v).y, (v).z, 0)

        rayPos = _vrset( ray.getOrigin() );
        negDir = _vrset( -ray.getDirection() );

        originX = vectorSetFloat1( ray.getOrigin().x );
        originY = vectorSetFloat1( ray.getOrigin().y );
        originZ = vectorSetFloat1( ray.getOrigin().z );
        invDirX = vectorSetFloat1( ray.getOneOverDir().x );
        invDirY = vectorSetFloat1( ray.getOneOverDir().y );
        invDirZ = vectorSetFloat1( ray.getOneOverDir().z );

        #undef _vrset
    }

    SceneBVH::SceneBVH() : m_root(0)
    {
    }

    SceneBVH::~SceneBVH()
    {
        if ( m_root )
            SceneBVHNode::deleteDerived( m_root );

        _clearAllFaceInfos();
    }

    void SceneBVH::addSceneNode( MeshNode* node )
    {
        khaosAssert( std::find( m_nodes.begin(), m_nodes.end(), node ) == m_nodes.end() );
        m_nodes.push_back( node );
    }

    void SceneBVH::build()
    {
        // 1. ��������Ϣ����
        for ( size_t i = 0; i < m_nodes.size(); ++i )
        {
            SceneNode* node = m_nodes[i];
            if ( !node->isEnabled() )
                continue;

            if ( KHAOS_OBJECT_IS( node, MeshNode ) )
            {
                MeshNode* meshNode = static_cast<MeshNode*>(node);
                _buildMeshNode( meshNode, (int)i );
            }
        }

        // 2. ������
        m_root = SceneBVHNode::createNode();
        m_root->beginFaceIndex = 0;
        m_root->endFaceIndex = (int)m_faceInfos.size();

        _devideDerived( m_root, 0 );
    }

    void SceneBVH::_buildMeshNode( MeshNode* node, int instanceID )
    {
        Mesh* mesh = node->getMesh();
        const Matrix4& matWorld = node->getDerivedMatrix();

        // ����ÿ����ģ��
        for ( int i = 0; i < mesh->getSubMeshCount(); ++i )
        {
            SubMesh* subMesh = mesh->getSubMesh( i );

            // �Ǹ������μ���
            if ( subMesh->getPrimitiveType() == PT_TRIANGLELIST )
            {
                // ����ÿ����
                for ( int f = 0; f < subMesh->getPrimitiveCount(); ++f )
                {
                    // ��ȡ���������
                    Vector3* v0;
                    Vector3* v1;
                    Vector3* v2;

                    subMesh->getTriVertices( f, v0, v1, v2 );

                    // ����һ����
                    SBVHFaceInfo* fi = KHAOS_NEW_T_ALIGN_16(SBVHFaceInfo);

                    fi->instanceID = instanceID;
                    fi->subIndex = i;
                    fi->faceIdx = f;

                    fi->worldVtx0 = matWorld.transformAffine( *v0 );
                    fi->worldVtx1 = matWorld.transformAffine( *v1 );
                    fi->worldVtx2 = matWorld.transformAffine( *v2 );
                    
                    fi->buildOtherInfo();

                    m_faceInfos.push_back( fi );
                } // ÿ����
            } // �Ǹ�������
        } // ÿ����ģ��
    }

    void SceneBVH::_clearAllFaceInfos()
    {
        for ( size_t i = 0; i < m_faceInfos.size(); ++i )
        {
            SBVHFaceInfo* fi = m_faceInfos[i];
            KHAOS_DELETE_T_ALIGN_16(fi);
        }

        m_faceInfos.clear();
    }

    void SceneBVH::_devideDerived( SceneBVHNode* node, int level )
    {
        const int faceNumInNode = node->endFaceIndex - node->beginFaceIndex;

        // 0. �����������Ҷ�ӽڵ���
        if ( faceNumInNode <= 4 )
        {
            node->makeLeaf(); // ʹ�Լ���ΪҶ��

            for ( int i = 0; i < faceNumInNode; ++i ) // ��������
                node->faceGroup->faceIDs[i] = node->beginFaceIndex + i;

            SBVHBox box = _calcFaceGroupBound( node->beginFaceIndex, node->endFaceIndex ); // ��������İ�Χ��
            node->boxGroup->setBox( 0, box ); // Ҷ�ӽڵ�box�Ͳ���0��

            _outputDebugStr( "bvh leaf:[L%d] %d-%d\n", level, node->beginFaceIndex, node->endFaceIndex );
            return;
        }

        int   bestPlane    = -1;
        float bestAvg      = 0.f;
        float bestVariance = 0.f;

        // 1. ���������������󷽲�Դ���Ϊ�и���
        for ( int planeIdx = 0; planeIdx < 3; ++planeIdx )
        {
            float samplesAvg = 0.f; // ����ƽ��
            float samplesVariance = 0.f; // ��������

            // ����ƽ��ֵ
            for ( int tri = node->beginFaceIndex; tri < node->endFaceIndex; ++tri )
            {
                samplesAvg += m_faceInfos[tri]->worldCenter[planeIdx];
            }

            samplesAvg /= faceNumInNode;

            // ���㷽��
            for ( int tri = node->beginFaceIndex; tri < node->endFaceIndex; ++tri )
            {
                float val = m_faceInfos[tri]->worldCenter[planeIdx];
                samplesVariance += (val - samplesAvg) * (val - samplesAvg);
            }

            samplesVariance /= faceNumInNode;

            // ȡ�������ģ�˵����������з�
            if ( samplesVariance >= bestVariance )
            {
                bestPlane    = planeIdx;
                bestVariance = samplesVariance;
                bestAvg      = samplesAvg;
            }
        }

        // 2. ���࣬��С��ƽ��ֵ�ĸϵ���ߣ��Ѵ���ƽ��ֵ�ĸϵ��ұ�
        int currLeft  = node->beginFaceIndex - 1;
        int currRight = node->beginFaceIndex + faceNumInNode;

        while ( currLeft < currRight )
        {
            float val;
            
            // �������һ������ƽ��ֵ�ģ�����Ҫ���������ұ�
            do
            {
                ++currLeft;
                val = m_faceInfos[currLeft]->worldCenter[bestPlane];
            }
            while ( val < bestAvg && currLeft < currRight );

            // ���ұ���һ������ƽ��ֵ�ģ�����Ҫ�����������
            do
            {
                --currRight;
                val = m_faceInfos[currRight]->worldCenter[bestPlane];
            }
            while ( val >= bestAvg && currRight > 0 && currLeft < currRight );

            // ���������ҵ��˸��Լ��ŵ�������ͽ�����ǰ����ǽ���һ��
            if ( currLeft < currRight )
            {
                swapVal( m_faceInfos[currLeft], m_faceInfos[currRight] );
            }
        }

        // �������������������һ��
        if ( currLeft == node->endFaceIndex || currRight == node->beginFaceIndex )
        {
            currLeft = node->beginFaceIndex + (faceNumInNode / 2);
        }

        // 3. ���ˣ����ǿ��Եݹ黮��
        SceneBVHNode* nodeLeft = SceneBVHNode::createNode();
        nodeLeft->beginFaceIndex = node->beginFaceIndex;
        nodeLeft->endFaceIndex = currLeft;
        _devideDerived( nodeLeft, level+1 );

        SceneBVHNode* nodeRight = SceneBVHNode::createNode();
        nodeRight->beginFaceIndex = currLeft;
        nodeRight->endFaceIndex = node->endFaceIndex;
        _devideDerived( nodeRight, level+1 );
       
        // 4. ���֮�����ǾͿ��Թ��쵱ǰ�ڵ���
        khaosAssert( !node->faceGroup );
        node->leftChild = nodeLeft;
        node->rightChild = nodeRight;

        node->boxGroup->setBox( 0, nodeLeft->getSelfBox() ); //���ӵķ�Χ
        node->boxGroup->setBox( 1, nodeRight->getSelfBox() ); // �Һ��ӵķ�Χ
        node->boxGroup->setBox( 2, nodeLeft->boxGroup->getBox(0) ); // ���ӵ����ӷ�Χ
        node->boxGroup->setBox( 3, nodeLeft->boxGroup->getBox(1) ); // ���ӵ��Һ��ӷ�Χ
    }

    SBVHBox SceneBVH::_calcFaceGroupBound( int beginIdx, int endIdx ) const
    {
        khaosAssert( beginIdx <= endIdx );

        SBVHBox box;

        const SBVHFaceInfo* info = m_faceInfos[beginIdx];
        box.mergeFirstPoint( info->worldVtx0 );
        box.mergeNextPoint( info->worldVtx1 );
        box.mergeNextPoint( info->worldVtx2 );

        for ( int f = beginIdx+1; f < endIdx; ++f )
        {
            const SBVHFaceInfo* info = m_faceInfos[f];

            box.mergeNextPoint( info->worldVtx0 );
            box.mergeNextPoint( info->worldVtx1 );
            box.mergeNextPoint( info->worldVtx2 );
        }

        return box;
    }

    //////////////////////////////////////////////////////////////////////////
    bool SceneBVH::_rayIntersectsTri( const QueryContext<Ray>& context, const SBVHFaceInfo* info,
        float& t, float& u, float& v ) const
    {
#if 0 // cpp version
        const Ray& ray = context.ray;

        const SBVHVec3& v0 = info->worldVtx0;
        const SBVHVec3& v1 = info->worldVtx1;
        const SBVHVec3& v2 = info->worldVtx2;

        const SBVHVec3& e1 = info->worldEdge1;
        const SBVHVec3& e2 = info->worldEdge2;

        SBVHVec3 nd = -ray.getDirection();

        // ��D
        const SBVHVec3& e1_e2 = info->worldNorm;
        float   D     = e1_e2.dotProduct( nd );

        if ( Math::fabs(D) < 1e-4f )
            return false;

        float invD = 1.0f / D;

        // ��Dt, t
        SBVHVec3 q  = ray.getOrigin() - v0;
        float   Dt = e1_e2.dotProduct( q );
        t = Dt * invD;
        if ( t < 0 )
            return false;

        // ��Du, u
        SBVHVec3 q_e2 = q.crossProduct( e2 );
        float   Du   = q_e2.dotProduct( nd );
        u = Du * invD;

        if ( u < 0 || u > 1 )
            return false;

        // ��Dv, v
        Vector3 e1_q = e1.crossProduct( q );
        float   Dv   = e1_q.dotProduct( nd );
        v = Dv * invD;

        if ( v < 0 || u+v > 1 )
            return false;

        return true;

#else // sse version

        static const VectorRegister vrSmallNumber = { 1e-4f, 1e-4f, 1e-4f, 1e-4f };

        // ��D
        VectorRegister D = vectorDot3( info->vrNorm, context.negDir );
        if ( vectorCompareLTFloat1(D, vrSmallNumber) )
            return false;

        VectorRegister invD = vectorDivide( vectorOne(), D );

        // ��Dt, t
        VectorRegister q   = vectorSubtract( context.rayPos, info->vrVtx0 );
        VectorRegister Dt  = vectorDot3( info->vrNorm, q );
        VectorRegister vrt = vectorMultiply(Dt, invD);
        if ( vectorCompareLTFloat1(vrt, vectorZero()) )
            return false;

        // ��Du, u
        VectorRegister q_e2 = vectorCross( q, info->vrEdge2 );
        VectorRegister Du   = vectorDot3( q_e2, context.negDir );
        VectorRegister vru  = vectorMultiply( Du, invD );
        if ( vectorCompareLTFloat1(vru, vectorZero()) || vectorCompareGTFloat1(vru, vectorOne()) )
            return false;

        // ��Dv, v
        VectorRegister e1_q = vectorCross( info->vrEdge1, q );
        VectorRegister Dv   = vectorDot3( e1_q, context.negDir );
        VectorRegister vrv  = vectorMultiply( Dv, invD );
        VectorRegister vruv = vectorAdd( vru, vrv );
        if ( vectorCompareLTFloat1(vrv, vectorZero()) || vectorCompareGTFloat1(vruv, vectorOne()) )
            return false;

        vectorStoreFloat1( vrt, &t );
        vectorStoreFloat1( vru, &u );
        vectorStoreFloat1( vrv, &v );
        return true;
#endif
    }

    void SceneBVH::_rayIntersectsTriAlways( const QueryContext<Ray>& context, const SBVHFaceInfo* info,
        float& t, float& u, float& v ) const
    {
        static const VectorRegister vrSmallNumber = { 1e-4f, 1e-4f, 1e-4f, 1e-4f };

        // ��D
        VectorRegister D = vectorDot3( info->vrNorm, context.negDir );
        khaosAssert( !vectorCompareLTFloat1(D, vrSmallNumber) );

        VectorRegister invD = vectorDivide( vectorOne(), D );

        // ��Dt, t
        VectorRegister q   = vectorSubtract( context.rayPos, info->vrVtx0 );
        VectorRegister Dt  = vectorDot3( info->vrNorm, q );
        VectorRegister vrt = vectorMultiply(Dt, invD);
        khaosAssert( !vectorCompareLTFloat1(vrt, vectorZero()) );

        // ��Du, u
        VectorRegister q_e2 = vectorCross( q, info->vrEdge2 );
        VectorRegister Du   = vectorDot3( q_e2, context.negDir );
        VectorRegister vru  = vectorMultiply( Du, invD );
        khaosAssert( !(vectorCompareLTFloat1(vru, vectorZero()) || vectorCompareGTFloat1(vru, vectorOne())) );

        // ��Dv, v
        VectorRegister e1_q = vectorCross( info->vrEdge1, q );
        VectorRegister Dv   = vectorDot3( e1_q, context.negDir );
        VectorRegister vrv  = vectorMultiply( Dv, invD );
        VectorRegister vruv = vectorAdd( vru, vrv );
        khaosAssert ( !(vectorCompareLTFloat1(vrv, vectorZero()) || vectorCompareGTFloat1(vruv, vectorOne())) );

        vectorStoreFloat1( vrt, &t );
        vectorStoreFloat1( vru, &u );
        vectorStoreFloat1( vrv, &v );
    }

    template<class T>
    void SceneBVH::_calcIntersectPt( const SBVHFaceGroupInfo* faceGroup, const QueryContext<T>& context, Result& ret ) const
    {
        // ���غ������ཻ������棨û���򷵻�<-1, FLT_MAX>��
        KHAOS_ALIGN_16 float     t;        // ��ʱ���صľ���
        KHAOS_ALIGN_16 float     u, v;     // ��ʱ���ص���������

        for ( int it = 0; it < 4; ++it )
        {
            int faceID = faceGroup->faceIDs[it];

            if ( faceID != -1 )
            {
                const SBVHFaceInfo* info = m_faceInfos[faceID];

                if ( _rayIntersectsTri(context, info, t, u, v) )
                {
                    if ( t < ret.distance )
                    {
                        ret.faceID    = faceID;
                        ret.distance  = t;
                        ret.gravity.y = u;
                        ret.gravity.z = v;
                    }
                }
            }
        }
    }

    void SceneBVH::_checkNodeBoundBox( SceneBVHNode* node, const QueryContext<Ray>& context, float histroyMinTime, 
        SBVHVec4& hitTime, int boxHit[4] ) const
    {
        SBVHAABBGroup& boxGroup = *(node->boxGroup);
        
#define PLAIN_C 0
#if PLAIN_C
        const Ray& ray = context.ray;

        for( int boxIndex = 0; boxIndex < 4; ++boxIndex )
        {
            // 0: ��box��С�����ֵ
            SBVHVec4 boxMin( boxGroup.getMinX(boxIndex), boxGroup.getMinY(boxIndex), boxGroup.getMinZ(boxIndex), 0 );
            SBVHVec4 boxMax( boxGroup.getMaxX(boxIndex), boxGroup.getMaxY(boxIndex), boxGroup.getMaxZ(boxIndex), 0 );

            // 1: ����������box�������������루�������Ļ������Ǹ�����
            SBVHVec4 slab1 = (boxMin - ray.getOriginV4()) * ray.getOneOverDir();
            SBVHVec4 slab2 = (boxMax - ray.getOriginV4()) * ray.getOneOverDir();

            // 2: ָ��ÿһ�����飨��x, ��y, ��z������������С���ֵ
            SBVHVec4 slabMin( Math::minVal(slab1.x, slab2.x), Math::minVal(slab1.y, slab2.y), 
                Math::minVal(slab1.z, slab2.z), Math::minVal(slab1.w, slab2.w) );

            SBVHVec4 slabMax( Math::maxVal(slab1.x, slab2.x), Math::maxVal(slab1.y, slab2.y), 
                Math::maxVal(slab1.z, slab2.z), Math::maxVal(slab1.w, slab2.w) );

            // 3: ������������������㣬һ��Сһ����
            float minTime = slabMin.asVec3().maxValue(); // ÿһ�������е���Сֵ��ȡ���߱�֤��ȡ�Ľ�С�����ں�����
            float maxTime = slabMax.asVec3().minValue(); // ÿһ�������е����ֵ��ȡС�߱�֤��ȡ�Ľϴ󽻵��ں�����

            // 4: �����¼��С�Ľ���λ�ã����ܸ������򣩣�
            // �Լ��Ƿ��Ǹ�����Ļ��У������С�����֮ǰ��ʷ��ѯ�Ľ���λ��С����ô�п��ܣ�����û��Ҫ�ٲ�������ˣ�
            hitTime[boxIndex] = minTime;			
            boxHit[boxIndex] = (maxTime >= 0 && maxTime >= minTime && minTime < histroyMinTime) ? 0xFFFFFFFF : 0;
        }
#else
        // 0: �������߲���
        const VectorRegister currentHitTime	= vectorSetFloat1( histroyMinTime );

        // ����box����
        //if ( (uint32)&boxGroup %16 != 0 )
        //    _outputDebugStr( "bug1:%d\n", &boxGroup );

        //if ( (uint32)&boxGroup.minPts[0] %16 != 0 )
        //    _outputDebugStr( "bug2:%d\n", &boxGroup.minPts[0] );

        //if ( (uint32)boxGroup.minPts[0].ptr() %16 != 0 )
        //    _outputDebugStr( "bug3:%d\n", boxGroup.minPts[0].ptr() );

        const VectorRegister boxMinX		= vectorLoadAligned( boxGroup.minPts[0].ptr() );
        const VectorRegister boxMinY		= vectorLoadAligned( boxGroup.minPts[1].ptr() );
        const VectorRegister boxMinZ		= vectorLoadAligned( boxGroup.minPts[2].ptr() );
        const VectorRegister boxMaxX		= vectorLoadAligned( boxGroup.maxPts[0].ptr() );
        const VectorRegister boxMaxY		= vectorLoadAligned( boxGroup.maxPts[1].ptr() );
        const VectorRegister boxMaxZ		= vectorLoadAligned( boxGroup.maxPts[2].ptr() );

        // 1: ����������box�������������루�������Ļ������Ǹ�����
        const VectorRegister boxMinSlabX	= vectorMultiply( vectorSubtract( boxMinX, context.originX ), context.invDirX );
        const VectorRegister boxMinSlabY	= vectorMultiply( vectorSubtract( boxMinY, context.originY ), context.invDirY );
        const VectorRegister boxMinSlabZ	= vectorMultiply( vectorSubtract( boxMinZ, context.originZ ), context.invDirZ );		
        const VectorRegister boxMaxSlabX	= vectorMultiply( vectorSubtract( boxMaxX, context.originX ), context.invDirX );
        const VectorRegister boxMaxSlabY	= vectorMultiply( vectorSubtract( boxMaxY, context.originY ), context.invDirY );
        const VectorRegister boxMaxSlabZ	= vectorMultiply( vectorSubtract( boxMaxZ, context.originZ ), context.invDirZ );

        // 2: ָ��ÿһ�����飨��x, ��y, ��z������������С���ֵ
        const VectorRegister slabMinX		= vectorMin( boxMinSlabX, boxMaxSlabX );
        const VectorRegister slabMinY		= vectorMin( boxMinSlabY, boxMaxSlabY );
        const VectorRegister slabMinZ		= vectorMin( boxMinSlabZ, boxMaxSlabZ );
        const VectorRegister slabMaxX		= vectorMax( boxMinSlabX, boxMaxSlabX );
        const VectorRegister slabMaxY		= vectorMax( boxMinSlabY, boxMaxSlabY );
        const VectorRegister slabMaxZ		= vectorMax( boxMinSlabZ, boxMaxSlabZ );

        // 3: ������������������㣬һ��Сһ����
        const VectorRegister slabMinXY		= vectorMax( slabMinX , slabMinY );
        const VectorRegister minTime		= vectorMax( slabMinXY, slabMinZ );
        const VectorRegister slabMaxXY		= vectorMin( slabMaxX , slabMaxY );
        const VectorRegister maxTime		= vectorMin( slabMaxXY, slabMaxZ );

        // 4: �����¼��С�Ľ���λ�ã����ܸ������򣩣�
        // �Լ��Ƿ��Ǹ�����Ļ��У������С�����֮ǰ��ʷ��ѯ�Ľ���λ��С����ô�п��ܣ�����û��Ҫ�ٲ�������ˣ�	
        vectorStoreAligned( minTime, hitTime.ptr() );
        const VectorRegister outNodeHit		= vectorBitwiseAND( vectorCompareGE( maxTime, vectorZero() ), vectorCompareGE( maxTime, minTime ) );
        const VectorRegister closerNodeHit	= vectorBitwiseAND( outNodeHit, vectorCompareGT( currentHitTime, minTime ) );
        vectorStoreAligned( closerNodeHit, (float*)boxHit );
#endif
    }

    template<class T>
    void SceneBVH::_intersectDerivedPreCalculated( SceneBVHNode* node, const QueryContext<T>& ray, Result& ret, const SBVHVec4& hitTime, int boxHit[4] ) const
    {
        khaosAssert( node );

        // Ҷ�ӽڵ�
        if ( node->isLeaf() )
        {
            // ֱ�ӷ����Լ�����������
            _calcIntersectPt( node->faceGroup, ray, ret );
            return;
        }

        // �����������
        if ( boxHit[2] )
        {
            // ���ͬʱ�����Һ���
            if ( boxHit[3] )
            {
                // ���Ӹ���һЩ������ִ�����Ӳ���
                if ( hitTime.z < hitTime.w )
                {
                    _intersectDerived( node->leftChild, ray, ret );

                    // ����Һ��ӱȵ�ǰ�����������ô�������п��ܴ��ڸ����Ľ��
                    if ( hitTime.w < ret.distance )
                    {
                        _intersectDerived( node->rightChild, ray, ret );
                    }
                }
                else // �Һ��Ӹ���һЩ������ִ���Һ��Ӳ���
                {
                    _intersectDerived( node->rightChild, ray, ret );

                    // ������ӱȵ�ǰ�����������ô�������п��ܴ��ڸ����Ľ��
                    if ( hitTime.z < ret.distance )
                    {
                        _intersectDerived( node->leftChild, ray, ret );
                    }
                }
            }
            else // ֻ������
            {
                _intersectDerived( node->leftChild, ray, ret );
            }
        }
        else // û������
        {
            if ( boxHit[3] ) // ֻ���Һ���
            {			
                _intersectDerived( node->rightChild, ray, ret );					
            }
            else // No node was hit.
            {
                return;
            }
        }
    }

    template<class T>
    void SceneBVH::_intersectDerived( SceneBVHNode* node, const QueryContext<T>& ray, Result& ret ) const
    {
        khaosAssert( node );

        // Ҷ�ӽڵ�
        if ( node->isLeaf() )
        {
            // ֱ�ӷ����Լ�����������
            _calcIntersectPt( node->faceGroup, ray, ret );
            return;
        }

        // ͬʱ���ýڵ���ĸ����ӣ��ҵ����Һ��ӣ������ӵ����Һ��ӣ�
        KHAOS_ALIGN_16 SBVHVec4 hitTime;
        KHAOS_ALIGN_16 int	    boxHit[4];

        _checkNodeBoundBox( node, ray, ret.distance, hitTime, boxHit );

        // ������ӻ���
        if ( boxHit[0] )
        {
            // ����Һ���Ҳͬʱ����
            if ( boxHit[1] )
            {
                // ���ӱ��Һ��Ӹ��ӽ�һЩ�����ȴ������ӿ�
                if ( hitTime.x < hitTime.y )
                {
                    // ������Ϊ�Ѿ������ӵĽ���ˣ����Կ����Ż��õ�ǰ��Ϣ����
                    _intersectDerivedPreCalculated( node->leftChild, ray, ret, hitTime, boxHit );

                    // ����Һ��ӱȵ�ǰ�����������ô�п��ܻ���Ҫ��������
                    if ( hitTime.y < ret.distance )
                    {
                        _intersectDerived( node->rightChild, ray, ret );
                    }
                }
                else // ����Һ��ӱ����Ӹ��ӽ�һЩ�����ȴ����Һ���
                {
                    _intersectDerived( node->rightChild, ray, ret );

                    // ������ӱȵ�ǰ�����������ô�п��ܻ���Ҫ��������
                    if ( hitTime.x < ret.distance )
                    {
                        _intersectDerivedPreCalculated( node->leftChild, ray, ret, hitTime, boxHit );
                    }
                }
            }
            else // ֻ�����ӱ�����
            {
                _intersectDerivedPreCalculated( node->leftChild, ray, ret, hitTime, boxHit );
            }
        }
        else // ����û�л���
        {
            if ( boxHit[1] ) // ���Һ��ӱ�����
            {			
                _intersectDerived( node->rightChild, ray, ret );			
            }
            else // һ����û����
            {
                return;
            }
        }
    }

    template<class T>
    bool SceneBVH::_intersect( const T& ray, Result& ret ) const
    {
        khaosAssert( m_root );

        KHAOS_ALIGN_16 QueryContext<T> context(ray);
        ret.clear();
        
        _intersectDerived( m_root, context, ret );

        if ( ret.faceID != -1 )
        {
            ret.gravity.x = 1.0f - ret.gravity.y - ret.gravity.z;
            return true;
        }

        return false;
    }

    bool SceneBVH::intersect( const Ray& ray, Result& ret ) const
    {
        return _intersect<Ray>( ray, ret );
    }

    bool SceneBVH::intersect( const LimitRay& ray, Result& ret ) const
    {
        //return _intersect<LimitRay>( ray, ret );
        return false;
    }

    template<class T>
    void SceneBVH::_intersectTriAlways( const T& ray, int faceID, Result& ret ) const
    {
        KHAOS_ALIGN_16 QueryContext<T> context(ray);

        _rayIntersectsTriAlways( context, getFaceInfo(faceID), 
            ret.distance, ret.gravity.y, ret.gravity.z );
        
        ret.gravity.x = 1.0f - ret.gravity.y - ret.gravity.z;

        ret.faceID = faceID;
    }

    void SceneBVH::intersectTriAlways( const Ray& ray, int faceID, Result& ret ) const
    {
        _intersectTriAlways<Ray>( ray, faceID, ret );
    }

    void SceneBVH::intersectTriAlways( const LimitRay& ray, int faceID, Result& ret ) const
    {
        //_intersectTriAlways<LimitRay>( ray, faceID, ret );
    }

    void SceneBVH::_addSceneNodeForName( SceneNode* node )
    {
        m_nodes.push_back( node );
    }

    void SceneBVH::_completeAddSceneNodeForName()
    {
        for ( size_t i = 0; i < m_faceInfos.size(); ++i )
        {
            SBVHFaceInfo* fi = m_faceInfos[i];

            SceneNode* node = this->getSceneNode(fi->instanceID);
            const Matrix4& matWorld = node->getDerivedMatrix();

            MeshNode* meshNode = static_cast<MeshNode*>(node); // �ٶ���meshnode

            Vector3* v0;
            Vector3* v1;
            Vector3* v2;

            meshNode->getMesh()->getSubMesh(fi->subIndex)->getTriVertices( fi->faceIdx, v0, v1, v2 );

            fi->worldVtx0 = matWorld.transformAffine( *v0 );
            fi->worldVtx1 = matWorld.transformAffine( *v1 );
            fi->worldVtx2 = matWorld.transformAffine( *v2 );

            fi->buildOtherInfo();
        }
    }

    void SceneBVH::openFile( const String& file )
    {
        FILE* fp = FileLowAPI::open( file.c_str(), "rb" );

        // read node info
        int nodeCnt;
        FileLowAPI::readData( fp, nodeCnt );
        khaosAssert( m_nodeNames.empty() );
        m_nodes.clear(); // ensure clear

        for ( int i = 0; i < nodeCnt; ++i )
        {
            String nodeName = FileLowAPI::readString( fp );
            m_nodeNames.push_back( nodeName );
        }

        // read face info
        int faceCnt;
        FileLowAPI::readData( fp, faceCnt );
        khaosAssert( m_faceInfos.empty() );

        for ( int i = 0; i < faceCnt; ++i )
        {
            SBVHFaceInfo* fi = KHAOS_NEW_T_ALIGN_16(SBVHFaceInfo);
            FileLowAPI::readData( fp, &fi->instanceID, sizeof(int)*3 );
            m_faceInfos.push_back( fi );
        }

        FileLowAPI::close( fp );
    }

    void SceneBVH::saveFile( const String& file )
    {
        FILE* fp = FileLowAPI::open( file.c_str(), "wb" );

        // save node info
        int nodeCnt = (int)m_nodes.size();
        FileLowAPI::writeData( fp, nodeCnt );

        for ( int i = 0; i < nodeCnt; ++i )
        {
            FileLowAPI::writeString( fp, m_nodes[i]->getName() );
        }

        // save face info
        int faceCnt = (int)m_faceInfos.size();
        FileLowAPI::writeData( fp, faceCnt );

        for ( int i = 0; i < faceCnt; ++i )
        {
            FileLowAPI::writeData( fp, &m_faceInfos[i]->instanceID, sizeof(int)*3 );
        }

        FileLowAPI::close( fp );
    }
}

