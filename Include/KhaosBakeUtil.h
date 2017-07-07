#pragma once
#include "KhaosVector3.h"
#include "KhaosVector2.h"
#include "KhaosMatrix4.h"

namespace Khaos
{
#define BAKE_DEFAULT_THREAD         4
#define BAKE_DEFAULT_SAMPLETYPE     SphereSampleMapper::EntireSphereUniform
#define BAKE_DEFAULT_RANDTYPE       RandSamples::RT_Hammersley
#define BAKE_DEFAULT_SAMPLECOUNT    3000

    //////////////////////////////////////////////////////////////////////////
    class SceneGraph;
    class SceneNode;
    class MeshNode;
    class SubMesh;
    class Mesh;
    class Material;
    class VertexBuffer;
    class LightMapProcess;

    //////////////////////////////////////////////////////////////////////////
    // 顶点过程
    struct IBakeVertexProcessCallback
    {
        virtual void onPerVertex( int threadID, const Vector3& pos, const Vector3& norm, const Vector2& uv ) = 0;
    };

    class BakeVertexProcess : public AllocatedObject
    {
        struct TaskPara
        {
            BakeVertexProcess* bvp;
            int low;
            int high;
        };

    public:
        BakeVertexProcess( Mesh* mesh, const Matrix4& matWorld );

        void general( IBakeVertexProcessCallback* ev, int threads );

        SubMesh* getCurrSubMesh() const { return m_currSubMesh; }

    private:
        void _planTask( int vtxCnt, int threads );
        static void _bakeSatic( int threadID, void* para, int tIdx );

    private:
        Mesh*         m_mesh;
        Matrix4       m_matWorld;
        SubMesh*      m_currSubMesh;
        VertexBuffer* m_currVB;
        IBakeVertexProcessCallback* m_callback;
    };

    //////////////////////////////////////////////////////////////////////////
    // 场景唯一实例化处理
    class SceneBakeUniquify
    {
    public:
        struct NewMeshInfo
        {
            NewMeshInfo() : bakeTextureWidth(0), bakeTextureHeight(0) {}
            String newMeshName;
            int bakeTextureWidth;
            int bakeTextureHeight;
        };

        typedef map<Mesh*, int>::type MeshUsedMap; // old mesh -> used count
        typedef map<String, NewMeshInfo>::type NewMeshInfoMap; // node name -> new mesh info

        typedef map<Material*, int>::type MtrUsedMap; // old mtr -> used count

        typedef vector<Mesh*>::type MeshList; // new mesh list for save
        typedef vector<Material*>::type MtrList; // new mtr list for save

    public:
        enum
        {
            SBU_NEED_UV2 = 0x1,
            SBU_NEED_SH3 = 0x2,
            SBU_NEED_SH4 = 0x4,
            //SBU_UNIQ_MTR = 0x100,

            SBU_SAVE_MESH = 0x1,
            //SBU_SAVE_MTR  = 0x2,
            SBU_SAVE_INFO = 0x4,
            SBU_SAVE_ALL  = -1
        };

    public:
        SceneBakeUniquify() : m_config(0), m_totalUV2Area(0) {}

        void process( SceneGraph* sgIn, int config );
        void save( const String& extInfoFile, int flag );
        void load( const String& extInfoFile );

        const NewMeshInfoMap& getNewMeshInfoMap() const { return m_meshNewInfoMap; }
        NewMeshInfoMap& getNewMeshInfoMap() { return m_meshNewInfoMap; }

    private:
        void _dealNode( MeshNode* node );
        void _dealNodeMesh( MeshNode* node );
        void _dealNodeMtr( MeshNode* node );

        Mesh* _generalUV2( MeshNode* node, const String& meshNewName );
        Mesh* _generalSH(  const String& name, Mesh* meshOld );

        void _saveExtInfo( const String& extInfoFile );

    private:
        int             m_config;
        MeshUsedMap     m_meshUsedMap;
        //MtrUsedMap      m_mtrUsedMap;
        NewMeshInfoMap  m_meshNewInfoMap;
        MeshList        m_newMeshList;
        //MtrList         m_newMtrList;
        double          m_totalUV2Area;
    };

