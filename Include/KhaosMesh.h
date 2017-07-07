#pragma once
#include "KhaosResource.h"
#include "KhaosVertexIndexBuffer.h"
#include "KhaosAxisAlignedBox.h"
#include "KhaosAABBBVH.h"

namespace Khaos
{
    class SubMesh : public AllocatedObject
    {
        typedef vector<int>::type IntList;
        typedef vector<IntList>::type IntListList;

    public:
        SubMesh();
        ~SubMesh();

    public:
        PrimitiveType getPrimitiveType() const { return m_primType; }
        void setPrimitiveType( PrimitiveType pt ) { m_primType = pt; }

        int getPrimitiveCount() const;
        int getVertexCount() const;

        VertexBuffer* createVertexBuffer();
        IndexBuffer*  createIndexBuffer();

        VertexBuffer* getVertexBuffer() const { return m_vb.get(); }
        IndexBuffer*  getIndexBuffer() const { return m_ib.get(); }

        template<class T>
        void createVertexBuffer( const T* vbData, int vbCnt )
        {
            VertexBuffer* vb = createVertexBuffer();
            vb->create( vbCnt * sizeof(T), HBU_STATIC );
            vb->fillData( vbData );
            vb->setDeclaration( g_vertexDeclarationManager->getDeclaration(T::ID) );
        }
       
        template<class T>
        void createIndexBuffer( const T* ibData, int ibCnt )
        {
            IndexBuffer* ib = createIndexBuffer();
            ib->create( ibCnt * sizeof(T), HBU_STATIC, sizeof(T) == sizeof(uint16) ? IET_INDEX16 : IET_INDEX32 );
            ib->fillData( ibData );
        }

        void expandNormal();
        void expandTangent();
        void expandSH( int order );
        void expandTex2();

        void generateNormals( bool forceUpdate, bool refundImm = true );
        void generateTangents(  bool forceUpdate, bool refundImm = true );

        void setMaterialName( const String& mtrName ) { m_materialName = mtrName; }
        const String& getMaterialName() const { return m_materialName; }

        void updateAABB();
        void setAABB( const AxisAlignedBox& aabb );
        const AxisAlignedBox& getAABB() const { return m_aabb; }

        void draw();

        void cacheLocalData( bool forceUpdate );
        void freeLocalData();
        void refundLocalData( bool vb = true, bool ib = false );

        void getTriVertices( int face, Vector3*& v0, Vector3*& v1, Vector3*& v2 );
        Vector2 getTriUV( int face, const Vector3& gravity ) const;
        Vector2 getTriUV2( int face, const Vector3& gravity ) const;
        Vector3 getTriNormal( int face, const Vector3& gravity ) const;

        void buildBVH( bool forceUpdate );
        void clearBVH();

        AABBBVH::Result intersectDetail( const Ray& ray ) const;
        AABBBVH::Result intersectDetail( const LimitRay& ray ) const;

        void copyFrom( const SubMesh* rhs );

    private:
        void _expandVB( int maskAdd );
        void _getVertexAdjList( IntListList& adjList );
        Vector3 _getFaceNormal( int face ) const;

    private:
        PrimitiveType       m_primType;
        VertexBufferScpPtr  m_vb;
        IndexBufferScpPtr   m_ib;
        AxisAlignedBox      m_aabb;
        String              m_materialName;
        AABBBVH*            m_bvh;
    };

    class Mesh : public Resource
    {
        KHAOS_DECLARE_RTTI(Mesh)

    public:
        Mesh() {}
        virtual ~Mesh();

    public:
        typedef vector<SubMesh*>::type SubMeshList;

    public:
        SubMesh* createSubMesh();
        
        int getSubMeshCount() const { return (int)m_subMeshList.size(); }
        SubMesh* getSubMesh( int i ) const { return m_subMeshList[i]; }

        int getVertexCount() const;

    public:
        void setMaterialName( const String& mtrName );

    public:  
        const AxisAlignedBox& getAABB() const { return m_aabb; }

        void updateAABB( bool forceAll );
        void setAABB( const AxisAlignedBox& aabb );

        void expandSH( int order );
        void expandTex2();

        void generateNormals( bool forceUpdate, bool refundImm = true );
        void generateTangents( bool forceUpdate, bool refundImm = true );

        void drawSub( int i );

        void cacheLocalData( bool forceUpdate );
        void freeLocalData();

        void buildBVH( bool forceUpdate );
        void clearBVH();

        bool intersectBound( const Ray& ray, float* t = 0 ) const;
        bool intersectBoundMore( const Ray& ray, int* subIdx = 0, float* t = 0 ) const;
        bool intersectDetail( const Ray& ray, int* subIdx = 0, int* faceIdx = 0, float* t = 0, Vector3* gravity = 0 ) const;

        bool intersectBound( const LimitRay& ray, float* t = 0 ) const;
        bool intersectBoundMore( const LimitRay& ray, int* subIdx = 0, float* t = 0 ) const;
        bool intersectDetail( const LimitRay& ray, int* subIdx = 0, int* faceIdx = 0, float* t = 0, Vector3* gravity = 0 ) const;

    public:
        virtual void copyFrom( const Resource* rhs );
        virtual void _destructResImpl();

    private:
        void _clearSubMesh();

        template<class T>
        bool _intersectBoundImpl( const T& ray, float* t ) const;

        template<class T>
        bool _intersectBoundMoreImpl( const T& ray, int* subIdx, float* t ) const;

        template<class T>
        bool _intersectDetailImpl( const T& ray, int* subIdx, int* faceIdx, float* t, Vector3* gravity = 0 ) const;

    private:
        SubMeshList    m_subMeshList;
        AxisAlignedBox m_aabb;
    };

    typedef ResPtr<Mesh> MeshPtr;
}

