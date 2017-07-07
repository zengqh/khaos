#pragma once
#include "KhaosBakeUtil.h"
#include "KhaosLightMapUtil.h"
#include "KhaosSampleUtil.h"
#include "KhaosTimer.h"

#define KHAOS_RAY_TEST_USE_SCENEBVH 1

namespace Khaos
{
    class VolumeProbeNode;
    class VolumeProbe;

    class IrrBake : public AllocatedObject, public IBakeSystemCallback
    {
    public:
        enum
        {
            MAX_BAKE_THREADS = 64
        };

        enum
        {
            PHASE_LITMAP_ONLY = 0x1,
            PHASE_VOLUME_ONLY = 0x2,
            PHASE_ALL         = ~0
        };

    private:
        enum
        {
            IBS_PREBUILD,
            IBS_INIT_AMB,
            IBS_INIT_EMIT,
            IBS_NEXT,
            IBS_VOLPROB,
        };

        enum FilterType
        {
            FITERTYPE_BN,
            FITERTYPE_AO,
            FITERTYPE_IRR
        };

        struct LitMapSet : public AllocatedObject
        {
            LitMapSet() : bentNormalMap(0), initEmit(0), initAmb(0), indIrrPre(0), indIrrNext(0), lightMapID(0) {} 
            ~LitMapSet();

            void createLitMap( int w, int h, bool needBentNormal, bool needAmb );
            void setOutMaps( const String& lma, const String& lmb, int id );
            void swapPreNext();

            SimpleLightMap* bentNormalMap;
            SimpleLightMap* initEmit;
            SimpleLightMap* initAmb;
            SimpleLightMap* indIrrPre;
            SimpleLightMap* indIrrNext;

            String outLightMapA;
            String outLightMapB;
            String preFileName;
            int    lightMapID;
        };

        struct VolumeMapSet : public AllocatedObject
        {
            VolumeMapSet() : indIrrVolMapR(0), indIrrVolMapG(0), indIrrVolMapB(0), mapID(0) {}
            ~VolumeMapSet();

            void createMaps( int w, int h, int d );

            void setOutMaps( const String& fileNameFmt, int id );

            union
            {
                SimpleVolumeMap* indIrrVolMaps[3];

                struct  
                {
                    SimpleVolumeMap* indIrrVolMapR;
                    SimpleVolumeMap* indIrrVolMapG;
                    SimpleVolumeMap* indIrrVolMapB;
                };
            };

            String preFileName;
            String outFileNameFmt;
            int    mapID;
        };

        typedef map<SceneNode*, LitMapSet*>::type LitMapSetMap;
        typedef map<SceneNode*, VolumeMapSet*>::type VolumeMapSetMap;

        struct TempContext
        {
            TempContext( const Vector3& pos1, const Vector3& norm1, const Vector3* basHL ) :
                baker(0), preFile(0), texelItem(0),
                pos(pos1), norm(norm1),
                basisHLWorld(basHL)
            {}

            IrrBake* baker;
            LightMapPreBuildFile* preFile;
            const LightMapPreBuildFile::TexelItem* texelItem;

            const Vector3& pos;
            const Vector3& norm;
            const Vector3* basisHLWorld;
        };

        struct HemiInfo
        {
            HemiInfo( int i = 0, float n = 0, bool f = false ) : id(i), NdotL(n), isFront(f) {}

            int   id;
            float NdotL;
            bool  isFront;
        };

        typedef vector<HemiInfo>::type HemiInfoList;
        typedef vector<HemiInfoList>::type ProbeSampleInfos;

    public:
        IrrBake();
        ~IrrBake();

        void setPreBVHName( const String& dataPath );

        void setPreBuildName( const String& dataPath, bool onlyUse );
        void setPreVolBuildName( const String& dataPath, bool onlyUse );

        void setResPreName( const String& nameA, const String& nameB, int nextID );
        void setResVolName( const String nameFmt, int nextID );

        void useDirectional( bool en );
        void setAmbient( const Color& clr );        

        void setPhase( uint phase );

        void general( SceneGraph* sg, IBakeInputData* id, int iterCnt );

        LitMapSetMap& getLightmapSets() { return m_litMapSet; }
        VolumeMapSetMap& getVolumemapSets() { return m_volMapSet; }

    private:
        // prepare
        virtual void onPrepareMesh( SceneNode* node, const NodeBakeInfo* info );
        virtual void onPrepareLight( SceneNode* node );
        virtual void onPrepareVolumeProbe( SceneNode* node );

        // bake node
        virtual void onBeginBakeNode( int idx, SceneNode* node, const NodeBakeInfo* info );
        virtual void onEndBakeNode( SceneNode* node, const NodeBakeInfo* info );

        // per-vertex
        virtual void onSetupBakeVertexProcess( BakeVertexProcess* bvProcess ) {}
        virtual void onBakePerVertex( int threadID, const Vector3& pos, const Vector3& norm, const Vector2& uv ) {}

