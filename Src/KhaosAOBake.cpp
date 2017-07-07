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

        // 应当有第二套uv
        khaosAssert( mesh->getSubMesh(0)->getVertexBuffer()->getDeclaration()->getID() & VFM_TEX2 );

        // 假如有lightmap属性，关闭先
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
        // 保存到文件
        //MeshNode* meshNode = static_cast<MeshNode*>(node);
        //String mainName, extName;
        //splitFileName( meshNode->getMesh()->getResFileName(), mainName, extName ); // 使用mesh文件名，存放和它相同目录

        char resName[1024] = {};
        sprintf( resName, m_resBaseName.c_str(), m_litmapNextID );
        m_nameMap[m_litmapNextID] = resName;

        String resFullFileName = g_fileSystem->getFullFileName(resName);

        // 先保存未后处理的调试文件
        //m_litMap->saveDebugTemp( (resName + String(".dbg")).c_str() ); // 保存debug

        // 过滤黑色边缘
        LightMapRemoveZero lmRem;
        lmRem.filter( m_litMap, _isAOBlack, 25.0f/255.0f, 55.0f/255.0f, true );

        // 填充边缘
        LightMapPostFill lmFill;
        lmFill.fill( m_litMap, 6 );

        // 后期模糊
        //SimpleLightMap mapFilter;
        //mapFilter.initARGB( m_litMap->getWidth(), m_litMap->getHeight(), Color(0.0f, 0.0f, 0.0f) );
        //LightMapPostBlur lmBlur;
        //lmBlur.filter( m_litMap, &mapFilter );

        // 写ao纹理文件
        PixelFormat fmt = PIXFMT_A8R8G8B8; //PIXFMT_16F PIXFMT_A16B16G16R16F PIXFMT_A8R8G8B8
        m_litMap->saveFile( fmt, resFullFileName.c_str() ); // 保存纹理文件
        TexCfgSaver::saveSimple( resName, TEXTYPE_2D, fmt, TEXADDR_MIRROR, TEXF_LINEAR, TEXF_NONE, false, 1 ); // 保存纹理描述文件

        // 增加属性
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
        const float e_dot = 0.00873f; // 89.5度 
        const float e_ray = 0.001f;

        float dot = smpl.dir.dotProduct( tc->norm );

        if ( dot > e_dot ) // 在正半球
        {
            if ( m_maxRayLenUsed ) // 限定长度
            {
                LimitRay ray( tc->pos + smpl.dir * e_ray, smpl.dir, m_maxRayLen ); // 往射线方向出去一些，避免痤疮问题

                float t = 0;
                if ( !rayIntersectSGDetail( m_bakeSys.getSG(), ray, 0, 0, &t ) ) // 开放
                {
                    return dot;
                }
                else // 遮蔽
                {
                    t = Math::clamp( t, 0.0f, m_maxRayLen );
                    float m = (m_maxRayLen * m_falloff) / t;
                    float g_t = Math::clamp( (1 - m) / (1 - m_falloff), 0.0f, 1.0f );
                    return g_t * dot;
                }
            }
            else  // 无限长射线方法
            {
                Ray ray( tc->pos + smpl.dir * e_ray, smpl.dir ); // 往射线方向出去一些，避免痤疮问题

                if ( !rayIntersectSGDetail( m_bakeSys.getSG(), ray ) ) // 开放
                    return dot;
                else // 遮蔽
                    return 0;
            }
        }

        return 0; // 负半球
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
        // 计算ao
        // WARNING: 这里是多线程运行
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
        // 简单计算光照模式
        Vector3 litDir = -Vector3(-0.7f, -1, -0.5f);//Vector3(0,5,0) - pos;//
        litDir.normalise();
        float aor = litDir.dotProduct( norm );
        if ( aor < 0 )
            aor = 0;
#endif

#if 1
        // 简单测试模式
        float aor = 1;
#endif

        // 存到litmap
        m_litMap->setColor( xp, yp, Color(aor, aor, aor) );
        m_litMap->setUsed( xp, yp, true );
    }
}

