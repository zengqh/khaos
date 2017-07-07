#include "KhaosPreHeaders.h"
#include "KhaosPRT.h"
#include "KhaosSceneGraph.h"
#include "KhaosSampleUtil.h"
#include "KhaosRayQuery.h"
#include "KhaosMeshFile.h"
#include "KhaosResourceManager.h"
#include "KhaosMaterialManager.h"

namespace Khaos
{
#if 0
    class TestLight
    {
    public:
        TestLight()
        {
            KHAOS_CLEAR_ARRAY(_lightCoeff);
        }

        void init()
        {
            SphereSamples::projectSH( _testLight, 0, _lightCoeff, 16 );
        }

        static float _testLight( const SphereSamples::Sample& smpl, void* context )
        {
            return (smpl.theta < Math::PI/2) ? 1.0f : 0.0f;
        }

        float _lightCoeff[16];
    } g_testLight;
#endif

    PRTSystem::PRTSystem() : m_sg(0), m_tech(0), m_order(0), m_coeffMax(0), m_instanceContext(0)
    {
        m_rands.general( 1000, RandSamples::RT_Jitter );
        m_mapper.setRandsDistribution( &m_rands, SphereSampleMapper::EntireSphereUniform );
        m_mapper.setCommConfig( true, true );
        m_mapper.general();
        m_ss.setSamples( &m_mapper );
#if 0
        g_testLight.init();
#endif
    }

    void PRTSystem::prtTechAO( SceneGraph* sg, int order )
    {
        m_tech  = 0;
        m_order = order;
        m_coeffMax = order * order;
        m_sg = sg;

        _getSceneInstance();

        if ( !_checkSceneInstance() )
            return;

         _calcSceneSH();
    }

    void PRTSystem::_changeTechAO( SceneGraph* sg, int order )
    {
#if 0
        m_tech  = 0;
        m_order = order;
        m_coeffMax = order * order;
        m_sg = sg;

        _getSceneInstance();

        for ( size_t i = 0; i < m_meshNodes.size(); ++i )
        {
            MeshNode* meshNode = static_cast<MeshNode*>( m_meshNodes[i] );
            Mesh* mesh = meshNode->getMesh();

            int subMeshCnt = mesh->getSubMeshCount();

            for ( int i = 0; i < subMeshCnt; ++i )
            {
                SubMesh* sm = mesh->getSubMesh(i);
                VertexBuffer* vb = sm->getVertexBuffer();
                int vtxCnt = sm->getVertexCount();

                for ( int v = 0; v < vtxCnt; ++v )
                {
                    float* sh0 = vb->getCacheSH(v);

                    float ao = SHMath::shDotN( g_testLight._lightCoeff, sh0, m_coeffMax );
                    ao /= Math::PI;

                    Color clrDiff(Color::WHITE);
                    clrDiff *= ao;
                    clrDiff.saturate();
                    *(vb->getCacheColor(v)) = clrDiff.getAsARGB();
                }

                sm->refundLocalData();
            }
        }
#endif
    }

    void PRTSystem::_calcSceneSH()
    {
        for ( size_t i = 0; i < m_meshNodes.size(); ++i )
        {
            _calcNodeSH( m_meshNodes[i] );
        }
    }

    void PRTSystem::_saveFile( Mesh* mesh )
    {
        MeshExporter file;
        String resFullName = g_resourceManager->getGroup(Mesh::classType())
            ->getAutoCreator()->getLocateFileName( mesh->getName() );

        file.exportMesh( resFullName, mesh );
    }