        // per-texel
        virtual void onSetupLightMapProcess( LightMapProcess* lmProcess );
        virtual void onBakePerSubMeshBegin( SubMesh* subMesh, int subIdx );
        virtual void onBakePerFaceBegin( int threadID, int faceIdx, const Vector3& faceTangent, const Vector3& faceBinormal, const Vector3& faceNormal );
        virtual int  onBakeDiscardTexel( int threadID, int xp, int yp );
        virtual void onBakeRegisterTexel( int threadID, int xp, int yp );
        virtual void onBakePerTexel( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv );
        virtual void onBakePerSubMeshEnd( SubMesh* subMesh );

    private:
        LitMapSet* _getLitMapSet( SceneNode* node );
        void _freeAllLitMapSet();

        VolumeMapSet* _getVolMapSet( SceneNode* node );
        void _freeAllVolMapSet();

        void _createPreBuildFiles( LightMapPreBuildHeadFile*& headFile, LightMapPreBuildFile** preFiles,
            const String& name, int w, int h, int d, int threadCount, int sampleCnt, bool onlyRead );

        // lightmap bake
        void _initSphereMappers();
        void _cleanSphereMappers();
        void _initNextBake();
        
        void _postFilter();
        void _filterResult( LitMapSet* lms );
        static void _filterResultStatic( int threadID, void* para, int mapIdx );
        static Vector3 _getTexelBlackLumin( LightMapRemoveZero* fiter, ILightMapSource* lmSrc, int x, int y );
        static Vector3 _getTexelDLMBlackLumin( LightMapRemoveZero* fiter, ILightMapSource* lmSrc, int x, int y );

        void _finishLitMapDone();
        void _saveResult( SceneNode* node, LitMapSet* lms );

        void _readAllIndIrrLitMaps();
        void _readResult( SceneNode* node, LitMapSet* lms );
        void _readTextureToLitMap( SimpleLightMap* litMap, const String& file );

        void _bakePerTexel_Prebuild( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv );
        void _bakePerTexel_InitAmb( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv );
        void _bakePerTexel_InitEmit( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv );
        void _bakePerTexel_Irr( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv );

        static void _calcAOStatic( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt );
        float _calcAOWithPre( const SphereSamples::Sample& smpl, TempContext* tc );

        static void _calcIndIrrStatic( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt );
        void _calcIndIrrWithPre( const SphereSamples::Sample& smpl, TempContext* tc, float* vals );

        void _tempContextBindPreFile( TempContext& tc, int threadID, int xp, int yp );

        // volume probe bake
        void _finishVolMapDone();
        void _saveVolResult( SceneNode* node, VolumeMapSet* vms );

        void _initProbeSampleInfos();
        void _buildAllVolumeProbes();
        void _buildOneVolumeProbe();

        void _voxelIDToXYZ( VolumeProbe* probe, int voxelID, int& x, int& y, int& z ) const;

        static void _bakeVolProbeSatic( int threadID, void* para, int voxelID );
        void _bakeVolProbePre( int threadID, int voxelID );
        void _bakeVolProbe( int threadID, int voxelID );
        
        static void _projectProbeToSHStatic( SphereSamples* theSS, const SphereSamples::Sample& smpl,
            void* context, float* vals, int groupCnt );

    private:
        BakeSystem       m_bakeSys;
        LightMapProcess* m_lmProcess;
        LitMapSetMap     m_litMapSet;
        VolumeMapSetMap  m_volMapSet;
        uint             m_phase;
        int              m_status;
        int              m_currIterator;
        int              m_maxIterator;

        // amb
        Color            m_clrAmb;
        bool             m_isAmbSet;

        // directional
        bool             m_useDirectional;

        // prefile
        String m_bvhPreFileName;

        LightMapPreBuildHeadFile* m_preHeadFile;
        LightMapPreBuildHeadFile* m_preVolHeadFile;

        LightMapPreBuildFile* m_preFile[MAX_BAKE_THREADS];
        String           m_prebuildDataPath;
        bool             m_onlyUse;

        LightMapPreBuildFile* m_preVolFile[MAX_BAKE_THREADS];
        String           m_prebuildVolDataPath;
        bool             m_onlyUseVol;

        // curr info
        SceneNode*       m_currNode;
        LitMapSet*       m_currLitMapSet;
        Material*        m_currMtr;
        VolumeProbeNode* m_currVolProbNode;
        int              m_currSubIdx;
        int              m_currFaceIdx[MAX_BAKE_THREADS];
        //Vector3          m_currFaceTangentWorld[MAX_BAKE_THREADS];
        //Vector3          m_currFaceBinormalWorld[MAX_BAKE_THREADS];
        //Vector3          m_currFaceNormalWorld[MAX_BAKE_THREADS];
        bool             m_currLastLightMapPass;

        // res info
        String           m_resBaseAName;
        String           m_resBaseBName;
        String           m_resBaseVolName;
        int              m_litmapNextID;
        int              m_volmapNextID;

        // samples
        RandSamples         m_randHemi;
        SphereSampleMapper  m_sphInitMapper;
        SphereSampleMapper* m_sphMappers[MAX_BAKE_THREADS];

        RandSamples         m_randFull;
        SphereSampleMapper  m_sphVolMapper;
        ProbeSampleInfos    m_probeSampleInfos;

        // debug
        DebugTestTimer*     m_debugTimer;
    };

}

