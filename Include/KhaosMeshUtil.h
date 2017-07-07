#pragma once
#include "KhaosVector3.h"
#include "KhaosVector2.h"
#include "KhaosMatrix4.h"
#include "KhaosMath.h"
#include "KhaosNameDef.h"
#include "KhaosVertexIndexBuffer.h"

namespace Khaos
{
    class Mesh;

    //////////////////////////////////////////////////////////////////////////
    class SmoothGroupGather
    {
    public:
        typedef vector<int>::type IntList;
        typedef vector<IntList>::type IntListArray;
        typedef set<int>::type IntSet;

        struct Side
        {
            Side( int _v0 = 0, int _v1 = 0 ) : v0(_v0), v1(_v1) 
            {
                if ( v0 > v1 )
                    swapVal(v0, v1); // 总是从小到大排序
            }

            int v0, v1;

            bool operator<( const Side& rhs ) const
            {
                return *(uint64*)(this) < *(uint64*)(&rhs);
            }
        };

        typedef map<Side, int>::type SideMap; // side => used count

    public:
        void gatherFaces( const IntList& ib, IntListArray& groups );
        void gatherVertices( const IntList& ib, const IntList& faces, IntSet& vertices );
        void gatherSides( const IntList& ib, const IntList& faces, SideMap& sides );
    };

    //////////////////////////////////////////////////////////////////////////
    class GeneralMeshUV
    {
    public:
        enum
        {
            AUTO_TEX_SIZE_HIGH = -1,
            AUTO_TEX_SIZE_MED  = -2,
            AUTO_TEX_SIZE_LOW  = -3
        };

    private:
        struct VertexID
        {
            VertexID() : subIdx(0), vtxIdx(0), id(0) {}
            VertexID( int s, int v, int i ) : subIdx(s), vtxIdx(v), id(i) {}

            int subIdx; // submesh index in mesh
            int vtxIdx; // vertex index in submesh
            int id;     // global id

            bool operator==( const VertexID& rhs ) const
            {
#if KHAOS_DEBUG
                if ( id == rhs.id )
                    khaosAssert( subIdx == rhs.subIdx && vtxIdx == rhs.vtxIdx );
#endif
                return id == rhs.id;
            }

            Vector3 posWorld;  // position in world space
            Vector3 normWorld; // normal in world space
            Vector4 tangentWorld; // tangent in world space
            Vector2 uv1;       // uv 1
        };

        struct SimpleVertex
        {
            Vector3 pos;
            int     id;
            Vector3 norm;
            Vector2 uv1;
            Vector2 uv2;
        };

        typedef vector<VertexID>::type VertexIDList;
        typedef vector<int>::type IntList;
        typedef vector<IntList>::type IntListMap;// [sm][idx] = vid

        typedef vector<VertexPNTT>::type VertexPNTTOutList;
        typedef vector<VertexPNTTOutList>::type VertexPNTTOutListArray;

        typedef vector<VertexPNGTT>::type VertexPNGTTOutList;
        typedef vector<VertexPNGTTOutList>::type VertexPNGTTOutListArray;

        // 面信息
        struct FaceInfo
        {
            FaceInfo() : area(0) {}
            Vector3 norm;
            float   area;
        };

        typedef vector<FaceInfo>::type FaceInfoList;
        
        // 平滑组信息
        struct SmoothGroupInfo
        {
            SmoothGroupInfo() : areaReal(0), areaBox(0), isFlat(false) {}

            float areaReal;
            float areaBox;
            Vector3 vecMax;
            Vector3 avgNorm;
            bool    isFlat;

            SmoothGroupGather::IntSet vertices;
        };

        typedef vector<SmoothGroupInfo>::type SmoothGroupInfoArray;
        typedef SmoothGroupGather::IntListArray IntListArray;

    public:
        GeneralMeshUV( Mesh* mesh, const Matrix4& matWorld ) : 
            m_mesh(mesh), m_meshOut(0), m_matWorld(matWorld) {}

        Mesh* apply( const String& newMeshName, int* texSize );

        static void setTexSizeBound( int minSize, int maxSize );

    private:
        void _prepare();

        void  _generalFaceInfo( bool needNorm, bool needArea );
        float _calcTotalArea() const;

        bool _calcSmoothGroupAvgNormal( const SmoothGroupGather::IntList& faceGroup, Vector3& avgNorm ) const;
        bool _findSmoothGroupStretchVector( const SmoothGroupGather::IntList& faceGroup, 
            Vector3& vecMax, Vector3& avgNorm ) const;
        bool _getMagnifyMatrix( const SmoothGroupGather::IntSet& vertices, const Vector3& vecMax, const Vector3& avgNorm, 
            Matrix4& matAdj, float& lenX, float& lenZ ) const;
        void _transformSmoothGroup( const SmoothGroupGather::IntSet& vertices, const Matrix4& mat );

        void _adjustSmoothGroupStretch( int i );
        void _adjustSmoothGroupArea( int i, float avgArea );

        float _getAutoTexSizeUnit( int type ) const;
        int  _estimateTexSize( int type, float area, int smoothGroups ) const;
        int  _calcSuitableTexSize( int texSize );

        void* _convertD3DMesh() const;
        void* _createUVAtlas( void* d3dMesh, int texSize, void** ppVertexRemapArray );
        Mesh* _buildNewMesh( const String& newMeshName, void* d3dMesh, void* d3dMeshOut, void* pVertexRemapArray ) const;

        template<class VertexOutListArray>
        Mesh* _createNewMesh( const String& newMeshName, const VertexOutListArray& newVbs, const IntListMap& newIbs ) const;

    private:
        Mesh*        m_mesh;
        Mesh*        m_meshOut;
        Matrix4      m_matWorld;
        
        VertexIDList m_vb;
        IntListMap   m_vbMap;
        IntList      m_ib;
        FaceInfoList m_faceInfo;

        IntListArray         m_faceGroups;
        SmoothGroupInfoArray m_sgInfos;

        static int   s_minTexSize;
        static int   s_maxTexSize;
    };
}


