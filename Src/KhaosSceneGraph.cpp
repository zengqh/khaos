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
        // �޳�ʧЧ��
        if ( m_sfo->cullDisabled )
        {
            if ( !node->isEnabled() )
                return 0;
        }

        // ���ͼ��
        int type = node->getType();
        khaosAssert( 0 <= type && type < KHAOS_ARRAY_SIZE(m_sfo->resultNodes) );
        SceneFindOption::FindCallbackFunc findFunc = m_sfo->resultNodes[type];
        if ( !findFunc )
            return 0;

        // Ԥ�޳�
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
        if ( SceneFindOption::FindCallbackFunc func = _preCheck(obj) ) // Ԥ���
        {
            if ( vis == AreaOctree::FULL ) // �Ѿ���ȫ�ɼ���
                func( m_sfo->context, obj ); // ֱ�Ӽ�����
            else if ( m_sfo->camera->testVisibility( obj->getWorldAABB() ) ) // ���ֿɼ���Ҫ�����ཻ
                func( m_sfo->context, obj ); // ������
        }

        return AreaOctree::CONTINUE;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    AreaOctree::Visibility _FindByRay<T>::onTestAABB( const AxisAlignedBox& box )
    {
        if ( static_cast<T*>(m_sfo->ray)->intersects(box).first ) // ת���������ͣ����ﲻ���麯��
            return AreaOctree::PARTIAL; // ���ǲ���
        return AreaOctree::NONE;
    }

    template<class T>
    AreaOctree::ActionResult _FindByRay<T>::onTestObject( Octree::ObjectType* obj, AreaOctree::Visibility vis )
    {
        if ( SceneFindOption::FindCallbackFunc func = _preCheck(obj) ) // Ԥ���
        {
            if ( static_cast<T*>(m_sfo->ray)->intersects(obj->getWorldAABB()).first )
                func( m_sfo->context, obj ); // ������
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
        // �Ƿ��Ѿ����ڽڵ�����
        if ( getSceneNode(name) )
        {
            khaosLogLn( KHL_L2, "Already exist scene node: %s", name.c_str() );
            return 0;
        }

        // �����µĳ����ڵ�
        SceneNode* snode = g_nodeFactory->createSceneNode( type );
        snode->_setName( name );
        
        // ע��
        // ע������
        khaosAssert( snode->getSceneGraph() == 0 );
        m_nodeMap.insert( NodeMap::value_type(snode->getName(), snode) );
        snode->_setSceneGraph( this );
        return snode;
    }

    AreaNode* SceneGraph::createRootNode( const String& name )
    {
        // ĿǰAreaNode���ܳ�Ϊ���ڵ�
        AreaNode* node = createSceneNode<AreaNode>( name );

        if ( node )
            m_roots.push_back( node );

        return node;
    }

    AreaNode* SceneGraph::getRootNode( const String& name ) const
    {
        // Ŀǰ��ʱ��������
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
        // ע������
        NodeMap::iterator it = m_nodeMap.find( name );
        khaosAssert( it != m_nodeMap.end() );
        SceneNode* node = it->second;
        node->_onDestroyBefore(); // ����ɾ��֮ǰ��׼������

        khaosAssert( node->getSceneGraph() == this );
        m_nodeMap.erase( it );
        node->_setSceneGraph( 0 );

        if ( KHAOS_OBJECT_TYPE(node) == KHAOS_CLASS_TYPE(AreaNode) )
        {
            NodeList::iterator it = std::find( m_roots.begin(), m_roots.end(), node );
            khaosAssert( it != m_roots.end() );
            m_roots.erase( it );
        }

        // ɾ���ڵ�
        // ע�������ӽڵ�û��ɾ���������ĸ��ӽڵ㶼����������ϵ
        g_nodeFactory->destroySceneNode( node );
    }

    void SceneGraph::destroySceneNode( SceneNode* node )
    {
        destroySceneNode( node->getName() );
    }

    void SceneGraph::destroyDerivedSceneNode( SceneNode* node )
    {
        // �ݹ�ɾ���ڵ�����������ӽڵ�
        // ��ɾ������
        Node::NodeList& children = node->_getChildren();
        while ( children.size() )
        {
            SceneNode* node = static_cast<SceneNode*>(*children.begin());
            destroyDerivedSceneNode( node );
        }

        // Ȼ�������Լ�
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
        // ɾ�����нڵ�
        while ( m_roots.size() )
        {
            destroyDerivedSceneNode( m_roots.back() );
        }

        // ɾ�����ڳ���ͼ��node
        while ( m_nodeMap.size() )
            destroyDerivedSceneNode( m_nodeMap.begin()->second );
    }

    SceneNode* SceneGraph::getSceneNode( const String& name ) const
    {
        // �������ַ���
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

