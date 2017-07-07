#include "KhaosPreHeaders.h"
#include "KhaosAOBake.h"
#include "KhaosMesh.h"
#include "KhaosMeshNode.h"
#include "KhaosInstObjSharedData.h"
#include "KhaosSceneGraph.h"
#include "KhaosMaterialManager.h"
#include "KhaosTextureManager.h"
#include "KhaosTexCfgParser.h"
#include "KhaosMaterialFile.h"
#include "KhaosRayQuery.h"
#include "KhaosTimer.h"

namespace Khaos
{
    AOBake::AOBake() : m_lmProcess(0), m_litMap(0), m_maxRayLen(0), m_maxRayLenUsed(false),
        m_falloff(0.25f), m_litmapNextID(0) 
    {
        m_rands.general( BAKE_DEFAULT_SAMPLECOUNT, BAKE_DEFAULT_RANDTYPE );
    }

    AOBake::~AOBake()
    {
        KHAOS_DELETE m_litMap;
    }

    void AOBake::general( SceneGraph* sg, IBakeInputData* id )
    {
        KHAOS_DEBUG_TEST_TIMER("AOBake::general")

        m_bakeSys.init( sg, id, this, BakeSystem::BSF_PER_TEXEL, BAKE_DEFAULT_THREAD );
        m_bakeSys.general();
    }

    void AOBake::setResPreName( const String& name, int nextID )
    {
        m_resBaseName = name;
        m_litmapNextID = Math::maxVal( nextID, 0 );
    }

    void AOBake::setMaxRayLen( float maxLen )
    {
        m_maxRayLen = Math::maxVal(maxLen, 0.0f);
        m_maxRayLenUsed = ( maxLen < Math::POS_INFINITY );
    }

    void AOBake::setFallOff( float falloff )
    {
        m_falloff = Math::clamp( falloff, 0.0f, 0.99f );
    }

    void AOBake::onPrepareMesh( SceneNode* node, const NodeBakeInfo* info )
    {
        MeshNode* entity = static_cast<MeshNode*>(node);
        Mesh* mesh = entity->getMesh();

        // Ӧ���еڶ���uv
        khaosAssert( mesh->getSubMesh(0)->getVertexBuffer()->getDeclaration()->getID() & VFM_TEX2 );

        // ������lightmap���ԣ��ر���
        if ( entity->getLightMapID() >= 0 )
        {
            m_bakeSys.getSG()->getInstSharedDataMgr().removeLightmap( entity->getLightMapID() );
            entity->setLightMapID( InvalidInstSharedDataID );
        }
    }

    void AOBake::onBeginBakeNode( int idx, SceneNode* node, const NodeBakeInfo* info )
    {
        KHAOS_DELETE m_litMap;

        m_litMap = KHAOS_NEW SimpleLightMap;
        m_litMap->setSize( info->textureWidth, info->textureHeight, 1 );
    }

    Vector3 AOBake::_isAOBlack( LightMapRemoveZero* fiter, ILightMapSource* lmSrc, int x, int y )
    {
        SimpleLightMap* litMap = static_cast<SimpleLightMap*>(lmSrc);
        float v = litMap->getRed( x, y );
        return Vector3(v);
    }

