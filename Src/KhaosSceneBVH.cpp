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
        // 计算中点
        worldCenter = (worldVtx0 + worldVtx1 + worldVtx2) / 3.0f;

        // 为规则化的法线
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
        // 1. 建立面信息数据
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

        // 2. 构造树
        m_root = SceneBVHNode::createNode();
        m_root->beginFaceIndex = 0;
        m_root->endFaceIndex = (int)m_faceInfos.size();

        _devideDerived( m_root, 0 );
    }

    void SceneBVH::_buildMeshNode( MeshNode* node, int instanceID )
    {
        Mesh* mesh = node->getMesh();
        const Matrix4& matWorld = node->getDerivedMatrix();

        // 遍历每个子模型
        for ( int i = 0; i < mesh->getSubMeshCount(); ++i )
        {
            SubMesh* subMesh = mesh->getSubMesh( i );

            // 是个三角形几何
            if ( subMesh->getPrimitiveType() == PT_TRIANGLELIST )
            {
                // 遍历每个面
                for ( int f = 0; f < subMesh->getPrimitiveCount(); ++f )
                {
                    // 获取面的三个点
                    Vector3* v0;
                    Vector3* v1;
                    Vector3* v2;

                    subMesh->getTriVertices( f, v0, v1, v2 );

                    // 加入一个面
                    SBVHFaceInfo* fi = KHAOS_NEW_T_ALIGN_16(SBVHFaceInfo);

                    fi->instanceID = instanceID;
                    fi->subIndex = i;
                    fi->faceIdx = f;

                    fi->worldVtx0 = matWorld.transformAffine( *v0 );
                    fi->worldVtx1 = matWorld.transformAffine( *v1 );
                    fi->worldVtx2 = matWorld.transformAffine( *v2 );
                    
                    fi->buildOtherInfo();

                    m_faceInfos.push_back( fi );
                } // 每个面
            } // 是个三角形
        } // 每个子模型
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

        // 0. 数量不足就是叶子节点了
        if ( faceNumInNode <= 4 )
        {
            node->makeLeaf(); // 使自己成为叶子

            for ( int i = 0; i < faceNumInNode; ++i ) // 加入面组
                node->faceGroup->faceIDs[i] = node->beginFaceIndex + i;

            SBVHBox box = _calcFaceGroupBound( node->beginFaceIndex, node->endFaceIndex ); // 计算面组的包围盒
            node->boxGroup->setBox( 0, box ); // 叶子节点box就藏于0处

            _outputDebugStr( "bvh leaf:[L%d] %d-%d\n", level, node->beginFaceIndex, node->endFaceIndex );
            return;
        }

        int   bestPlane    = -1;
        float bestAvg      = 0.f;
        float bestVariance = 0.f;

        // 1. 计算三个轴向的最大方差，以此作为切割线
        for ( int planeIdx = 0; planeIdx < 3; ++planeIdx )
        {
            float samplesAvg = 0.f; // 样本平均
            float samplesVariance = 0.f; // 样本方差

            // 计算平均值
            for ( int tri = node->beginFaceIndex; tri < node->endFaceIndex; ++tri )
            {
                samplesAvg += m_faceInfos[tri]->worldCenter[planeIdx];
            }

            samplesAvg /= faceNumInNode;

            // 计算方差
            for ( int tri = node->beginFaceIndex; tri < node->endFaceIndex; ++tri )
            {
                float val = m_faceInfos[tri]->worldCenter[planeIdx];
                samplesVariance += (val - samplesAvg) * (val - samplesAvg);
            }

            samplesVariance /= faceNumInNode;

            // 取方差最大的（说明差异大）来切分
            if ( samplesVariance >= bestVariance )
            {
                bestPlane    = planeIdx;
                bestVariance = samplesVariance;
                bestAvg      = samplesAvg;
            }
        }

        // 2. 归类，把小于平均值的赶到左边，把大于平均值的赶到右边
        int currLeft  = node->beginFaceIndex - 1;
        int currRight = node->beginFaceIndex + faceNumInNode;

        while ( currLeft < currRight )
        {
            float val;
            
            // 从左边找一个超过平均值的，我们要把他赶向右边
            do
            {
                ++currLeft;
                val = m_faceInfos[currLeft]->worldCenter[bestPlane];
            }
            while ( val < bestAvg && currLeft < currRight );

            // 从右边找一个低于平均值的，我们要把他赶向左边
            do
            {
                --currRight;
                val = m_faceInfos[currRight]->worldCenter[bestPlane];
            }
            while ( val >= bestAvg && currRight > 0 && currLeft < currRight );

            // 现在我们找到了各自集团的两个叛徒，我们把他们交换一下
            if ( currLeft < currRight )
            {
                swapVal( m_faceInfos[currLeft], m_faceInfos[currRight] );
            }
        }

        // 极端情况，整个样本都一样
        if ( currLeft == node->endFaceIndex || currRight == node->beginFaceIndex )
        {
            currLeft = node->beginFaceIndex + (faceNumInNode / 2);
        }

        // 3. 好了，我们可以递归划分
        SceneBVHNode* nodeLeft = SceneBVHNode::createNode();
        nodeLeft->beginFaceIndex = node->beginFaceIndex;
        nodeLeft->endFaceIndex = currLeft;
        _devideDerived( nodeLeft, level+1 );

        SceneBVHNode* nodeRight = SceneBVHNode::createNode();
        nodeRight->beginFaceIndex = currLeft;
        nodeRight->endFaceIndex = node->endFaceIndex;
        _devideDerived( nodeRight, level+1 );
       
        // 4. 完成之后，我们就可以构造当前节点了
        khaosAssert( !node->faceGroup );
        node->leftChild = nodeLeft;
        node->rightChild = nodeRight;

        node->boxGroup->setBox( 0, nodeLeft->getSelfBox() ); //左孩子的范围
        node->boxGroup->setBox( 1, nodeRight->getSelfBox() ); // 右孩子的范围
        node->boxGroup->setBox( 2, nodeLeft->boxGroup->getBox(0) ); // 左孩子的左孩子范围
        node->boxGroup->setBox( 3, nodeLeft->boxGroup->getBox(1) ); // 左孩子的右孩子范围
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

        // 求D
        const SBVHVec3& e1_e2 = info->worldNorm;
        float   D     = e1_e2.dotProduct( nd );

        if ( Math::fabs(D) < 1e-4f )
            return false;

        float invD = 1.0f / D;

        // 求Dt, t
        SBVHVec3 q  = ray.getOrigin() - v0;
        float   Dt = e1_e2.dotProduct( q );
        t = Dt * invD;
        if ( t < 0 )
            return false;

        // 求Du, u
        SBVHVec3 q_e2 = q.crossProduct( e2 );
        float   Du   = q_e2.dotProduct( nd );
        u = Du * invD;

        if ( u < 0 || u > 1 )
            return false;

        // 求Dv, v
        Vector3 e1_q = e1.crossProduct( q );
        float   Dv   = e1_q.dotProduct( nd );
        v = Dv * invD;

        if ( v < 0 || u+v > 1 )
            return false;

        return true;

#else // sse version

        static const VectorRegister vrSmallNumber = { 1e-4f, 1e-4f, 1e-4f, 1e-4f };

        // 求D
        VectorRegister D = vectorDot3( info->vrNorm, context.negDir );
        if ( vectorCompareLTFloat1(D, vrSmallNumber) )
            return false;

        VectorRegister invD = vectorDivide( vectorOne(), D );

        // 求Dt, t
        VectorRegister q   = vectorSubtract( context.rayPos, info->vrVtx0 );
        VectorRegister Dt  = vectorDot3( info->vrNorm, q );
        VectorRegister vrt = vectorMultiply(Dt, invD);
        if ( vectorCompareLTFloat1(vrt, vectorZero()) )
            return false;

        // 求Du, u
        VectorRegister q_e2 = vectorCross( q, info->vrEdge2 );
        VectorRegister Du   = vectorDot3( q_e2, context.negDir );
        VectorRegister vru  = vectorMultiply( Du, invD );
        if ( vectorCompareLTFloat1(vru, vectorZero()) || vectorCompareGTFloat1(vru, vectorOne()) )
            return false;

        // 求Dv, v
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

        // 求D
        VectorRegister D = vectorDot3( info->vrNorm, context.negDir );
        khaosAssert( !vectorCompareLTFloat1(D, vrSmallNumber) );

        VectorRegister invD = vectorDivide( vectorOne(), D );

        // 求Dt, t
        VectorRegister q   = vectorSubtract( context.rayPos, info->vrVtx0 );
        VectorRegister Dt  = vectorDot3( info->vrNorm, q );
        VectorRegister vrt = vectorMultiply(Dt, invD);
        khaosAssert( !vectorCompareLTFloat1(vrt, vectorZero()) );

        // 求Du, u
        VectorRegister q_e2 = vectorCross( q, info->vrEdge2 );
        VectorRegister Du   = vectorDot3( q_e2, context.negDir );
        VectorRegister vru  = vectorMultiply( Du, invD );
        khaosAssert( !(vectorCompareLTFloat1(vru, vectorZero()) || vectorCompareGTFloat1(vru, vectorOne())) );

        // 求Dv, v
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
        // 返回和射线相交最近的面（没有则返回<-1, FLT_MAX>）
        KHAOS_ALIGN_16 float     t;        // 临时返回的距离
        KHAOS_ALIGN_16 float     u, v;     // 临时返回的重心坐标

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
            // 0: 该box最小最大数值
            SBVHVec4 boxMin( boxGroup.getMinX(boxIndex), boxGroup.getMinY(boxIndex), boxGroup.getMinZ(boxIndex), 0 );
            SBVHVec4 boxMax( boxGroup.getMaxX(boxIndex), boxGroup.getMaxY(boxIndex), boxGroup.getMaxZ(boxIndex), 0 );

            // 1: 计算射线与box几个面的有向距离（如果反向的话可能是负数）
            SBVHVec4 slab1 = (boxMin - ray.getOriginV4()) * ray.getOneOverDir();
            SBVHVec4 slab2 = (boxMax - ray.getOriginV4()) * ray.getOneOverDir();

            // 2: 指出每一对面组（±x, ±y, ±z）有向距离的最小最大值
            SBVHVec4 slabMin( Math::minVal(slab1.x, slab2.x), Math::minVal(slab1.y, slab2.y), 
                Math::minVal(slab1.z, slab2.z), Math::minVal(slab1.w, slab2.w) );

            SBVHVec4 slabMax( Math::maxVal(slab1.x, slab2.x), Math::maxVal(slab1.y, slab2.y), 
                Math::maxVal(slab1.z, slab2.z), Math::maxVal(slab1.w, slab2.w) );

            // 3: 射线与盒子有两个交点，一个小一个大
            float minTime = slabMin.asVec3().maxValue(); // 每一对面组中的最小值中取大者保证获取的较小交点在盒子内
            float maxTime = slabMax.asVec3().minValue(); // 每一对面组中的最大值中取小者保证获取的较大交点在盒子内

            // 4: 这里记录较小的交点位置（可能负数反向），
            // 以及是否是个合理的击中（如果较小交点比之前历史查询的交点位置小，那么有可能，否则没必要再查这盒子了）
            hitTime[boxIndex] = minTime;			
            boxHit[boxIndex] = (maxTime >= 0 && maxTime >= minTime && minTime < histroyMinTime) ? 0xFFFFFFFF : 0;
        }
