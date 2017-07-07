#include "KhaosPreHeaders.h"
#include "KhaosNodeFactory.h"
#include "KhaosAreaNode.h"
#include "KhaosMeshNode.h"
#include "KhaosLightNode.h"
#include "KhaosCameraNode.h"
#include "KhaosEnvProbeNode.h"
#include "KhaosVolumeProbeNode.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    NodeFactory* g_nodeFactory = 0;

    //////////////////////////////////////////////////////////////////////////
    NodeFactory::NodeFactory()
    {
        khaosAssert( !g_nodeFactory );
        g_nodeFactory = this;
        _init();
    }

    NodeFactory::~NodeFactory()
    {
        g_nodeFactory = 0;
    }

    SceneNode* NodeFactory::createSceneNode( ClassType type )
    {
        CreatorType fn = _findCreator( type );
        if ( fn )
            return fn();
        return 0;
    }

    void NodeFactory::destroySceneNode( SceneNode* node )
    {
        node->_destruct();
    }

    template<class T>
    SceneNode* NodeFactory::_createSceneNode()
    {
        return KHAOS_NEW T;
    }

    NodeFactory::CreatorType NodeFactory::_findCreator( ClassType type ) const
    {
        CreatorMap::const_iterator it = m_creatorMap.find(type);
        if ( it != m_creatorMap.end() )
            return it->second;
        return 0;
    }

    template<class T>
    void NodeFactory::_registerCreator()
    {
        if ( _findCreator(KHAOS_CLASS_TYPE(T)) )
        {
            khaosLogLn( KHL_L2, "Already exist creator: %s", KHAOS_CLASS_TO_NAME(T) );
            return;
        }

        m_creatorMap.insert( CreatorMap::value_type(KHAOS_CLASS_TYPE(T), _createSceneNode<T>) );
    }

    void NodeFactory::_init()
    {
        _registerCreator<SceneNode>();
        _registerCreator<AreaNode>();
        _registerCreator<MeshNode>();
        _registerCreator<LightNode>();
        _registerCreator<CameraNode>();
        _registerCreator<EnvProbeNode>();
        _registerCreator<VolumeProbeNode>();
    }
}

