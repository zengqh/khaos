#include "KhaosPreHeaders.h"
#include "KhaosMeshManager.h"
#include "KhaosMeshManualCreator.h"
#include "KhaosMeshFile.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    MeshResAutoCreator::MeshResAutoCreator()
    {
        m_basePath = "/Model/";
    }

    bool MeshResAutoCreator::_prepareResourceImpl( Resource* res, DataBuffer& buff )
    {
        res->_getBufferTmp() = buff;
        return true; 
    }

    bool MeshResAutoCreator::buildResource( Resource* res ) 
    { 
        Mesh* mesh = static_cast<Mesh*>(res);
        FileSystem::DataBufferAuto buffAuto(mesh->_getBufferTmp());

        MeshImporter meshImp;
        meshImp.importMesh( buffAuto, mesh );
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    Mesh* MeshManager::createCube( const String& name, float size )
    {
        Mesh* mesh = createMesh( name );
        if ( mesh )
            mesh->setResCreator( KHAOS_NEW MeshCubeCreatorImpl(size) );
        return mesh;
    }

    Mesh* MeshManager::createSphere( const String& name, float size, int segments )
    {
        Mesh* mesh = createMesh( name );
        if ( mesh )
            mesh->setResCreator( KHAOS_NEW MeshSphereCreatorImpl(size, segments) );
        return mesh;
    }

    Mesh* MeshManager::createPlane( const String& name, float xLen, float zLen, int xSegments, int zSegments )
    {
        Mesh* mesh = createMesh( name );
        if ( mesh )
            mesh->setResCreator( KHAOS_NEW MeshPlaneCreatorImpl(xLen, zLen, xSegments, zSegments) );
        return mesh;
    }

    Mesh* MeshManager::createBox( const String& name, float xLen, float yLen, float zLen, 
        int xSegments, int ySegments, int zSegments, bool isIn )
    {
        Mesh* mesh = createMesh( name );
        if ( mesh )
            mesh->setResCreator( KHAOS_NEW MeshBoxCreatorImpl(xLen, yLen, zLen, xSegments, ySegments, zSegments, isIn ) );
        return mesh;
    }

    Mesh* MeshManager::createCone( const String& name, float width, float height, int wsegments, int hsegments, bool useNZDir )
    {
        Mesh* mesh = createMesh( name );
        if ( mesh )
            mesh->setResCreator( KHAOS_NEW MeshConeCreatorImpl(width, height, wsegments, hsegments, useNZDir) );
        return mesh;
    }
}

