#include "KhaosPreHeaders.h"
#include "KhaosSceneGraph.h"
#include "KhaosNodeFactory.h"
#include "KhaosRay.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    // Scene Find Option
    _FindBaseOp::_FindBaseOp( SceneGraph* sg, SceneFindOption* sfo ) :
        m_sg(sg), m_sfo(sfo)
    {
    }

    SceneFindOption::FindCallbackFunc _FindBaseOp::_preCheck( SceneNode* node ) const
    {
        // 剔除失效的
        if ( m_sfo->cullDisabled )
        {
            if ( !node->isEnabled() )
                return 0;
        }

        // 类型检查
        int type = node->getType();
        khaosAssert( 0 <= type && type < KHAOS_ARRAY_SIZE(m_sfo->resultNodes) );
        SceneFindOption::FindCallbackFunc findFunc = m_sfo->resultNodes[type];
        if ( !findFunc )
            return 0;

        // 预剔除
        SceneFindOption::PreCullCallbackFunc preCull = m_sfo->preCull;
        if ( preCull && preCull(m_sfo->context, node) )
            return 0;

        return findFunc;
    }

    //////////////////////////////////////////////////////////////////////////
    AreaOctree::Visibility _FindByCamera::onTestAABB( const AxisAlignedBox& box )
    {
        return (AreaOctree::Visibility)m_sfo->camera->testVisibilityEx( box );
    }

    AreaOctree::ActionResult _FindByCamera::onTestObject( Octree::ObjectType* obj, AreaOctree::Visibility vis )
    {
        if ( SceneFindOption::FindCallbackFunc func = _preCheck(obj) ) // 预检查
        {
            if ( vis == AreaOctree::FULL ) // 已经完全可见了
                func( m_sfo->context, obj ); // 直接加入结果
            else if ( m_sfo->camera->testVisibility( obj->getWorldAABB() ) ) // 部分可见需要测试相交
                func( m_sfo->context, obj ); // 加入结果
        }

        return AreaOctree::CONTINUE;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    AreaOctree::Visibility _FindByRay<T>::onTestAABB( const AxisAlignedBox& box )
    {
        if ( static_cast<T*>(m_sfo->ray)->intersects(box).first ) // 转到射线类型，这里不搞虚函数
            return AreaOctree::PARTIAL; // 总是部分
        return AreaOctree::NONE;
    }

    template<class T>
    AreaOctree::ActionResult _FindByRay<T>::onTestObject( Octree::ObjectType* obj, AreaOctree::Visibility vis )
    {
        if ( SceneFindOption::FindCallbackFunc func = _preCheck(obj) ) // 预检查
        {
            if ( static_cast<T*>(m_sfo->ray)->intersects(obj->getWorldAABB()).first )
                func( m_sfo->context, obj ); // 加入结果
        }

        return AreaOctree::CONTINUE;
    }

    //////////////////////////////////////////////////////////////////////////
    // SceneGraph
    SceneGraph::SceneGraph() : m_bvh(0)
    {
    }

    SceneGraph::~SceneGraph()
    {
        destroyAllSceneNodes();
        KHAOS_DELETE m_bvh;
    }

    SceneNode* SceneGraph::createSceneNode( ClassType type, const String& name )
    {
        // 是否已经存在节点名字
        if ( getSceneNode(name) )
        {
            khaosLogLn( KHL_L2, "Already exist scene node: %s", name.c_str() );
            return 0;
        }

        // 创建新的场景节点
        SceneNode* snode = g_nodeFactory->createSceneNode( type );
        snode->_setName( name );
        
        // 注册
        // 注册名字
        khaosAssert( snode->getSceneGraph() == 0 );
        m_nodeMap.insert( NodeMap::value_type(snode->getName(), snode) );
        snode->_setSceneGraph( this );
        return snode;
    }

    AreaNode* SceneGraph::createRootNode( const String& name )
    {
        // 目前AreaNode才能成为根节点
        AreaNode* node = createSceneNode<AreaNode>( name );

        if ( node )
            m_roots.push_back( node );

        return node;
    }

    AreaNode* SceneGraph::getRootNode( const String& name ) const
    {
        // 目前暂时这样处理
        KHAOS_FOR_EACH_CONST( NodeList, m_roots, it )
        {
            SceneNode* node = *it;
            if ( node->getName() == name )
                return static_cast<AreaNode*>(node);
        }

        return 0;
    }

    void SceneGraph::destroySceneNode( const String& name )
    {
        // 注销名字
        NodeMap::iterator it = m_nodeMap.find( name );
        khaosAssert( it != m_nodeMap.end() );
        SceneNode* node = it->second;
        node->_onDestroyBefore(); // 给个删除之前的准备工作

        khaosAssert( node->getSceneGraph() == this );
        m_nodeMap.erase( it );
        node->_setSceneGraph( 0 );

        if ( KHAOS_OBJECT_TYPE(node) == KHAOS_CLASS_TYPE(AreaNode) )
        {
            NodeList::iterator it = std::find( m_roots.begin(), m_roots.end(), node );
            khaosAssert( it != m_roots.end() );
            m_roots.erase( it );
        }

        // 删除节点
        // 注意它的子节点没有删除，但它的父子节点都会和他脱离干系
        g_nodeFactory->destroySceneNode( node );
    }

    void SceneGraph::destroySceneNode( SceneNode* node )
    {
        destroySceneNode( node->getName() );
    }

    void SceneGraph::destroyDerivedSceneNode( SceneNode* node )
    {
        // 递归删除节点和他的所有子节点
        // 先删除孩子
        Node::NodeList& children = node->_getChildren();
        while ( children.size() )
        {
            SceneNode* node = static_cast<SceneNode*>(*children.begin());
            destroyDerivedSceneNode( node );
        }

        // 然后销毁自己
        destroySceneNode( node );
    }

    void SceneGraph::destroyDerivedSceneNode( const String& name )
    {
        NodeMap::iterator it = m_nodeMap.find( name );
        khaosAssert( it != m_nodeMap.end() );
        destroyDerivedSceneNode( it->second );
    }

    void SceneGraph::destroyAllSceneNodes()
    {
        // 删除所有节点
        while ( m_roots.size() )
        {
            destroyDerivedSceneNode( m_roots.back() );
        }

        // 删除不在场景图的node
        while ( m_nodeMap.size() )
            destroyDerivedSceneNode( m_nodeMap.begin()->second );
    }

    SceneNode* SceneGraph::getSceneNode( const String& name ) const
    {
        // 根据名字访问
        NodeMap::const_iterator it = m_nodeMap.find( name );
        if ( it != m_nodeMap.end() )
            return it->second;
        return 0;
    }

    void SceneGraph::find( SceneFindOption& sfo )
    {
        if ( sfo.option == SceneFindOption::OP_CAMERA_CULL )
        {
            _findByCamera( sfo );
        }
        else if ( sfo.option == SceneFindOption::OP_RAY_INTERSECT )
        {
            _findByRay<Ray>( sfo );
        }
        else if ( sfo.option == SceneFindOption::OP_LMRAY_INTERSECT )
        {
            _findByRay<LimitRay>( sfo );
        }
    }

    void SceneGraph::_findBase( AreaOctree::IFindObjectCallback* fn )
    {
        KHAOS_FOR_EACH( NodeList, m_roots, it )
        {
            khaosAssert( (*it)->getType() == NT_AREA );
            AreaNode* node = static_cast<AreaNode*>(*it);
            node->getArea()->findObject( fn );
        }
    }

    void SceneGraph::_findByCamera( SceneFindOption& sfo )
    {
        _FindByCamera func(this, &sfo);
        _findBase( &func );
    }

    template<class T>
    void SceneGraph::_findByRay( SceneFindOption& sfo )
    {
        _FindByRay<T> func(this, &sfo);
        _findBase( &func );
    }

    void SceneGraph::travel( ITravelSceneGraphCallback* trv )
    {
        KHAOS_FOR_EACH( NodeList, m_roots, it )
        {
            SceneNode* rootNode = *it;
            _travelNode(rootNode, trv);
        }
    }

    void SceneGraph::_travelNode( SceneNode* node, ITravelSceneGraphCallback* trv )
    {
        trv->onVisitNode(node);

        KHAOS_FOR_EACH ( SceneNode::NodeList, node->_getChildren(), it )
        {
            SceneNode* child = static_cast<SceneNode*>(*it);
            _travelNode( child, trv );
        }
    }

    void SceneGraph::update()
    {
        KHAOS_FOR_EACH( NodeList, m_roots, it )
        {
            SceneNode* snode = *it;
            snode->_updateDerived();
        }
    }

    void SceneGraph::initSceneBVH()
    {
        khaosAssert( !m_bvh );
        m_bvh = KHAOS_NEW SceneBVH;
    }

    //////////////////////////////////////////////////////////////////////////
    // SGFindBase
    void SGFindBase::setPreCull( SceneFindOption::PreCullCallbackFunc preCull )
    {
        m_sfo.preCull = preCull;
    }

    void SGFindBase::setResult( int type, SceneFindOption::FindCallbackFunc result )
    {
        khaosAssert( 0 <= type && type < KHAOS_ARRAY_SIZE(m_sfo.resultNodes) );
        m_sfo.resultNodes[type] = result;
    }

    void SGFindBase::setFinder( Camera* camera )
    {
        m_sfo.option = SceneFindOption::OP_CAMERA_CULL;
        m_sfo.camera = camera;
    }

    void SGFindBase::setFinder( Ray* ray )
    {
        m_sfo.option = SceneFindOption::OP_RAY_INTERSECT;
        m_sfo.ray = ray;
    }

    void SGFindBase::setFinder( LimitRay* ray )
    {
        m_sfo.option = SceneFindOption::OP_LMRAY_INTERSECT;
        m_sfo.ray = ray;
    }

    void SGFindBase::setNodeMask( uint32 nodeMask )
    {
        m_sfo.nodeMask = nodeMask;
    }

    void SGFindBase::setCullDisable( bool cullDisable )
    {
        m_sfo.cullDisabled = true;
    }

    void SGFindBase::setContext( void* context )
    {
        m_sfo.context = context;
    }

    void SGFindBase::find( SceneGraph* sg )
    {
        sg->find( m_sfo );
    }
}

