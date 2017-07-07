#pragma once
#include "KhaosTimer.h"
#include "KhaosMsgQueue.h"
#include "KhaosFileSystem.h"
#include "KhaosRenderDevice.h"
#include "KhaosMaterialManager.h"
#include "KhaosTextureManager.h"
#include "KhaosMeshManager.h"
#include "KhaosSceneGraph.h"
#include "KhaosRenderSystem.h"
#include "KhaosRayQuery.h"
#include "KhaosNameDef.h"

namespace Khaos
{
    struct RootConfig
    {
        RenderDeviceCreateContext rdcContext;
        String                    fileSystemPath;
    };

    class Root : public AllocatedObject
    {
    public:
        typedef unordered_map<String, SceneGraph*>::type SceneGraphMap;
        typedef map<int, CameraNode*>::type CameraMap;

    public:
        Root();
        ~Root();

    public:
        void init( const RootConfig& config );
        void shutdown();
        void run();

    public:
        SceneGraph* createSceneGraph( const String& name );
        void        destroySceneGraph( const String& name );
        void        destroyAllSceneGraphs();
        SceneGraph* getSceneGraph( const String& name ) const;

    public:
        void addActiveCamera( CameraNode * cameraNode, int priority = -1 );
        void removeActiveCamera( CameraNode* cameraNode );

    private:
        int _getMaxActiveCameraPriority() const;

    private:
        SceneGraphMap       m_sceneGraphMap;
        CameraMap           m_activeCameraMap;    
    };
    
    extern Root* g_root;
}

