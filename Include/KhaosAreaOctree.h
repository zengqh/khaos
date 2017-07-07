#pragma once
#include "KhaosStdTypes.h"
#include "KhaosOctree.h"

namespace Khaos
{
    class AreaOctree : public AllocatedObject
    {
    public:
        AreaOctree();
        ~AreaOctree();

    public:
        // ��ʼ��
        void setAreaRoot( SceneNode* root ) { m_areaRoot = root; }
        void initOctree( const AxisAlignedBox& box, int maxDepth );

        // ����
        void updateMaxDepth( int maxDepth );
        void updateAreaBox( const AxisAlignedBox& box );
        void updateSceneNodeInOctree( SceneNode* node );
        
        // ��ȡ
        const AxisAlignedBox& getCullBoundBox() const;

    public:
        // ��ѯ
        enum Visibility
        {
            NONE,
            PARTIAL,
            FULL
        };

        enum ActionResult
        {
            CONTINUE,
            END
        };

        struct IFindObjectCallback
        {
            virtual Visibility   onTestAABB( const AxisAlignedBox& box ) = 0;
            virtual ActionResult onTestObject( Octree::ObjectType* obj, Visibility vis ) = 0;
        };

        ActionResult findObject( IFindObjectCallback* fn );

    private:
        ActionResult _findObject( OctreeNode* octNode, IFindObjectCallback* fn, Visibility parentVis );

    private:
        // ������ĸ��ڵ�
        SceneNode* m_areaRoot;

        // ������İ˲���
        Octree* m_octree;
    };
}

