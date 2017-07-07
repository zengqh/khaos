#pragma once
#include "KhaosBakeUtil.h"
#include "KhaosLightMapUtil.h"
#include "KhaosSampleUtil.h"

namespace Khaos
{
    class AOBake : public AllocatedObject, public IBakeSystemCallback
    {
        struct TempContext
        {
            AOBake* baker;
            Vector3 pos;
            Vector3 norm;
        };
        
    public:
        typedef map<int, String>::type NameMap; // light map id => light map name
        typedef map<String, int>::type IDMap; // scene node name => light map id

    public:
        AOBake();
        ~AOBake();

        void general( SceneGraph* sg, IBakeInputData* id );

        void setResPreName( const String& name, int nextID );

        void setMaxRayLen( float maxLen );

        void setFallOff( float falloff );

        const NameMap& getLitmaps() const { return m_nameMap; }
        NameMap& getLitmaps() { return m_nameMap; }

        const IDMap& getObjIDs() const { return m_idMap; }
        IDMap& getObjIDs() { return m_idMap; }

    private:
        // prepare
        virtual void onPrepareMesh( SceneNode* node, const NodeBakeInfo* info );
        virtual void onPrepareLight( SceneNode* node ) {}
        virtual void onPrepareVolumeProbe( SceneNode* node ) {}

        // bake node
        virtual void onBeginBakeNode( int idx, SceneNode* node, const NodeBakeInfo* info );
        virtual void onEndBakeNode( SceneNode* node, const NodeBakeInfo* info );

        // per-vertex
        virtual void onSetupBakeVertexProcess( BakeVertexProcess* bvProcess ) {}
        virtual void onBakePerVertex( int threadID, const Vector3& pos, const Vector3& norm, const Vector2& uv ) {}

        // per-texel
        virtual void onSetupLightMapProcess( LightMapProcess* lmProcess );
        virtual void onBakePerSubMeshBegin( SubMesh* subMesh, int subIdx ) {}
        virtual void onBakePerFaceBegin( int threadID, int faceIdx, const Vector3& faceTangent, const Vector3& faceBinormal, const Vector3& faceNormal ) {}
        virtual int  onBakeDiscardTexel( int threadID, int xp, int yp ) { return -1; }
        virtual void onBakeRegisterTexel( int threadID, int xp, int yp ) {}
        virtual void onBakePerTexel( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv );
        virtual void onBakePerSubMeshEnd( SubMesh* subMesh ) {}

    private:
        float _aoCalc( const SphereSamples::Sample& smpl, TempContext* tc );
        static void _aoCalcStatic( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt );

    public:
        static Vector3 _isAOBlack( LightMapRemoveZero* fiter, ILightMapSource* lmSrc, int x, int y );

    private:
        BakeSystem       m_bakeSys;
        LightMapProcess* m_lmProcess;
        SimpleLightMap*  m_litMap;

        NameMap          m_nameMap;
        IDMap            m_idMap;

        float            m_maxRayLen;
        bool             m_maxRayLenUsed;
        float            m_falloff;

        String           m_resBaseName;
        int              m_litmapNextID;

        RandSamples      m_rands;
    };

}

