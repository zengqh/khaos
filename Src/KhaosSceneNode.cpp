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

        // 在同一个场景图
        if ( child->m_sceneGraph != m_sceneGraph )
        {
            khaosAssert( 0 );
            khaosLogLn( KHL_L2, "Add child node: %s, scene graph not same", node->getName().c_str() );
            return;
        }

        // 如果孩子是个区域节点，那么自己必须也是个区域节点
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

        // 应当没有加入过其他区域
        khaosAssert( m_area == 0 && m_octNode == 0 );

        // 如果父亲拥有私有区域，显然我就在父亲的私有区域中；
        // 否则，父亲所在的区域作为我所在的区域
        SceneNode* parent = static_cast<SceneNode*>(m_parent);
        m_area = parent->_getPrivateArea() ? parent->_getPrivateArea() : parent->m_area;
    }

    void SceneNode::_onSelfRemove()
    {
        Node::_onSelfRemove();

        // 将自己从剔除树中移除
        if ( m_octNode )
            m_octNode->removeObject( this );

        // 退出这个区域
        m_area = 0;
    }

    void SceneNode::_setOctreeNode( OctreeNode* octNode ) 
    {
        m_octNode = octNode;
        if ( !octNode ) // 不在剔除树中的时候总是在下次申请新位置
            _setOctreePosDirty();
    }

    void SceneNode::_setSelfTransformDirty()
    {
        Node::_setSelfTransformDirty();

        // 需要更新aabb，transform导致aabb脏
        _setWorldAABBDirty();
    }

    void SceneNode::_setParentTransformDirty()
    {
        Node::_setParentTransformDirty();

        // 需要更新aabb，transform导致aabb脏
        _setWorldAABBDirty();
    }

    void SceneNode::_setWorldAABBDirty()
    {
        m_dirtyFlag.setFlag(DF_WORLD_AABB);
        _setOctreePosDirty(); // aabb导致octree pos脏
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
        // world aabb依赖transform
        _checkUpdateTransform();

        // 节点有脏，重新计算AABB
        if ( m_dirtyFlag.testFlag(DF_WORLD_AABB) )
        {
            _makeWorldAABB();
            m_dirtyFlag.unsetFlag( DF_WORLD_AABB );
        }
    }

    void SceneNode::_checkUpdateOctreePos()
    {
        // 树上位置依赖world aabb
        _checkUpdateWorldAABB();

        // 更新在剔除树上的位置
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
        // NB:调用本函数，aabb必须已经计算完成

        if ( !m_lightsInfo )
            m_lightsInfo = KHAOS_NEW LightsInfo;

        uint32 sysFrameNo = g_renderSystem->_getCurrFrame();

        // 只在一帧内更新一次
        if ( m_lightsInfoUpdate < sysFrameNo )
        {
            m_lightsInfoUpdate = sysFrameNo;

            g_renderSystem->queryLightsInfo( m_worldAABB, *m_lightsInfo, isReceiveShadow() );
        }

        return m_lightsInfo;
    }
}

