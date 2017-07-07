#include "KhaosPreHeaders.h"
#include "KhaosAABBBVH.h"
#include "KhaosMesh.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void BVHTreeNode::createPosHalfSpace()
    {
        if ( m_leftChild )
            return;

        const AxisAlignedBox& box = m_box;
        const Plane* plane = m_plane;

        BVHTreeNode* node = KHAOS_NEW BVHTreeNode;
        node->setBox( box );
        node->m_box.getMinimum()[m_adjustAxis] = m_adjustCenter;

        m_leftChild = node;
    }

    void BVHTreeNode::createNegHalfSpace()
    {
        if ( m_rightChild )
            return;

        const AxisAlignedBox& box = m_box;
        const Plane* plane = m_plane;

        BVHTreeNode* node = KHAOS_NEW BVHTreeNode;
        node->setBox( box );
        node->m_box.getMaximum()[m_adjustAxis] = m_adjustCenter;

        m_rightChild = node;
    }

    //////////////////////////////////////////////////////////////////////////
    void AABBBVH::build( bool forceUpdate )
    {
        if ( forceUpdate)
            clear();

        if ( m_root )
            return;

        // 创建root
        m_root = KHAOS_NEW BVHTreeNode;
        m_root->setBox( m_owner->getAABB() );

        // 创建划分
        BVHTreeNode::FaceList faces;

        int faceCnt = m_owner->getPrimitiveCount();
        for ( int f = 0; f < faceCnt; ++f )
            faces.push_back(f);

        _devideDerived( m_root, faces, 0 );
    }

    void AABBBVH::clear()
    {
        if ( m_root )
        {
            _deleteDerived( m_root );
            m_root = 0;
        }
    }

    void AABBBVH::_calcDevidePlane( const AxisAlignedBox& box, const BVHTreeNode::FaceList& faces,
        Plane& plane, int& adjustAxis, float& adjustCenter ) const
    {
        // 找到哪个边最长
        Vector3 size = box.getSize();

        adjustAxis = 2; // max = z

        if ( size.x > size.y )
        {
            if ( size.x > size.z ) // max = x
            {
                adjustAxis = 0;
            }
        }
        else
        {
            if ( size.y > size.z ) // max = y
            {
                adjustAxis = 1;
            }
        }

        // 在此边上找到中点
        const float minBound = box.getMinimum()[adjustAxis];
        const float maxBound = box.getMaximum()[adjustAxis];

#if 0
        double total = 0;

        for ( BVHTreeNode::FaceList::const_iterator it = faces.begin(), ite = faces.end(); it != ite; ++it )
        {
            int faceID = *it;

            Vector3* v0;
            Vector3* v1;
            Vector3* v2;
            m_owner->getTriVertices( faceID, v0, v1, v2 );

            total += Math::clamp( (*v0)[adjustAxis], minBound, maxBound );
            total += Math::clamp( (*v1)[adjustAxis], minBound, maxBound );
            total += Math::clamp( (*v2)[adjustAxis], minBound, maxBound );
        }

        adjustCenter = (float)(total / (faces.size() * 3));

        khaosAssert( adjustCenter >= minBound );
        khaosAssert( adjustCenter <= maxBound );
#else
        adjustCenter = (minBound + maxBound) * 0.5f;
#endif
        // 生成切割面
        static const Vector3 axis[3] = { Vector3::UNIT_X, Vector3::UNIT_Y, Vector3::UNIT_Z };

        Vector3 center = box.getCenter();
        center[adjustAxis] = adjustCenter; // 用统计的中心点取代aabb中点

        plane = Plane( axis[adjustAxis], center );
    }

    int AABBBVH::_calcFacePlace( const Plane* plane, int face )
    {
        Vector3* v0;
        Vector3* v1;
        Vector3* v2;

        m_owner->getTriVertices( face, v0, v1, v2 );
        return plane->getSide( *v0, *v1, *v2 );
    }

    void AABBBVH::_devideDerived( BVHTreeNode* node, const BVHTreeNode::FaceList& faces, int level )
    {
        const int MIN_BVH_FACES  = 30;
        const int MAX_BVH_LEVEL  = 20;
        const float MIN_BVH_SIZE  = 0.001f;

        Vector3 curBoxSize = node->m_box.getSize();
        float curBoxMaxSide = Math::maxVal( Math::maxVal(curBoxSize.x, curBoxSize.y), curBoxSize.z );

        if ( faces.size() <= MIN_BVH_FACES || level >= MAX_BVH_LEVEL || curBoxMaxSide <= MIN_BVH_SIZE )
        {
            // 满足面数要求，直接存入，不再划分
            node->addFaceList( faces );
            return;
        }

        // 可以分，确定分割平面
        if ( !node->m_plane )
        {
            Plane plane;
            int   axis;
            float center;
            _calcDevidePlane( node->m_box, faces, plane, axis, center );
            node->setPlane( plane, axis, center );
        }
        
        BVHTreeNode::FaceList facesLeft, facesRight;

        // 确定将面存放到该区域的哪个子空间中
        KHAOS_FOR_EACH_CONST( BVHTreeNode::FaceList, faces, it )
        {
            int face   = *it;
            int result = _calcFacePlace( node->m_plane, face );

            if ( result == Plane::BOTH_SIDE ) // 和划分平面交，就存放在本空间
            {
                node->addFace( face );
            }
            else if ( result == Plane::POSITIVE_SIDE ) // 正半空间
            {
                node->createPosHalfSpace(); // 还没有创建正半空间，那么创建它
                facesLeft.push_back( face );
            }
            else // 负半空间
            {
                khaosAssert( result == Plane::NEGATIVE_SIDE );
                node->createNegHalfSpace(); // 还没有创建负半空间，那么创建它
                facesRight.push_back( face );
            }
        }

        // 分两半加入
        if ( facesLeft.size() )
            _devideDerived( node->m_leftChild, facesLeft, level+1 );

        if ( facesRight.size() )
            _devideDerived( node->m_rightChild, facesRight, level+1 );
      
    }

    template<class T>
    AABBBVH::Result AABBBVH::_calcIntersectPt( const BVHTreeNode::FaceList& faces, const T& ray ) const
    {
        // 返回和射线相交最近的面（没有则返回<-1, FLT_MAX>）
        Result ret;
        
        Vector3* v0;
        Vector3* v1;
        Vector3* v2;

        float     t;        // 临时返回的距离
        Vector3   graRet;   // 临时返回的重心坐标

        int cnt = (int)faces.size();

        for ( int i = 0; i < cnt; ++i )
        {
            int face = faces[i];
            m_owner->getTriVertices( face, v0, v1, v2 );

            if ( ray.intersects( *v0, *v1, *v2, &t, &graRet ) )
            {
                if ( t < ret.distance )
                {
                    ret.face     = face;
                    ret.distance = t;
                    ret.gravity  = graRet;
                }
            }
        }

        return ret;
    }

    template<class T>
    AABBBVH::Result AABBBVH::_intersect( BVHTreeNode* node, const T& ray ) const
    {
        // 没有划分平面了
        if ( !node->m_plane )
        {
            // 不可能有子空间，直接返回自己层的面计算结果
            khaosAssert( !node->m_leftChild && !node->m_rightChild );
            return _calcIntersectPt( node->m_faces, ray );
        }

        // 先计算自己层的一些面
        Result selfRet = _calcIntersectPt( node->m_faces, ray );

        // 优先查找的顺序
        BVHTreeNode* firstNode = 0;
        BVHTreeNode* secondNode = 0;

        // 确定射线所在子空间
        bool isInPositiveSide = ray.getOrigin()[node->m_adjustAxis] > node->m_adjustCenter; // 超过中心点就在正空间
        float dot = ray.getDirection()[node->m_adjustAxis]; // plane normal dot ray dir
        bool sideOthInPositiveSide;

        if ( isInPositiveSide ) // 正空间
        {
            firstNode = node->m_leftChild; // 正空间优先
            if ( dot < 0 ) // 射线负方向可能有负空间
            {
                secondNode = node->m_rightChild;
                sideOthInPositiveSide = false; // 有限条件，在负空间
            }
        }
        else // 负空间
        {
            firstNode = node->m_rightChild; // 负空间优先
            if ( dot > 0 ) // 射线正方向可能有正空间
            {   
                secondNode = node->m_leftChild;
                sideOthInPositiveSide = true; // 有限条件，在正空间
            }
        }

        // 判断第1子空间
        if ( firstNode )
        {
            Result subRet = _intersect( firstNode, ray );
            if ( subRet.face != -1 ) // 如果第1子空间有交点那么不用再考虑第2子空间了
            {
                if ( subRet.distance < selfRet.distance ) // 跟中介的比较取近的
                    return subRet;
                else
                    return selfRet;
            }
        }

        // 判断第2子空间
        if ( secondNode )
        {
            if ( std::is_same<T, LimitRay>::value ) // 如果是限制射线
            {
                // 做一个优化，判断端点是否在第二空间
                const LimitRay& rayTmp = static_cast<const LimitRay&>(ray); // 这个转换只为编译过
                bool side = rayTmp.getEndPoint()[node->m_adjustAxis] > node->m_adjustCenter;
                if ( side != sideOthInPositiveSide ) // 不满足对边条件
                    secondNode = 0; // 不用查第二空间了
            }
        }

        if ( secondNode )
        {
            Result subRet = _intersect( secondNode, ray );
            if ( subRet.face != -1 ) 
            {
                if ( subRet.distance < selfRet.distance ) // 跟中介的比较取近的
                    return subRet;
                else
                    return selfRet;
            }
        }

        return selfRet;
    }

    template<class T>
    AABBBVH::Result AABBBVH::_intersect( const T& ray ) const
    {
        khaosAssert( m_root );

        // 先和最外层aabb比较，不相交直接返回
        if ( !ray.intersects(m_root->m_box).first )
            return Result();

        return _intersect( m_root, ray );
    }

    AABBBVH::Result AABBBVH::intersect( const Ray& ray ) const
    {
        return _intersect(ray);
    }

    AABBBVH::Result AABBBVH::intersect( const LimitRay& ray ) const
    {
        return _intersect(ray);
    }

    void AABBBVH::_deleteDerived( BVHTreeNode* node )
    {
        if ( !node )
            return;

        _deleteDerived( node->m_leftChild );
        _deleteDerived( node->m_rightChild );

        KHAOS_DELETE node;
    }
}

