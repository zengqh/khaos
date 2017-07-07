#pragma once
#include "KhaosMesh.h"
#include "KhaosResourceManager.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class MeshResAutoCreator : public ResAutoCreatorBase
    {
    public:
        MeshResAutoCreator();

    private:
        virtual bool buildResource( Resource* res );
        virtual bool _prepareResourceImpl( Resource* res, DataBuffer& buff );
    };

    //////////////////////////////////////////////////////////////////////////
    class MeshManager
    {
        KHAOS_RESMAN_COMM_IMPL(Mesh, MeshResAutoCreator)

    public:
        // 内部对象
        static Mesh* createCube( const String& name, float size );
        static Mesh* createSphere( const String& name, float size, int segments );
        static Mesh* createPlane( const String& name, float xLen, float zLen, int xSegments, int zSegments );
        static Mesh* createBox( const String& name, float xLen, float yLen, float zLen, int xSegments, int ySegments, int zSegments, bool isIn = false );
        static Mesh* createCone( const String& name, float width, float height, int wsegments, int hsegments, bool useNZDir = false );
    };
}