#else
        // 0: 设置射线参数
        const VectorRegister currentHitTime	= vectorSetFloat1( histroyMinTime );

        // 设置box参数
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

        // 1: 计算射线与box几个面的有向距离（如果反向的话可能是负数）
        const VectorRegister boxMinSlabX	= vectorMultiply( vectorSubtract( boxMinX, context.originX ), context.invDirX );
        const VectorRegister boxMinSlabY	= vectorMultiply( vectorSubtract( boxMinY, context.originY ), context.invDirY );
        const VectorRegister boxMinSlabZ	= vectorMultiply( vectorSubtract( boxMinZ, context.originZ ), context.invDirZ );		
        const VectorRegister boxMaxSlabX	= vectorMultiply( vectorSubtract( boxMaxX, context.originX ), context.invDirX );
        const VectorRegister boxMaxSlabY	= vectorMultiply( vectorSubtract( boxMaxY, context.originY ), context.invDirY );
        const VectorRegister boxMaxSlabZ	= vectorMultiply( vectorSubtract( boxMaxZ, context.originZ ), context.invDirZ );

        // 2: 指出每一对面组（±x, ±y, ±z）有向距离的最小最大值
        const VectorRegister slabMinX		= vectorMin( boxMinSlabX, boxMaxSlabX );
        const VectorRegister slabMinY		= vectorMin( boxMinSlabY, boxMaxSlabY );
        const VectorRegister slabMinZ		= vectorMin( boxMinSlabZ, boxMaxSlabZ );
        const VectorRegister slabMaxX		= vectorMax( boxMinSlabX, boxMaxSlabX );
        const VectorRegister slabMaxY		= vectorMax( boxMinSlabY, boxMaxSlabY );
        const VectorRegister slabMaxZ		= vectorMax( boxMinSlabZ, boxMaxSlabZ );

        // 3: 射线与盒子有两个交点，一个小一个大
        const VectorRegister slabMinXY		= vectorMax( slabMinX , slabMinY );
        const VectorRegister minTime		= vectorMax( slabMinXY, slabMinZ );
        const VectorRegister slabMaxXY		= vectorMin( slabMaxX , slabMaxY );
        const VectorRegister maxTime		= vectorMin( slabMaxXY, slabMaxZ );

        // 4: 这里记录较小的交点位置（可能负数反向），
        // 以及是否是个合理的击中（如果较小交点比之前历史查询的交点位置小，那么有可能，否则没必要再查这盒子了）	
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

        // 叶子节点
        if ( node->isLeaf() )
        {
            // 直接返回自己层的面计算结果
            _calcIntersectPt( node->faceGroup, ray, ret );
            return;
        }

        // 如果击中左孩子
        if ( boxHit[2] )
        {
            // 如果同时击中右孩子
            if ( boxHit[3] )
            {
                // 左孩子更近一些，优先执行左孩子部分
                if ( hitTime.z < hitTime.w )
                {
                    _intersectDerived( node->leftChild, ray, ret );

                    // 如果右孩子比当前结果更近，那么它还是有可能存在更近的结果
                    if ( hitTime.w < ret.distance )
                    {
                        _intersectDerived( node->rightChild, ray, ret );
                    }
                }
                else // 右孩子更近一些，优先执行右孩子部分
                {
                    _intersectDerived( node->rightChild, ray, ret );

                    // 如果左孩子比当前结果更近，那么它还是有可能存在更近的结果
                    if ( hitTime.z < ret.distance )
                    {
                        _intersectDerived( node->leftChild, ray, ret );
                    }
                }
            }
            else // 只有左孩子
            {
                _intersectDerived( node->leftChild, ray, ret );
            }
        }
        else // 没有左孩子
        {
            if ( boxHit[3] ) // 只有右孩子
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

        // 叶子节点
        if ( node->isLeaf() )
        {
            // 直接返回自己层的面计算结果
            _calcIntersectPt( node->faceGroup, ray, ret );
            return;
        }

        // 同时检查该节点的四个盒子（我的左右孩子，和左孩子的左右孩子）
        KHAOS_ALIGN_16 SBVHVec4 hitTime;
        KHAOS_ALIGN_16 int	    boxHit[4];

        _checkNodeBoundBox( node, ray, ret.distance, hitTime, boxHit );

        // 如果左孩子击中
        if ( boxHit[0] )
        {
            // 如果右孩子也同时击中
            if ( boxHit[1] )
            {
                // 左孩子比右孩子更加近一些，优先处理左孩子咯
                if ( hitTime.x < hitTime.y )
                {
                    // 这里因为已经有左孩子的结果了，所以可以优化用当前信息传递
                    _intersectDerivedPreCalculated( node->leftChild, ray, ret, hitTime, boxHit );

                    // 如果右孩子比当前结果更近，那么有可能还需要继续查找
                    if ( hitTime.y < ret.distance )
                    {
                        _intersectDerived( node->rightChild, ray, ret );
                    }
                }
                else // 如果右孩子比左孩子更加近一些，优先处理右孩子
                {
                    _intersectDerived( node->rightChild, ray, ret );

                    // 如果左孩子比当前结果更近，那么有可能还需要继续查找
                    if ( hitTime.x < ret.distance )
                    {
                        _intersectDerivedPreCalculated( node->leftChild, ray, ret, hitTime, boxHit );
                    }
                }
            }
            else // 只有左孩子被击中
            {
                _intersectDerivedPreCalculated( node->leftChild, ray, ret, hitTime, boxHit );
            }
        }
        else // 左孩子没有击中
        {
            if ( boxHit[1] ) // 仅右孩子被击中
            {			
                _intersectDerived( node->rightChild, ray, ret );			
            }
            else // 一个都没击中
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

            MeshNode* meshNode = static_cast<MeshNode*>(node); // 假定是meshnode

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

