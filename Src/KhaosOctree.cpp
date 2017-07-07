#include "KhaosPreHeaders.h"
#include "KhaosOctree.h"
#include "KhaosSceneNode.h"

namespace Khaos
{
    OctreeNode::OctreeNode() : m_parent(0)
    {
        memset( m_childrenList, 0, sizeof(m_childrenList) );
    }

    OctreeNode::~OctreeNode()
    {
        // 解除挂载关系
        removeAllObjects();

        // 删除子树
        for ( int i = 0; i < 8; ++i )
        {
            KHAOS_DELETE m_childrenList[i];
        }
    }

    void OctreeNode::addObject( ObjectType* obj )
    {
        // 挂接一个对象
        khaosAssert( m_objList.find(obj) == m_objList.end() );
        khaosAssert( obj->_getOctreeNode() == 0 );
        m_objList.insert( obj );
        obj->_setOctreeNode( this );
    }

    void OctreeNode::removeObject( ObjectType* obj )
    {
        // 卸载一个对象
        ObjectList::iterator it = m_objList.find(obj);
        khaosAssert( it != m_objList.end() );
        khaosAssert( obj->_getOctreeNode() == this );
        m_objList.erase( it );
        obj->_setOctreeNode( 0 );
    }

    void OctreeNode::removeAllObjects()
    {
        // 卸载所有对象
        for ( ObjectList::iterator it = m_objList.begin(), ite = m_objList.end(); it != ite; ++it )
            (*it)->_setOctreeNode(0);
        m_objList.clear();
    }

    bool OctreeNode::selfCanHold( const AxisAlignedBox &box, bool ignoreSize ) const
    {
        // 判断box是否能容纳到子节点，这里考虑中心点和大小
        // 松散的八叉树
        // 一个对象能插入的条件：中心点位于节点内，对象大小不超过节点范围

        // 对象无限大无法插入
        if ( box.isInfinite() )
            return false;

        // 空对象不能插入
        if ( box.isNull() )
            return false;

        // 检查中心点是否在自己内
        if ( !m_box.contains( box.getCenter() ) )
            return false;

        if ( ignoreSize )
        {
            // root节点会调用，只要对象不是无限大不关心大小
            return true;
        }

        //对象大小比自己小
        Vector3 boxSize = box.getSize();
        Vector3 selfSize = m_box.getSize();
        return (boxSize.x <= selfSize.x) && (boxSize.y <= selfSize.y) && (boxSize.z <= selfSize.z);
    }

    bool OctreeNode::childCanHoldBySize( const AxisAlignedBox &box ) const
    {
        // 判断box是否能容纳到子节点，只考虑大小，不考虑中心位置
        // 松散的八叉树
        // 一个对象能插入的条件：中心点位于节点内，对象大小不超过节点范围
        // 这里只考虑节点范围，不考虑中心

        // 对象无限大无法插入
        if ( box.isInfinite() )
            return false;

        // 空对象不能插入
        if ( box.isNull() )
            return false;

        // 对象大小要比孩子大小小
        Vector3 childSize = m_box.getHalfSize();
        Vector3 boxSize = box.getSize();
        return ((boxSize.x <= childSize.x) && (boxSize.y <= childSize.y) && (boxSize.z <= childSize.z));
    }

    void OctreeNode::getChildIndexes( const AxisAlignedBox &box, int *x, int *y, int *z ) const
    {
        // 假如一个aabb能插入，判断插入到自己的哪个孩子中
        Vector3 selfCenter = m_box.getCenter();
        Vector3 othCenter  = box.getCenter();

        *x = othCenter.x > selfCenter.x ? 1 : 0;
        *y = othCenter.y > selfCenter.y ? 1 : 0;
        *z = othCenter.z > selfCenter.z ? 1 : 0;
    }

    void OctreeNode::setBoundBox( const AxisAlignedBox& box )
    {
        m_box = box;
        _updateCullBoundBox();
    }

    void OctreeNode::setBoundBox( const Vector3& vmin, const Vector3& vmax )
    {
        m_box.setExtents( vmin, vmax );
        _updateCullBoundBox();
    }

    void OctreeNode::_updateCullBoundBox()
    {
        // 松散八叉树，这里向两边各扩大了50%
        Vector3 halfSize = m_box.getHalfSize();
        m_boxCull.setExtents( m_box.getMinimum() - halfSize, m_box.getMaximum() + halfSize );
    }

    //////////////////////////////////////////////////////////////////////////
    Octree::Octree() : m_root(0), m_maxDepth(0)
    {
    }

    Octree::~Octree()
    {
        clear();
    }

    void Octree::init( const AxisAlignedBox& box, int maxDepth )
    {
        updateBox( box );
        updateMaxDepth( maxDepth );
    }

