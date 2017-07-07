#pragma once
#include "KhaosSceneGraph.h"

namespace Khaos
{
    
    //////////////////////////////////////////////////////////////////////////
    class SceneImporter : public AllocatedObject
    {
    public:
        bool importScene( const DataBuffer& data, SceneNode* node );


    };

    //////////////////////////////////////////////////////////////////////////
    class SceneExporter : public AllocatedObject
    {
    public:
        bool exportScene( const String& file, SceneNode* node );
    };
}

