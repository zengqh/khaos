#include "KhaosPreHeaders.h"
#include "KhaosMeshFile.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    bool MeshImporter::importMesh( const String& file, Mesh* mesh )
    {
        FileSystem::DataBufferAuto data;

        if ( !g_fileSystem->getFileData( file, data ) )
            return false;

        return importMesh( data, mesh );
    }

    bool MeshImporter::importMesh( const DataBuffer& data, Mesh* mesh )
    {
        BinStreamReader reader(data.data, data.dataLen);
        
        MeshFileHead head;
        reader.read( head );

        mesh->setAABB( head.aabb );

        for ( int i = 0; i < head.subMeshCount; ++i )
        {
            _readSubMesh( reader, mesh );
        }

        return true;
    }

    void MeshImporter::_readSubMesh( BinStreamReader& reader, Mesh* mesh )
    {
        SubMesh* sm = mesh->createSubMesh();

        // aabb
        AxisAlignedBox aabb;
        reader.read( aabb );
        sm->setAABB( aabb );

        // prim type
        int primType = 0;
        reader.read( primType );
        sm->setPrimitiveType( (PrimitiveType)primType );

        // vb type
        // ib type
        int vbType = 0;
        int ibType = 0;
        reader.read( vbType );
        reader.read( ibType );

        // vb bytes
        // vb
        int vbBytes = 0;
        reader.read( vbBytes );
        vector<char>::type vbData(vbBytes+1);
        reader.read( &vbData[0], vbBytes );

        VertexBuffer* vb = sm->createVertexBuffer();
        vb->create( vbBytes, HBU_STATIC );
        vb->fillData( &vbData[0] );
        vb->setDeclaration( g_vertexDeclarationManager->getDeclaration(vbType) );

        // ib bytes
        // ib
        int ibBytes = 0;
        reader.read( ibBytes );
        vector<char>::type ibData(ibBytes+1);
        reader.read( &ibData[0], ibBytes );

        IndexBuffer* ib = sm->createIndexBuffer();
        ib->create( ibBytes, HBU_STATIC, (IndexElementType)ibType );
        ib->fillData( &ibData[0] );

        // mtr name
        String mtrName;
        reader.readString( mtrName );
        sm->setMaterialName( mtrName );
    }

    //////////////////////////////////////////////////////////////////////////
    bool MeshExporter::exportMesh( const String& file, const Mesh* mesh )
    {
        BinStreamWriter writer;

        if ( !exportMesh(writer, mesh) )
            return false;

        DataBuffer buff;
        buff.data = writer.getBlock();
        buff.dataLen = writer.getCurrentSize();

        return g_fileSystem->writeFile( file, buff );
    }

    bool MeshExporter::exportMesh( BinStreamWriter& writer, const Mesh* mesh )
    {
        MeshFileHead head;

        head.version = 0;
        head.aabb = mesh->getAABB();
        head.subMeshCount = mesh->getSubMeshCount();

        writer.write( head );

        for ( int i = 0; i < mesh->getSubMeshCount(); ++i )
        {
            _writeSubMesh( writer, mesh->getSubMesh(i) );
        }

        return true;
    }

    void MeshExporter::_writeSubMesh( BinStreamWriter& writer, const SubMesh* sm )
    {
        // aabb
        writer.write( sm->getAABB() );

        // prim type
        writer.write( (int)sm->getPrimitiveType() );

        // vb type
        // ib type
        writer.write( (int) sm->getVertexBuffer()->getDeclaration()->getID() );
        writer.write( (int) sm->getIndexBuffer()->getIndexType() );

        // vb bytes
        // vb
        int vbBytes = sm->getVertexBuffer()->getSize();
        writer.write( vbBytes );
        vector<char>::type vbData(vbBytes+1);
        sm->getVertexBuffer()->readData( &vbData[0] );
        writer.write( &vbData[0], vbBytes );

        // ib bytes
        // ib
        int ibBytes = sm->getIndexBuffer()->getSize();
        writer.write( ibBytes );
        vector<char>::type ibData(ibBytes+1);
        sm->getIndexBuffer()->readData( &ibData[0] );
        writer.write( &ibData[0], ibBytes );

        // mtr name
        writer.writeString( sm->getMaterialName() );
    }
}