    void Octree::clear()
    {
        KHAOS_SAFE_DELETE(m_root);
    }

    void Octree::updateMaxDepth( int maxDepth )
    {
        khaosAssert( maxDepth > 0 );
        if ( m_maxDepth == maxDepth )
            return;

        // 比原来浅
        if ( maxDepth < m_maxDepth )
        {
            // 删除原来的树，重建一把，反正不会经常调用
            AxisAlignedBox box = m_root->getBoundBox();
            KHAOS_DELETE m_root;
            m_root = KHAOS_NEW OctreeNode;
            m_root->setBoundBox( box );
        }

        m_maxDepth = maxDepth;
    }

    void Octree::updateBox( const AxisAlignedBox& box )
    {
        // 不应当经常调用，重建一把
        KHAOS_DELETE m_root;
        m_root = KHAOS_NEW OctreeNode;
        m_root->setBoundBox( box );
    }

    void Octree::_addObjectToNode( OctreeNode* node, ObjectType* obj, int depth )
    {
        // 将一个对象插入到八叉树
        // 注意要求对象中心至少在根节点范围内
        const AxisAlignedBox& objBox = obj->getWorldAABB();

        // 假如此八叉树节点区域大小是此对象2倍，也就是说可以继续插入到它的节点
        // 同时八叉树的深度没有过深
        if ( (depth < m_maxDepth) && node->childCanHoldBySize(objBox) )
        {
            // 位于哪个子节点
            int x, y, z;
            node->getChildIndexes( objBox, &x, &y, &z );

            // 被选中的子节点
            OctreeNode*& nodeChild = node->m_children[x][y][z];

            // 是否首次创建
            if ( !nodeChild )
            {
                // 创建子节点
                nodeChild = KHAOS_NEW OctreeNode;
                nodeChild->m_parent = node;

                // 推算子节点区域范围
                const Vector3& nodeMin = node->getBoundBox().getMinimum();
                const Vector3& nodeMax = node->getBoundBox().getMaximum();

                Vector3 childMin, childMax;

                if ( x == 0 )
                {
                    childMin.x = nodeMin.x;
                    childMax.x = (nodeMin.x + nodeMax.x) * 0.5f;
                }
                else
                {
                    childMin.x = (nodeMin.x + nodeMax.x) * 0.5f;
                    childMax.x = nodeMax.x;
                }

                if ( y == 0 )
                {
                    childMin.y = nodeMin.y;
                    childMax.y = (nodeMin.y + nodeMax.y) * 0.5f;
                }
                else
                {
                    childMin.y = (nodeMin.y + nodeMax.y) * 0.5f;
                    childMax.y = nodeMax.y;
                }

                if ( z == 0 )
                {
                    childMin.z = nodeMin.z;
                    childMax.z = (nodeMin.z + nodeMax.z) * 0.5f;
                }
                else
                {
                    childMin.z = (nodeMin.z + nodeMax.z) * 0.5f;
                    childMax.z = nodeMax.z;
                }

                nodeChild->setBoundBox( childMin, childMax );
            } // end of 首次创建

            // 继续测试是否加入子节点
            ++depth; // 加入子节点后深度增加1次
            _addObjectToNode( nodeChild, obj, depth );
        }
        else // 不能继续加入子节点，那么只能加入本节点
        {
            node->addObject( obj );
        }
    }

    void Octree::_removeObjectFromNode( ObjectType* obj )
    {
        // 将对象从八叉树节点移除
        OctreeNode * node = obj->_getOctreeNode();
        if ( node )
            node->removeObject( obj );
    }

    void Octree::updateObject( ObjectType* obj )
    {
        // 更新一个对象在八叉树上的挂载情况
        const AxisAlignedBox& objBox = obj->getWorldAABB();
        OctreeNode* node = obj->_getOctreeNode();

        // 首次挂载
        if ( !node )
        {
            // 根节点能容纳则递归插入
            if ( m_root->selfCanHold( objBox, true ) )
            {
                // 从八叉树根节点开始尝试插入
                _addObjectToNode( m_root, obj, 0 );
            }
            else // 只能放到根节点
            {
                m_root->addObject( obj );
            }

            return;
        }

        // 已经有了查看是否还在原节点区域
        if ( ! node->selfCanHold( objBox, false ) )
        {
            // 有变更，将对象从此八叉树节点移走
            _removeObjectFromNode( obj );

            // 重新插入
            // 根节点能容纳则递归插入
            if ( m_root->selfCanHold( objBox, true ) )
            {
                // 从八叉树根节点开始尝试插入
                _addObjectToNode( m_root, obj, 0 );
            }
            else // 只能放到根节点
            {
                m_root->addObject( obj );
            }
        }
    }

}