    void AOBake::onEndBakeNode( SceneNode* node, const NodeBakeInfo* info )
    {
        // ���浽�ļ�
        //MeshNode* meshNode = static_cast<MeshNode*>(node);
        //String mainName, extName;
        //splitFileName( meshNode->getMesh()->getResFileName(), mainName, extName ); // ʹ��mesh�ļ�������ź�����ͬĿ¼

        char resName[1024] = {};
        sprintf( resName, m_resBaseName.c_str(), m_litmapNextID );
        m_nameMap[m_litmapNextID] = resName;

        String resFullFileName = g_fileSystem->getFullFileName(resName);

        // �ȱ���δ����ĵ����ļ�
        //m_litMap->saveDebugTemp( (resName + String(".dbg")).c_str() ); // ����debug

        // ���˺�ɫ��Ե
        LightMapRemoveZero lmRem;
        lmRem.filter( m_litMap, _isAOBlack, 25.0f/255.0f, 55.0f/255.0f, true );

        // ����Ե
        LightMapPostFill lmFill;
        lmFill.fill( m_litMap, 6 );

        // ����ģ��
        //SimpleLightMap mapFilter;
        //mapFilter.initARGB( m_litMap->getWidth(), m_litMap->getHeight(), Color(0.0f, 0.0f, 0.0f) );
        //LightMapPostBlur lmBlur;
        //lmBlur.filter( m_litMap, &mapFilter );

        // дao�����ļ�
        PixelFormat fmt = PIXFMT_A8R8G8B8; //PIXFMT_16F PIXFMT_A16B16G16R16F PIXFMT_A8R8G8B8
        m_litMap->saveFile( fmt, resFullFileName.c_str() ); // ���������ļ�
        TexCfgSaver::saveSimple( resName, TEXTYPE_2D, fmt, TEXADDR_MIRROR, TEXF_LINEAR, TEXF_NONE, false, 1 ); // �������������ļ�

        // ��������
        //meshNode->setLightMapID(m_litmapNextID);
        m_idMap[node->getName()] = m_litmapNextID;

        _outputDebugStr( "AOBake: node[%s] <= %d, %s\n", node->getName().c_str(), m_litmapNextID, resName );

        // next id ...
        ++m_litmapNextID;
    }

    void AOBake::onSetupLightMapProcess( LightMapProcess* lmProcess )
    {
        m_lmProcess = lmProcess;
    }

    float AOBake::_aoCalc( const SphereSamples::Sample& smpl, TempContext* tc )
    {
        const float e_dot = 0.00873f; // 89.5�� 
        const float e_ray = 0.001f;

        float dot = smpl.dir.dotProduct( tc->norm );

        if ( dot > e_dot ) // ��������
        {
            if ( m_maxRayLenUsed ) // �޶�����
            {
                LimitRay ray( tc->pos + smpl.dir * e_ray, smpl.dir, m_maxRayLen ); // �����߷����ȥһЩ�����������

                float t = 0;
                if ( !rayIntersectSGDetail( m_bakeSys.getSG(), ray, 0, 0, &t ) ) // ����
                {
                    return dot;
                }
                else // �ڱ�
                {
                    t = Math::clamp( t, 0.0f, m_maxRayLen );
                    float m = (m_maxRayLen * m_falloff) / t;
                    float g_t = Math::clamp( (1 - m) / (1 - m_falloff), 0.0f, 1.0f );
                    return g_t * dot;
                }
            }
            else  // ���޳����߷���
            {
                Ray ray( tc->pos + smpl.dir * e_ray, smpl.dir ); // �����߷����ȥһЩ�����������

                if ( !rayIntersectSGDetail( m_bakeSys.getSG(), ray ) ) // ����
                    return dot;
                else // �ڱ�
                    return 0;
            }
        }

        return 0; // ������
    }

    void AOBake::_aoCalcStatic( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt )
    {
        TempContext* tc = (TempContext*)(context);
        float val = tc->baker->_aoCalc( smpl, tc );
        *vals = val;
    }

    void AOBake::onBakePerTexel( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv )
    {
#if 0
        // ����ao
        // WARNING: �����Ƕ��߳�����
        TempContext tc;
        tc.baker = this;
        tc.pos   = pos;
        tc.norm  = norm;

        float aor = 0;

        SphereSampleMapper mapper;
        mapper.setupAll( &m_rands, 0, 1, BAKE_DEFAULT_SAMPLETYPE,
            norm, Vector3::ZERO, 0, true, false );
        mapper.general();

        SphereSamples theSS;
        theSS.setSamples( &mapper );
        theSS.integrate( _aoCalcStatic, &tc, &aor, 1 );

        aor /= Math::PI;

#endif

#if 0
        // �򵥼������ģʽ
        Vector3 litDir = -Vector3(-0.7f, -1, -0.5f);//Vector3(0,5,0) - pos;//
        litDir.normalise();
        float aor = litDir.dotProduct( norm );
        if ( aor < 0 )
            aor = 0;
#endif

#if 1
        // �򵥲���ģʽ
        float aor = 1;
#endif

        // �浽litmap
        m_litMap->setColor( xp, yp, Color(aor, aor, aor) );
        m_litMap->setUsed( xp, yp, true );
    }
}

