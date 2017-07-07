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

        // ����root
        m_root = KHAOS_NEW BVHTreeNode;
        m_root->setBox( m_owner->getAABB() );

        // ��������
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
        // �ҵ��ĸ����
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

        // �ڴ˱����ҵ��е�
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
        // �����и���
        static const Vector3 axis[3] = { Vector3::UNIT_X, Vector3::UNIT_Y, Vector3::UNIT_Z };

        Vector3 center = box.getCenter();
        center[adjustAxis] = adjustCenter; // ��ͳ�Ƶ����ĵ�ȡ��aabb�е�

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
            // ��������Ҫ��ֱ�Ӵ��룬���ٻ���
            node->addFaceList( faces );
            return;
        }

        // ���Է֣�ȷ���ָ�ƽ��
        if ( !node->m_plane )
        {
            Plane plane;
            int   axis;
            float center;
            _calcDevidePlane( node->m_box, faces, plane, axis, center );
            node->setPlane( plane, axis, center );
        }
        
        BVHTreeNode::FaceList facesLeft, facesRight;

        // ȷ�������ŵ���������ĸ��ӿռ���
        KHAOS_FOR_EACH_CONST( BVHTreeNode::FaceList, faces, it )
        {
            int face   = *it;
            int result = _calcFacePlace( node->m_plane, face );

            if ( result == Plane::BOTH_SIDE ) // �ͻ���ƽ�潻���ʹ���ڱ��ռ�
            {
                node->addFace( face );
            }
            else if ( result == Plane::POSITIVE_SIDE ) // ����ռ�
            {
                node->createPosHalfSpace(); // ��û�д�������ռ䣬��ô������
                facesLeft.push_back( face );
            }
            else // ����ռ�
            {
                khaosAssert( result == Plane::NEGATIVE_SIDE );
                node->createNegHalfSpace(); // ��û�д�������ռ䣬��ô������
                facesRight.push_back( face );
            }
        }

        // ���������
        if ( facesLeft.size() )
            _devideDerived( node->m_leftChild, facesLeft, level+1 );

        if ( facesRight.size() )
            _devideDerived( node->m_rightChild, facesRight, level+1 );
      
    }

    template<class T>
    AABBBVH::Result AABBBVH::_calcIntersectPt( const BVHTreeNode::FaceList& faces, const T& ray ) const
    {
        // ���غ������ཻ������棨û���򷵻�<-1, FLT_MAX>��
        Result ret;
        
        Vector3* v0;
        Vector3* v1;
        Vector3* v2;

        float     t;        // ��ʱ���صľ���
        Vector3   graRet;   // ��ʱ���ص���������

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
        // û�л���ƽ����
        if ( !node->m_plane )
        {
            // ���������ӿռ䣬ֱ�ӷ����Լ�����������
            khaosAssert( !node->m_leftChild && !node->m_rightChild );
            return _calcIntersectPt( node->m_faces, ray );
        }

        // �ȼ����Լ����һЩ��
        Result selfRet = _calcIntersectPt( node->m_faces, ray );

        // ���Ȳ��ҵ�˳��
        BVHTreeNode* firstNode = 0;
        BVHTreeNode* secondNode = 0;

        // ȷ�����������ӿռ�
        bool isInPositiveSide = ray.getOrigin()[node->m_adjustAxis] > node->m_adjustCenter; // �������ĵ�������ռ�
        float dot = ray.getDirection()[node->m_adjustAxis]; // plane normal dot ray dir
        bool sideOthInPositiveSide;

        if ( isInPositiveSide ) // ���ռ�
        {
            firstNode = node->m_leftChild; // ���ռ�����
            if ( dot < 0 ) // ���߸���������и��ռ�
            {
                secondNode = node->m_rightChild;
                sideOthInPositiveSide = false; // �����������ڸ��ռ�
            }
        }
        else // ���ռ�
        {
            firstNode = node->m_rightChild; // ���ռ�����
            if ( dot > 0 ) // ������������������ռ�
            {   
                secondNode = node->m_leftChild;
                sideOthInPositiveSide = true; // ���������������ռ�
            }
        }

        // �жϵ�1�ӿռ�
        if ( firstNode )
        {
            Result subRet = _intersect( firstNode, ray );
            if ( subRet.face != -1 ) // �����1�ӿռ��н�����ô�����ٿ��ǵ�2�ӿռ���
            {
                if ( subRet.distance < selfRet.distance ) // ���н�ıȽ�ȡ����
                    return subRet;
                else
                    return selfRet;
            }
        }

        // �жϵ�2�ӿռ�
        if ( secondNode )
        {
            if ( std::is_same<T, LimitRay>::value ) // �������������
            {
                // ��һ���Ż����ж϶˵��Ƿ��ڵڶ��ռ�
                const LimitRay& rayTmp = static_cast<const LimitRay&>(ray); // ���ת��ֻΪ�����
                bool side = rayTmp.getEndPoint()[node->m_adjustAxis] > node->m_adjustCenter;
                if ( side != sideOthInPositiveSide ) // ������Ա�����
                    secondNode = 0; // ���ò�ڶ��ռ���
            }
        }

        if ( secondNode )
        {
            Result subRet = _intersect( secondNode, ray );
            if ( subRet.face != -1 ) 
            {
                if ( subRet.distance < selfRet.distance ) // ���н�ıȽ�ȡ����
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

        // �Ⱥ������aabb�Ƚϣ����ֱཻ�ӷ���
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

