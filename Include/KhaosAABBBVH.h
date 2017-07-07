#pragma once
#include "KhaosPlane.h"
#include "KhaosAxisAlignedBox.h"
#include "KhaosRay.h"
#include "KhaosVector2.h"

namespace Khaos
{
    class SubMesh;

    //////////////////////////////////////////////////////////////////////////
    class BVHTreeNode : public AllocatedObject
    {
    public:
        typedef vector<int>::type FaceList;

        BVHTreeNode() : m_plane(0), m_leftChild(0), m_rightChild(0), m_adjustAxis(0), m_adjustCenter(0) {}

        ~BVHTreeNode()
        {
            if ( m_plane )
                KHAOS_DELETE_T(m_plane);
        }

        void setBox( const AxisAlignedBox& box )
        {
            m_box = box;
        }

        void setPlane( const Plane& plane, int adjustAxis, float adjustCenter )
        {
            m_plane = KHAOS_NEW_T(Plane)(plane);
            m_adjustAxis = adjustAxis;
            m_adjustCenter = adjustCenter;
        }

        void addFace( int f )
        {
            m_faces.push_back( f );
        }

        void addFaceList( const FaceList& faces )
        {
            KHAOS_FOR_EACH_CONST( FaceList, faces, it )
            {
                addFace(*it);
            }
        }

        void createPosHalfSpace();
        void createNegHalfSpace();

    public:
        AxisAlignedBox  m_box;
        Plane*          m_plane;
        FaceList        m_faces;
        BVHTreeNode*    m_leftChild;
        BVHTreeNode*    m_rightChild;
        int             m_adjustAxis;
        float           m_adjustCenter;
    };

    //////////////////////////////////////////////////////////////////////////
    class AABBBVH : public AllocatedObject
    {
    public:
        struct Result
        {
            Result() : face(-1), distance(Math::POS_INFINITY) {}
            //Result( int f, float d ) : face(f), distance(d) {}
            //Result( int f, float d, const Vector3& g ) : face(f), distance(d), gravity(g) {}

            void clear()
            {
                face = -1;
                distance = Math::POS_INFINITY;
            }

            int     face;
            float   distance;
            Vector3 gravity;
        };

    public:
        AABBBVH() : m_owner(0), m_root(0) {}
        ~AABBBVH() { clear(); }

    public:
        void _init( SubMesh* sm ) { m_owner = sm; }
        void build( bool forceUpdate );
        void clear();

        Result intersect( const Ray& ray ) const;
        Result intersect( const LimitRay& ray ) const;

    private:
        void _devideDerived( BVHTreeNode* node, const BVHTreeNode::FaceList& faces, int level );

        void _calcDevidePlane( const AxisAlignedBox& box, const BVHTreeNode::FaceList& faces, Plane& plane, int& adjustAxis, float& adjustCenter ) const;

        int  _calcFacePlace( const Plane* plane, int face );

        void _deleteDerived( BVHTreeNode* node );

        template<class T>
        Result _calcIntersectPt( const BVHTreeNode::FaceList& faces, const T& ray ) const;
        
        template<class T>
        Result _intersect( BVHTreeNode* node, const T& ray ) const;
        
        template<class T>
        Result _intersect( const T& ray ) const;

    private:
        SubMesh*     m_owner;
        BVHTreeNode* m_root;
    };
}

