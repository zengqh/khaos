#pragma once
#include "KhaosAxisAlignedBox.h"
#include "KhaosMesh.h"
#include "KhaosFileSystem.h"
#include "KhaosBinStream.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    struct MeshFileHead
    {
        int            version;
        AxisAlignedBox aabb;
        int            subMeshCount;
    };

    // [submesh]
    // aabb
    // prim type
    // vb type
    // ib type
    // vb bytes
    // vb
    // ib bytes
    // ib
    // mtr name

    //////////////////////////////////////////////////////////////////////////
    class MeshImporter : public AllocatedObject
    {
    public:
        bool importMesh( const String& file, Mesh* mesh );
        bool importMesh( const DataBuffer& data, Mesh* mesh );

    private:
        void _readSubMesh( BinStreamReader& reader, Mesh* mesh );
    };

    //////////////////////////////////////////////////////////////////////////
    class MeshExporter : public AllocatedObject
    {
    public:
        bool exportMesh( const String& file, const Mesh* mesh );
        bool exportMesh( BinStreamWriter& writer, const Mesh* mesh );

    private:
        void _writeSubMesh( BinStreamWriter& writer, const SubMesh* sm );
    };
}

