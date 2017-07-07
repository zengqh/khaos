#include "KhaosPreHeaders.h"
#include "KhaosSceneNode.h"
#include "KhaosAreaOctree.h"
#include "KhaosOctree.h"
#include "KhaosRenderSystem.h"
#include "KhaosLight.h"


namespace Khaos
{
    SceneNode::SceneNode() : m_sceneGraph(0), m_area(0), m_octNode(0)
    {
        m_type = NT_SCENE;
        m_dirtyFlag.setFlag( DF_WORLD_AABB | DF_OCTREE_POS );
        m_featureFlag.setFlag( SNODE_ENABLE );
    }

    void SceneNode::addChild( Node* node )
    {
        SceneNode* child = static_cast<SceneNode*>(node);

        // ��ͬһ������ͼ
        if ( child->m_sceneGraph != m_sceneGraph )
        {
            khaosAssert( 0 );
            khaosLogLn( KHL_L2, "Add child node: %s, scene graph not same", node->getName().c_str() );
            return;
        }

        // ��������Ǹ�����ڵ㣬��ô�Լ�����Ҳ�Ǹ�����ڵ�
        if ( child->_getPrivateArea() && !_getPrivateArea() )
        {
            khaosAssert( 0 );
            khaosLogLn( KHL_L2, "Add child node: %s, no private area", node->getName().c_str() );
            return;
        }

        Node::addChild(node);
    }

    void SceneNode::_onSelfAdd()
    {
        Node::_onSelfAdd();

        // Ӧ��û�м������������
        khaosAssert( m_area == 0 && m_octNode == 0 );

        // �������ӵ��˽��������Ȼ�Ҿ��ڸ��׵�˽�������У�
        // ���򣬸������ڵ�������Ϊ�����ڵ�����
        SceneNode* parent = static_cast<SceneNode*>(m_parent);
        m_area = parent->_getPrivateArea() ? parent->_getPrivateArea() : parent->m_area;
    }

    void SceneNode::_onSelfRemove()
    {
        Node::_onSelfRemove();

        // ���Լ����޳������Ƴ�
        if ( m_octNode )
            m_octNode->removeObject( this );

        // �˳��������
        m_area = 0;
    }

    void SceneNode::_setOctreeNode( OctreeNode* octNode ) 
    {
        m_octNode = octNode;
        if ( !octNode ) // �����޳����е�ʱ���������´�������λ��
            _setOctreePosDirty();
    }

    void SceneNode::_setSelfTransformDirty()
    {
        Node::_setSelfTransformDirty();

        // ��Ҫ����aabb��transform����aabb��
        _setWorldAABBDirty();
    }

    void SceneNode::_setParentTransformDirty()
    {
        Node::_setParentTransformDirty();

        // ��Ҫ����aabb��transform����aabb��
        _setWorldAABBDirty();
    }

    void SceneNode::_setWorldAABBDirty()
    {
        m_dirtyFlag.setFlag(DF_WORLD_AABB);
        _setOctreePosDirty(); // aabb����octree pos��
    }

    void SceneNode::_setOctreePosDirty()
    {
        m_dirtyFlag.setFlag(DF_OCTREE_POS);
    }

    const AxisAlignedBox& SceneNode::getWorldAABB() const
    {
        const_cast<SceneNode*>(this)->_checkUpdateWorldAABB();
        return m_worldAABB;
    }

    void SceneNode::_checkUpdateWorldAABB()
    {
        // world aabb����transform
        _checkUpdateTransform();

        // �ڵ����࣬���¼���AABB
        if ( m_dirtyFlag.testFlag(DF_WORLD_AABB) )
        {
            _makeWorldAABB();
            m_dirtyFlag.unsetFlag( DF_WORLD_AABB );
        }
    }

    void SceneNode::_checkUpdateOctreePos()
    {
        // ����λ������world aabb
        _checkUpdateWorldAABB();

        // �������޳����ϵ�λ��
        if ( m_dirtyFlag.testFlag(DF_OCTREE_POS) )
        {
            if ( m_area )
                m_area->updateSceneNodeInOctree( this );
            m_dirtyFlag.unsetFlag( DF_OCTREE_POS );
        }
    }

    void SceneNode::_update()
    {
        _checkUpdateOctreePos();
    }

    void SceneNode::onObjectAABBDirty()
    {
        _setWorldAABBDirty();
    }

    //////////////////////////////////////////////////////////////////////////
    RenderableSceneNode::RenderableSceneNode() :
        m_lightsInfo(0), m_lightsInfoUpdate(0)
    {
        m_type = NT_RENDER;
    }

    RenderableSceneNode::~RenderableSceneNode()
    {
        KHAOS_DELETE m_lightsInfo;
    }

    LightsInfo* RenderableSceneNode::getLightInfo()
    {
        // NB:���ñ�������aabb�����Ѿ��������

        if ( !m_lightsInfo )
            m_lightsInfo = KHAOS_NEW LightsInfo;

        uint32 sysFrameNo = g_renderSystem->_getCurrFrame();

        // ֻ��һ֡�ڸ���һ��
        if ( m_lightsInfoUpdate < sysFrameNo )
        {
            m_lightsInfoUpdate = sysFrameNo;

            g_renderSystem->queryLightsInfo( m_worldAABB, *m_lightsInfo, isReceiveShadow() );
        }

        return m_lightsInfo;
    }
}

