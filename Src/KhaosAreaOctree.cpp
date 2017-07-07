#include "KhaosPreHeaders.h"
#include "KhaosAreaOctree.h"

namespace Khaos
{
    AreaOctree::AreaOctree() : m_areaRoot(0), m_octree(0)
    {
    }

    AreaOctree::~AreaOctree()
    {
        KHAOS_DELETE m_octree;
    }

    void AreaOctree::initOctree( const AxisAlignedBox& box, int maxDepth )
    {
        m_octree = KHAOS_NEW Octree;
        m_octree->init( box, maxDepth );
    }

    void AreaOctree::updateMaxDepth( int maxDepth )
    {
        m_octree->updateMaxDepth( maxDepth );
    }

    void AreaOctree::updateAreaBox( const AxisAlignedBox& box )
    {
        m_octree->updateBox( box );
    }

    void AreaOctree::updateSceneNodeInOctree( SceneNode* node )
    {
        m_octree->updateObject( node );
    }

    const AxisAlignedBox& AreaOctree::getCullBoundBox() const
    {
        return m_octree->m_root->getCullBoundBox();
    }

    AreaOctree::ActionResult AreaOctree::_findObject( OctreeNode* octNode, IFindObjectCallback* fn, Visibility parentVis )
    {
        // 测试此八叉树节点
        Visibility vis;
        if ( parentVis == FULL ) // 父节点已经完全可见
            vis = FULL; // 那么子节点也完全可见
        else // 否则需要测试
            vis = fn->onTestAABB( octNode->getCullBoundBox() );
        
        switch ( vis )
        {
        case NONE: // 不可见
            break;

        case PARTIAL: // 部分可见
        case FULL: // 完全可见
            {
                // 先遍历此节点上挂载的对象
                for ( OctreeNode::ObjectList::iterator it = octNode->m_objList.begin(), ite = octNode->m_objList.end();
                    it != ite; ++it )
                {
                    if ( fn->onTestObject( *it, vis ) == END )
                        return END; // 已经找到对象
                }

                // 继续测试子节点
                for ( int i = 0; i < 8; ++i )
                {
                    if ( OctreeNode* child = octNode->m_childrenList[i] )
                    {
                        if ( _findObject( child, fn, vis ) == END )
                            return END; // 已经找到对象
                    }
                }
            }
            break;
        }

        return CONTINUE; // 继续测试下个节点
    }

    AreaOctree::ActionResult AreaOctree::findObject( IFindObjectCallback* fn )
    {
        return _findObject( m_octree->m_root, fn, PARTIAL );
    }
}

