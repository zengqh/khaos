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
        // 初始化
        void setAreaRoot( SceneNode* root ) { m_areaRoot = root; }
        void initOctree( const AxisAlignedBox& box, int maxDepth );

        // 更新
        void updateMaxDepth( int maxDepth );
        void updateAreaBox( const AxisAlignedBox& box );
        void updateSceneNodeInOctree( SceneNode* node );
        
        // 获取
        const AxisAlignedBox& getCullBoundBox() const;

    public:
        // 查询
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
        // 该区域的根节点
        SceneNode* m_areaRoot;

        // 该区域的八叉树
        Octree* m_octree;
    };
}