    //////////////////////////////////////////////////////////////////////////
    // 烘焙通用系统
    struct NodeBakeInfo
    {
        NodeBakeInfo() : textureWidth(0), textureHeight(0) {}

        // for bake per texel
        int textureWidth;
        int textureHeight;
    };

    struct IBakeInputData
    {
        virtual const NodeBakeInfo* getBakeInfo( SceneNode* node ) const = 0;
    };

    class SimpleBakeInputData : public IBakeInputData
    {
    private:
        typedef map<SceneNode*, NodeBakeInfo>::type DataMap;

    public:
        void addBakeInfo( SceneNode* node, const NodeBakeInfo& info );

        virtual const NodeBakeInfo* getBakeInfo( SceneNode* node ) const;

    private:
        DataMap m_datas;
    };

    struct IBakeSystemCallback
    {
        // prepare
        virtual void onPrepareMesh( SceneNode* node, const NodeBakeInfo* info ) = 0;
        virtual void onPrepareLight( SceneNode* node ) = 0;
        virtual void onPrepareVolumeProbe( SceneNode* node ) = 0;

        // bake node
        virtual void onBeginBakeNode( int idx, SceneNode* node, const NodeBakeInfo* info ) = 0;
        virtual void onEndBakeNode( SceneNode* node, const NodeBakeInfo* info ) = 0;

        // per-vertex
        virtual void onSetupBakeVertexProcess( BakeVertexProcess* bvProcess ) = 0;
        virtual void onBakePerVertex( int threadID, const Vector3& pos, const Vector3& norm, const Vector2& uv ) = 0;

        // per-texel
        virtual void onSetupLightMapProcess( LightMapProcess* lmProcess ) = 0;
        virtual void onBakePerSubMeshBegin( SubMesh* subMesh, int subIdx ) = 0;
        virtual void onBakePerFaceBegin( int threadID, int faceIdx, const Vector3& faceTangent, const Vector3& faceBinormal, const Vector3& faceNormal ) = 0;
        virtual int  onBakeDiscardTexel( int threadID, int xp, int yp ) = 0;
        virtual void onBakeRegisterTexel( int threadID, int xp, int yp ) = 0;
        virtual void onBakePerTexel( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv ) = 0;
        virtual void onBakePerSubMeshEnd( SubMesh* subMesh ) = 0;
    };

    class BakeSystem : public AllocatedObject
    {
    public:
        enum
        {
            BSF_PER_VERTEX = 0x1,
            BSF_PER_TEXEL  = 0x2,

            BSF_PER_MASK = 0x3
        };

        typedef vector<SceneNode*>::type SceneNodeList;

    public:
        BakeSystem();

        void init( SceneGraph* sg, IBakeInputData* id, IBakeSystemCallback* cb, int config, int threads );
        void general();
        bool prepare();
        void bakeOnce();

        SceneGraph* getSG() const { return m_sg; }

        const SceneNodeList& getMeshNodeList() const { return m_meshNodes; }
        const SceneNodeList& getLightNodeList() const { return m_litNodes; }
        const SceneNodeList& getVolumeProbeNodeList() const { return m_volProbeNodes; }

        int getThreadsCount() const { return m_threads; }

    private:
        void _gatherAllNodes();
        bool _checkMeshNodes();
        void _checkLitNodes();

        void _bakeScene();
        void _bakeNode( int idx, SceneNode* node );
        void _bakeNodePerVertex( const NodeBakeInfo* info, Mesh* mesh, const Matrix4& matWorld );
        void _bakeNodePerTexel( const NodeBakeInfo* info, Mesh* mesh, const Matrix4& matWorld );

        int  _getPerBakeType() const;

    private:
        SceneGraph*          m_sg;
        IBakeInputData*      m_inputData;
        IBakeSystemCallback* m_callback;
        SceneNodeList        m_meshNodes;
        SceneNodeList        m_litNodes;
        SceneNodeList        m_volProbeNodes;
        int                  m_config;
        int                  m_threads;
    };
}


