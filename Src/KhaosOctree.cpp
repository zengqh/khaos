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
        // ������ع�ϵ
        removeAllObjects();

        // ɾ������
        for ( int i = 0; i < 8; ++i )
        {
            KHAOS_DELETE m_childrenList[i];
        }
    }

    void OctreeNode::addObject( ObjectType* obj )
    {
        // �ҽ�һ������
        khaosAssert( m_objList.find(obj) == m_objList.end() );
        khaosAssert( obj->_getOctreeNode() == 0 );
        m_objList.insert( obj );
        obj->_setOctreeNode( this );
    }

    void OctreeNode::removeObject( ObjectType* obj )
    {
        // ж��һ������
        ObjectList::iterator it = m_objList.find(obj);
        khaosAssert( it != m_objList.end() );
        khaosAssert( obj->_getOctreeNode() == this );
        m_objList.erase( it );
        obj->_setOctreeNode( 0 );
    }

    void OctreeNode::removeAllObjects()
    {
        // ж�����ж���
        for ( ObjectList::iterator it = m_objList.begin(), ite = m_objList.end(); it != ite; ++it )
            (*it)->_setOctreeNode(0);
        m_objList.clear();
    }

    bool OctreeNode::selfCanHold( const AxisAlignedBox &box, bool ignoreSize ) const
    {
        // �ж�box�Ƿ������ɵ��ӽڵ㣬���￼�����ĵ�ʹ�С
        // ��ɢ�İ˲���
        // һ�������ܲ�������������ĵ�λ�ڽڵ��ڣ������С�������ڵ㷶Χ

        // �������޴��޷�����
        if ( box.isInfinite() )
            return false;

        // �ն����ܲ���
        if ( box.isNull() )
            return false;

        // ������ĵ��Ƿ����Լ���
        if ( !m_box.contains( box.getCenter() ) )
            return false;

        if ( ignoreSize )
        {
            // root�ڵ����ã�ֻҪ���������޴󲻹��Ĵ�С
            return true;
        }

        //�����С���Լ�С
        Vector3 boxSize = box.getSize();
        Vector3 selfSize = m_box.getSize();
        return (boxSize.x <= selfSize.x) && (boxSize.y <= selfSize.y) && (boxSize.z <= selfSize.z);
    }

    bool OctreeNode::childCanHoldBySize( const AxisAlignedBox &box ) const
    {
        // �ж�box�Ƿ������ɵ��ӽڵ㣬ֻ���Ǵ�С������������λ��
        // ��ɢ�İ˲���
        // һ�������ܲ�������������ĵ�λ�ڽڵ��ڣ������С�������ڵ㷶Χ
        // ����ֻ���ǽڵ㷶Χ������������

        // �������޴��޷�����
        if ( box.isInfinite() )
            return false;

        // �ն����ܲ���
        if ( box.isNull() )
            return false;

        // �����СҪ�Ⱥ��Ӵ�СС
        Vector3 childSize = m_box.getHalfSize();
        Vector3 boxSize = box.getSize();
        return ((boxSize.x <= childSize.x) && (boxSize.y <= childSize.y) && (boxSize.z <= childSize.z));
    }

    void OctreeNode::getChildIndexes( const AxisAlignedBox &box, int *x, int *y, int *z ) const
    {
        // ����һ��aabb�ܲ��룬�жϲ��뵽�Լ����ĸ�������
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
        // ��ɢ�˲��������������߸�������50%
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

        // ��ԭ��ǳ
        if ( maxDepth < m_maxDepth )
        {
            // ɾ��ԭ���������ؽ�һ�ѣ��������ᾭ������
            AxisAlignedBox box = m_root->getBoundBox();
            KHAOS_DELETE m_root;
            m_root = KHAOS_NEW OctreeNode;
            m_root->setBoundBox( box );
        }

        m_maxDepth = maxDepth;
    }

    void Octree::updateBox( const AxisAlignedBox& box )
    {
        // ��Ӧ���������ã��ؽ�һ��
        KHAOS_DELETE m_root;
        m_root = KHAOS_NEW OctreeNode;
        m_root->setBoundBox( box );
    }

    void Octree::_addObjectToNode( OctreeNode* node, ObjectType* obj, int depth )
    {
        // ��һ��������뵽�˲���
        // ע��Ҫ��������������ڸ��ڵ㷶Χ��
        const AxisAlignedBox& objBox = obj->getWorldAABB();

        // ����˰˲����ڵ������С�Ǵ˶���2����Ҳ����˵���Լ������뵽���Ľڵ�
        // ͬʱ�˲��������û�й���
        if ( (depth < m_maxDepth) && node->childCanHoldBySize(objBox) )
        {
            // λ���ĸ��ӽڵ�
            int x, y, z;
            node->getChildIndexes( objBox, &x, &y, &z );

            // ��ѡ�е��ӽڵ�
            OctreeNode*& nodeChild = node->m_children[x][y][z];

            // �Ƿ��״δ���
            if ( !nodeChild )
            {
                // �����ӽڵ�
                nodeChild = KHAOS_NEW OctreeNode;
                nodeChild->m_parent = node;

                // �����ӽڵ�����Χ
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
            } // end of �״δ���

            // ���������Ƿ�����ӽڵ�
            ++depth; // �����ӽڵ���������1��
            _addObjectToNode( nodeChild, obj, depth );
        }
        else // ���ܼ��������ӽڵ㣬��ôֻ�ܼ��뱾�ڵ�
        {
            node->addObject( obj );
        }
    }

    void Octree::_removeObjectFromNode( ObjectType* obj )
    {
        // ������Ӱ˲����ڵ��Ƴ�
        OctreeNode * node = obj->_getOctreeNode();
        if ( node )
            node->removeObject( obj );
    }

    void Octree::updateObject( ObjectType* obj )
    {
        // ����һ�������ڰ˲����ϵĹ������
        const AxisAlignedBox& objBox = obj->getWorldAABB();
        OctreeNode* node = obj->_getOctreeNode();

        // �״ι���
        if ( !node )
        {
            // ���ڵ���������ݹ����
            if ( m_root->selfCanHold( objBox, true ) )
            {
                // �Ӱ˲������ڵ㿪ʼ���Բ���
                _addObjectToNode( m_root, obj, 0 );
            }
            else // ֻ�ܷŵ����ڵ�
            {
                m_root->addObject( obj );
            }

            return;
        }

        // �Ѿ����˲鿴�Ƿ���ԭ�ڵ�����
        if ( ! node->selfCanHold( objBox, false ) )
        {
            // �б����������Ӵ˰˲����ڵ�����
            _removeObjectFromNode( obj );

            // ���²���
            // ���ڵ���������ݹ����
            if ( m_root->selfCanHold( objBox, true ) )
            {
                // �Ӱ˲������ڵ㿪ʼ���Բ���
                _addObjectToNode( m_root, obj, 0 );
            }
            else // ֻ�ܷŵ����ڵ�
            {
                m_root->addObject( obj );
            }
        }
    }

}

