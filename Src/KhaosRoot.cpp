#include "KhaosPreHeaders.h"
#include "KhaosRoot.h"
#include "KhaosTimer.h"
#include "KhaosNodeFactory.h"
#include "KhaosSysResManager.h"
#include "KhaosEffectContext.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    struct _PartSystem
    {
        static void create( const RootConfig& config )
        {
            KHAOS_NEW MsgQueueManager;
            KHAOS_NEW FileSystem;
            g_fileSystem->setFileSystem( KHAOS_NEW DefaultFileSystem );
            g_fileSystem->setRootPath( config.fileSystemPath );
        }

        static void start()
        {
            g_msgQueueManager->start();
            g_fileSystem->start();
        }

        static void stop()
        {
            g_fileSystem->shutdown();
            g_msgQueueManager->shutdown();
        }

        static void destroy()
        {
            KHAOS_DELETE g_fileSystem;
            KHAOS_DELETE g_msgQueueManager;
        }
    };

    struct _PartDevice
    {
        static void create( const RootConfig& config )
        {
            createRenderDevice();
            g_renderDevice->init( config.rdcContext );
            KHAOS_NEW VertexDeclarationManager;
        }

        static void destroy()
        {
            KHAOS_DELETE g_vertexDeclarationManager;
            g_renderDevice->shutdown();
            KHAOS_DELETE g_renderDevice;
        }
    };

    struct _PartResource
    {
        static void create()
        {
            KHAOS_NEW DefaultResourceScheduler;
            KHAOS_NEW MtrAttribFactory;
            KHAOS_NEW ResourceManager;

            TextureManager::init();
            MaterialManager::init();
            MeshManager::init();

            KHAOS_NEW SysResManager;
            g_sysResManager->init();
        }

        static void destroy()
        {
            g_sysResManager->shutdown();
            KHAOS_DELETE g_sysResManager;

            MeshManager::shutdown();
            MaterialManager::shutdown();
            TextureManager::shutdown();

            KHAOS_DELETE g_resourceManager;
            KHAOS_DELETE g_mtrAttribFactory;
            KHAOS_DELETE g_defaultResourceScheduler;
        }
    };

    struct _PartScene
    {
        static void create()
        {
            KHAOS_NEW NodeFactory;
        }

        static void destroy()
        {
            KHAOS_DELETE g_nodeFactory;
        }
    };

    struct _PartRenderSys
    {
        static void create()
        {
            KHAOS_NEW RenderSystem;
            g_renderSystem->init();
        }

        static void destroy()
        {
            g_renderSystem->shutdown();
            KHAOS_DELETE g_renderSystem;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    extern Root* g_root = 0;

    Root::Root()
    {
        khaosAssert( !g_root );
        g_root = this;
    }
    
    Root::~Root()
    {
        g_root = 0;
    }

    void Root::init( const RootConfig& config )
    {
        khaosLogLn( KHL_L1, "Root::init" );

        // 创建各个部件
        _PartSystem::create( config );
        _PartDevice::create( config );
        _PartResource::create();
        _PartScene::create();
        _PartRenderSys::create();

        // 所有的创建完成后，一些部件必要的启动
        _PartSystem::start();
    }

    void Root::shutdown()
    {
        khaosLogLn( KHL_L1, "Root::shutdown" );

        ResourceManager::_setSystemClosing();

        // 场景数据删除
        destroyAllSceneGraphs();

        // 一些部件必要的关闭
        _PartSystem::stop();

        // 各个部件销毁
        _PartRenderSys::destroy();
        _PartScene::destroy();
        _PartResource::destroy();
        _PartDevice::destroy();
        _PartSystem::destroy();
    }

    SceneGraph* Root::createSceneGraph( const String& name )
    {
        if ( m_sceneGraphMap.find(name) != m_sceneGraphMap.end() )
        {
            khaosLogLn( KHL_L2, "Already exist scene graph %s", name.c_str() );
            return 0;
        }

        SceneGraph* sg = KHAOS_NEW SceneGraph;
        sg->_setName( name );
        m_sceneGraphMap.insert( SceneGraphMap::value_type(name, sg) );
        return sg;
    }

    void Root::destroySceneGraph( const String& name )
    {
        SceneGraphMap::iterator it = m_sceneGraphMap.find(name);
        if ( it == m_sceneGraphMap.end() )
        {
            khaosLogLn( KHL_L2, "Not found scene graph %s", name.c_str() );
            return;
        }

        SceneGraph* sg = it->second;
        KHAOS_DELETE sg;
        m_sceneGraphMap.erase( it );
    }

    void Root::destroyAllSceneGraphs()
    {
        for ( SceneGraphMap::iterator it = m_sceneGraphMap.begin(), ite = m_sceneGraphMap.end(); it != ite; ++it )
        {
            SceneGraph* sg = it->second;
            KHAOS_DELETE sg;
        }

        m_sceneGraphMap.clear();
    }

    SceneGraph* Root::getSceneGraph( const String& name ) const
    {
        SceneGraphMap::const_iterator it = m_sceneGraphMap.find(name);
        if ( it != m_sceneGraphMap.end() )
            return it->second;
        return 0;
    }

    int Root::_getMaxActiveCameraPriority() const
    {
        if ( m_activeCameraMap.empty() )
            return 0;

        return m_activeCameraMap.rbegin()->first;
    }

    void Root::addActiveCamera( CameraNode * cameraNode, int priority )
    {
        if ( priority == -1 )
            priority = _getMaxActiveCameraPriority() + 1;

        if ( m_activeCameraMap.find(priority) != m_activeCameraMap.end() )
        {
            khaosLogLn( KHL_L2, "Already found active camera priority %d", priority );
            return;
        }

        m_activeCameraMap[priority] = cameraNode;
    }

    void Root::removeActiveCamera( CameraNode* cameraNode )
    {
        KHAOS_FOR_EACH( CameraMap, m_activeCameraMap, it )
        {
            if ( it->second == cameraNode )
            {
                m_activeCameraMap.erase(it);
                return;
            }
        }
    }

    void Root::run()
    {
        // update
        g_timerSystem.update();

        float timePiece = Math::maxVal( 1.0f / 60 - g_timerSystem.getElapsedTime(), 0.005f );
        g_msgQueueManager->update( timePiece );

        KHAOS_FOR_EACH( SceneGraphMap, m_sceneGraphMap, it )
        {
            SceneGraph* sg = it->second;
            sg->update();
        }

        g_renderSystem->update();

        // render
        g_renderSystem->beginRender();

        KHAOS_FOR_EACH( CameraMap, m_activeCameraMap, it )
        {
            CameraNode* node = it->second;
            g_renderSystem->renderScene( node );
        }

        g_renderSystem->endRender();
    }
}