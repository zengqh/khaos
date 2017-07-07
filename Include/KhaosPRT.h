#pragma once
#include "KhaosVector3.h"
#include "KhaosMath.h"
#include "KhaosSampleUtil.h"

namespace Khaos
{
    class SceneGraph;
    class SceneNode;
    class Mesh;

    class PRTSystem : public AllocatedObject
    {
        typedef vector<SceneNode*>::type SceneNodeList;

    public:
        PRTSystem();

        void prtTechAO( SceneGraph* sg, int order );

        void _changeTechAO( SceneGraph* sg, int order );

    private:
        void _getSceneInstance();
        void _getNodeInstance( SceneNode* node );
        bool _checkSceneInstance();

        void _calcSceneSH();
        void _calcNodeSH( SceneNode* node );
        void _calcVertex( const Vector3& posWorld, const Vector3& normWorld, float* shCoeff );

        void _saveFile( Mesh* mesh );

        static void _staticVertexFunc( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt );
        void _vertexFunc( const SphereSamples::Sample& smpl, float* vals, int groupCnt );

    private:
        RandSamples        m_rands;
        SphereSampleMapper m_mapper;
        SphereSamples      m_ss;

        SceneGraph*   m_sg;
        SceneNodeList m_meshNodes;
        int           m_tech;
        int           m_order;
        int           m_coeffMax;
        void*         m_instanceContext;
    };
}

