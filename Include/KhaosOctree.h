#pragma once
#include "KhaosAxisAlignedBox.h"

namespace Khaos
{
    class SceneNode;

    class OctreeNode : public AllocatedObject
    {
    public:
        typedef SceneNode               ObjectType;
        typedef set<ObjectType*>::type  ObjectList;

    public:
        OctreeNode();
        ~OctreeNode();

    public:
        void addObject( ObjectType* obj );
        void removeObject( ObjectType* obj );
        void removeAllObjects();

        bool selfCanHold( const AxisAlignedBox &box, bool ignoreSize ) const;
        bool childCanHoldBySize( const AxisAlignedBox &box ) const;
        void getChildIndexes( const AxisAlignedBox &box, int *x, int *y, int *z ) const;
        
        void setBoundBox( const AxisAlignedBox& box );
        void setBoundBox( const Vector3& vmin, const Vector3& vmax );

        const AxisAlignedBox& getBoundBox() const { return m_box; }
        const AxisAlignedBox& getCullBoundBox() const { return m_boxCull; }

    private:
        void _updateCullBoundBox();

    public:
        // 节点父子关系
        OctreeNode* m_parent;

        union
        {
            OctreeNode* m_childrenList[8];
            OctreeNode* m_children[2][2][2];
        };

        // 此节点挂接的对象列表
        ObjectList m_objList;

    private:
        // 此节点区域
        AxisAlignedBox m_box;
        AxisAlignedBox m_boxCull;
    };

    //////////////////////////////////////////////////////////////////////////
    class Octree : public AllocatedObject
    {
    public:
        typedef OctreeNode::ObjectType ObjectType;

    public:
        Octree();
        ~Octree();

    public:
        void init( const AxisAlignedBox& box, int maxDepth );
        void clear();

        void updateMaxDepth( int maxDepth );
        void updateBox( const AxisAlignedBox& box );
        void updateObject( ObjectType* obj );

    private:
        void _addObjectToNode( OctreeNode* node, ObjectType* obj, int depth );
        void _removeObjectFromNode( ObjectType* obj );

    public:
        OctreeNode* m_root;
        int         m_maxDepth;
    };
}