    void PRTSystem::_calcNodeSH( SceneNode* node )
    {
        MeshNode* meshNode = static_cast<MeshNode*>( node );
        Mesh* mesh = meshNode->getMesh();
        
        const Matrix4& matWorld = meshNode->getDerivedMatrix();

        int subMeshCnt = mesh->getSubMeshCount();

        for ( int i = 0; i < subMeshCnt; ++i )
        {
            SubMesh* sm = mesh->getSubMesh(i);
            VertexBuffer* vb = sm->getVertexBuffer();
            int vtxCnt = sm->getVertexCount();
            
            for ( int v = 0; v < vtxCnt; ++v )
            {
                const Vector3* pos = vb->getCachePos(v);
                const Vector3* norm = vb->getCacheNormal(v);

                Vector3 posWorld  = matWorld.transformAffine( *pos );
                Vector3 normWorld = matWorld.transformAffineNormal( *norm ).normalisedCopy();

                float shCoeff[SHMath::MAX_COEFFS] = {};
                _calcVertex( posWorld, normWorld, shCoeff );

                float* sh0 = vb->getCacheSH(v);
                memcpy( sh0, shCoeff, m_coeffMax * sizeof(float) );

#if 0
                // test light
                float ao = SHMath::shDotN( g_testLight._lightCoeff, sh0, m_coeffMax );
                ao /= Math::PI;

                char msg[256];
                sprintf_s( msg, "a=%f\n", ao );
                OutputDebugStringA( msg );

                Color clrDiff(Color::WHITE);
                clrDiff *= ao;
                clrDiff.saturate();
                *(vb->getCacheColor(v)) = clrDiff.getAsARGB();
#endif
            }

            sm->refundLocalData();
        }

        // save file
        _saveFile( mesh );
    }

    void PRTSystem::_staticVertexFunc( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt )
    {
        return static_cast<PRTSystem*>(context)->_vertexFunc( smpl, vals, groupCnt );
    }

    void PRTSystem::_vertexFunc( const SphereSamples::Sample& sample, float* vals, int groupCnt )
    {
        Vector3** info = (Vector3**) m_instanceContext;
        const Vector3& posWorld  = *(info[0]);
        const Vector3& normWorld = *(info[1]);

        const float e_dot = 0.00873f; // 89.5度 
        const float e_ray = 0.01f;

        float dot = sample.dir.dotProduct( normWorld );

        if ( dot > e_dot ) // 在正半球
        {
            Ray ray( posWorld + sample.dir * e_ray, sample.dir ); // 往射线方向出去一些，避免痤疮问题

            if ( !rayIntersectSGDetail( m_sg, ray ) ) // 开放
            {
                *vals = dot;
                return;
            }
        }

        *vals = 0;
    }

    void PRTSystem::_calcVertex( const Vector3& posWorld, const Vector3& normWorld, float* shCoeff )
    {
        const Vector3* info[2] = { &posWorld, &normWorld };
        m_instanceContext = info;
        m_ss.projectSH( _staticVertexFunc, this, shCoeff, m_coeffMax, 1 );
    }

    bool PRTSystem::_checkSceneInstance()
    {
        set<Mesh*>::type records;

        for ( size_t i = 0; i < m_meshNodes.size(); ++i )
        {
            MeshNode* node = static_cast<MeshNode*>( m_meshNodes[i] );
            Mesh* mesh = node->getMesh();

            if ( !records.insert( mesh ).second )
                return false;
        }

        for ( set<Mesh*>::type::iterator it = records.begin(), ite = records.end(); it != ite; ++it )
        {
            Mesh* mesh = *it;
            mesh->expandSH( m_order );
            mesh->buildBVH( false );

            // save file
            //_saveFile( mesh );
        }

        return true;
    }

    void PRTSystem::_getSceneInstance()
    {
        for ( SceneGraph::NodeList::iterator it = m_sg->_getRootNodes().begin(), 
            ite = m_sg->_getRootNodes().end(); it != ite; ++it )
        {
            SceneNode* area = *it;
            _getNodeInstance(area);
        }
    }

    void PRTSystem::_getNodeInstance( SceneNode* node )
    {
        if ( node->isEnabled() && KHAOS_OBJECT_IS( node, MeshNode ) )
        {
            m_meshNodes.push_back( node );
        }

        for ( SceneNode::NodeList::iterator it = node->_getChildren().begin(),
            ite = node->_getChildren().end(); it != ite; ++it )
        {
            SceneNode* child = static_cast<SceneNode*>(*it);
            _getNodeInstance( child );
        }
    }
}

