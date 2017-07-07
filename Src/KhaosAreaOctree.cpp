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
        // ���Դ˰˲����ڵ�
        Visibility vis;
        if ( parentVis == FULL ) // ���ڵ��Ѿ���ȫ�ɼ�
            vis = FULL; // ��ô�ӽڵ�Ҳ��ȫ�ɼ�
        else // ������Ҫ����
            vis = fn->onTestAABB( octNode->getCullBoundBox() );
        
        switch ( vis )
        {
        case NONE: // ���ɼ�
            break;

        case PARTIAL: // ���ֿɼ�
        case FULL: // ��ȫ�ɼ�
            {
                // �ȱ����˽ڵ��Ϲ��صĶ���
                for ( OctreeNode::ObjectList::iterator it = octNode->m_objList.begin(), ite = octNode->m_objList.end();
                    it != ite; ++it )
                {
                    if ( fn->onTestObject( *it, vis ) == END )
                        return END; // �Ѿ��ҵ�����
                }

                // ���������ӽڵ�
                for ( int i = 0; i < 8; ++i )
                {
                    if ( OctreeNode* child = octNode->m_childrenList[i] )
                    {
                        if ( _findObject( child, fn, vis ) == END )
                            return END; // �Ѿ��ҵ�����
                    }
                }
            }
            break;
        }

        return CONTINUE; // ���������¸��ڵ�
    }

    AreaOctree::ActionResult AreaOctree::findObject( IFindObjectCallback* fn )
    {
        return _findObject( m_octree->m_root, fn, PARTIAL );
    }
}

