#include "KhaosPreHeaders.h"
#include "KhaosIrrBake.h"
#include "KhaosMesh.h"
#include "KhaosMeshNode.h"
#include "KhaosMaterialManager.h"
#include "KhaosTextureManager.h"
#include "KhaosTexCfgParser.h"
#include "KhaosMaterialFile.h"
#include "KhaosRayQuery.h"
#include "KhaosTimer.h"
#include "KhaosLightNode.h"
#include "KhaosVolumeProbeNode.h"
#include "KhaosSceneGraph.h"
#include "KhaosInstObjSharedData.h"
#include "KhaosNameDef.h"

namespace Khaos
{
    const float e_ray_off = 0.01f;
    const float e_norm_off = 0.01f;

    //////////////////////////////////////////////////////////////////////////
    IrrBake::IrrBake() : m_lmProcess(0), m_phase(PHASE_ALL), m_status(0), m_currIterator(0), m_maxIterator(0), 
        m_isAmbSet(false), m_useDirectional(false),
        m_preHeadFile(0), m_preVolHeadFile(0), m_onlyUse(false), m_onlyUseVol(false),
        m_currNode(0), m_currLitMapSet(0), m_currMtr(0), m_currVolProbNode(0), m_currSubIdx(0), m_currLastLightMapPass(false),
        m_litmapNextID(0), m_volmapNextID(0),
        m_debugTimer(0)
    {
        KHAOS_CLEAR_ARRAY(m_preFile);
        KHAOS_CLEAR_ARRAY(m_preVolFile);
        KHAOS_CLEAR_ARRAY(m_sphMappers);

        const int hemi_cnt = 35 * 35;

        {
            FLMRandomStream rs;
            m_randHemi.general( hemi_cnt, RandSamples::RT_Hammersley, &rs );
        }

        {
            FLMRandomStream rs;
            m_randFull.general( hemi_cnt * 2, RandSamples::RT_Hammersley, &rs );
        }
    }

    IrrBake::~IrrBake()
    {
        _freeAllLitMapSet();
        _freeAllVolMapSet();
        _cleanSphereMappers();
    }

    void IrrBake::setPreBVHName( const String& dataPath )
    {
        m_bvhPreFileName = dataPath;
    }

    void IrrBake::setPreBuildName( const String& dataPath, bool onlyUse )
    {
        m_prebuildDataPath = dataPath; 
        m_onlyUse = onlyUse;
    }

    void IrrBake::setPreVolBuildName( const String& dataPath, bool onlyUse )
    {
        m_prebuildVolDataPath = dataPath;
        m_onlyUseVol = onlyUse;
    }

    void IrrBake::setResPreName( const String& nameA, const String& nameB, int nextID )
    {
        m_resBaseAName = nameA;
        m_resBaseBName = nameB;
        m_litmapNextID = Math::maxVal( nextID, 0 );
    }

    void IrrBake::setResVolName( const String nameFmt, int nextID )
    {
        m_resBaseVolName = nameFmt;
        m_volmapNextID = Math::maxVal( nextID, 0 );
    }
    
    void IrrBake::useDirectional( bool en )
    {
        m_useDirectional = en;
    }

    void IrrBake::setAmbient( const Color& clr )
    {
        // ��ʼ�㶨�������趨
        m_clrAmb = clr;
        m_isAmbSet = clr.getRGB() != Color::BLACK;
    }

    void IrrBake::setPhase( uint phase )
    {
        m_phase = phase;
    }

    void IrrBake::_initSphereMappers()
    {
        SphereSampleMapper::DistributionType dtype = SphereSampleMapper::HemiSphereUniform;
        float limitTheta = Math::toRadians(89.0f); // ������89�㣬������Լ�ƽ������ཻ

        m_sphInitMapper.setRandsDistribution( &m_randHemi, dtype, limitTheta );
        m_sphInitMapper.setDiffuseBRDFParas( Vector3::UNIT_Y );
        m_sphInitMapper.setCommConfig( true, true );
        m_sphInitMapper.general();

        for ( int i = 0; i < m_bakeSys.getThreadsCount(); ++i )
        {
            m_sphMappers[i] = KHAOS_NEW SphereSampleMapper;
            m_sphMappers[i]->setRandsDistribution( &m_randHemi, dtype, limitTheta );
            m_sphMappers[i]->setDiffuseBRDFParas( Vector3::UNIT_Y );
            m_sphMappers[i]->setCommConfig( true, true );
            m_sphMappers[i]->general();
        }
    }

    void IrrBake::_cleanSphereMappers()
    {
        for ( int i = 0; i < m_bakeSys.getThreadsCount(); ++i )
            KHAOS_DELETE m_sphMappers[i];
    }

    void IrrBake::general( SceneGraph* sg, IBakeInputData* id, int iterCnt )
    {
        KHAOS_DEBUG_TEST_TIMER("IrrBake::general")

        m_bakeSys.init( sg, id, this, BakeSystem::BSF_PER_TEXEL, BAKE_DEFAULT_THREAD );

        _initSphereMappers();

        bool hasBakeLightmap = (m_phase & PHASE_LITMAP_ONLY) != 0;
        bool hasBakeVolumeProbes = (m_phase & PHASE_VOLUME_ONLY) != 0;

        bool needRayTestByLM = hasBakeLightmap && !m_onlyUse;
        bool needRayTestByVM = hasBakeVolumeProbes && !m_onlyUseVol;

        m_bakeSys.getSG()->initSceneBVH();

        // ׼������
        _outputDebugStr( "IrrBake: prepare ...\n" );
        if ( !m_bakeSys.prepare() )
            return;
        
        if ( !m_onlyUse && !m_onlyUseVol ) // �״Σ���Ҫ����bvh
        {
            m_bakeSys.getSG()->getBVH()->build();
            m_bakeSys.getSG()->getBVH()->saveFile( m_bvhPreFileName );
        }
        else // ���Ѿ��еĴ�
        {
            m_bakeSys.getSG()->getBVH()->openFile( m_bvhPreFileName );

            for ( size_t i = 0; i < m_bakeSys.getSG()->getBVH()->getSceneNodeNames().size(); ++i )
            {
                const String& name = m_bakeSys.getSG()->getBVH()->getSceneNodeNames()[i];
                m_bakeSys.getSG()->getBVH()->_addSceneNodeForName( sg->getSceneNode(name) );
            }

            m_bakeSys.getSG()->getBVH()->_completeAddSceneNodeForName();
        }

        // ����Ԥ�����ļ�
        if ( needRayTestByLM ) // lightmapԤ����
        {
            _outputDebugStr( "IrrBake: lightmap prebuild ...\n" );
            m_status = IBS_PREBUILD;
            m_bakeSys.bakeOnce();
            m_onlyUse = true; // �決���ˣ���ô�����½׶�����ʹ��
        }

        if ( needRayTestByVM ) // volumemapԤ����
        {
            _outputDebugStr( "IrrBake: volume probes build prebuild ...\n" );
            m_status = IBS_PREBUILD;
            _buildAllVolumeProbes();
            m_onlyUseVol = true;
        }

        LightMapPreBuildFile::startBuildThread();

        // �Ƿ��л������еĻ����Ⱥ決�ϻ�����
        if ( m_isAmbSet )
        {
            _outputDebugStr( "IrrBake: init amb ...\n" );
            m_status = IBS_INIT_AMB;
            m_bakeSys.bakeOnce();
            _postFilter();
        }

        // �決��ʼ����ȡemit��ֱ�ӹ���
        {
            _outputDebugStr( "IrrBake: init emit ...\n" );
            m_status = IBS_INIT_EMIT;
            m_bakeSys.bakeOnce();
            _postFilter();
        }

        // ��ʼ������ӣ���'����'lightmap
        if ( hasBakeLightmap )
        {
            _outputDebugStr( "IrrBake: bake next start ...\n" );
            m_status = IBS_NEXT;
            m_maxIterator = iterCnt;

            for ( int m_currIterator = 0; m_currIterator < m_maxIterator; ++m_currIterator )
            {
                _outputDebugStr( "IrrBake: bake next at [%d]...\n", m_currIterator );
                m_currLastLightMapPass = (m_currIterator == m_maxIterator-1);
                _initNextBake();
                m_bakeSys.bakeOnce();
                _postFilter();
            }

            // ����
            _outputDebugStr( "IrrBake: bake lightmap finish done ...\n" );
            _finishLitMapDone();
        }
        else
        {
            if ( hasBakeVolumeProbes ) // ��volume probes�決��Ҫ���ȡ�Ѿ��決��lightmap
                _readAllIndIrrLitMaps();
        }

        // �決volume probes
        if ( hasBakeVolumeProbes )
        {
            _outputDebugStr( "IrrBake: bake volume probes start ...\n" );
            _initProbeSampleInfos();

            m_status = IBS_VOLPROB;
            _buildAllVolumeProbes();

            _outputDebugStr( "IrrBake: bake volume probes finish done ...\n" );
            _finishVolMapDone();
        }

        LightMapPreBuildFile::shutdownBuildThread();
    }

    void IrrBake::_initNextBake()
    {
        KHAOS_FOR_EACH( LitMapSetMap, m_litMapSet, it )
        {
            LitMapSet* lms = it->second;
            lms->swapPreNext(); // next -> pre
            lms->indIrrNext->clearAll( Color::BLACK ); // next -> 0
            lms->indIrrNext->clearUsed();
        }
    }

    void IrrBake::_postFilter()
    {
        vector<LitMapSet*>::type mapList;

        KHAOS_FOR_EACH( LitMapSetMap, m_litMapSet, it )
        {
            mapList.push_back( it->second );
        }

        void* context[2] = { this, &mapList };

        BatchTasks tasks;
        tasks.setTask( _filterResultStatic );
        tasks.setUserData( context );
        tasks.planTask( (int)mapList.size(), m_bakeSys.getThreadsCount() );
    }

    void IrrBake::_filterResultStatic( int threadID, void* para, int mapIdx )
    {
        void** context = (void**)para;

        IrrBake* baker = (IrrBake*)(context[0]);
        vector<LitMapSet*>::type* mapList = (vector<LitMapSet*>::type*)(context[1]);

        LitMapSet* lms = (*mapList)[mapIdx];
        baker->_filterResult( lms );
    }

    void IrrBake::_filterResult( LitMapSet* lms )
    {
        if ( m_status == IBS_INIT_AMB ) // amb
        {
            LightMapRemoveZero lmRem;
            lmRem.filter( lms->initAmb, _getTexelBlackLumin, 25.0f/255.0f, 55.0f/255.0f, true );

            LightMapPostFill lmFill;
            lmFill.fill( lms->initAmb, 6 );
        }
        else if ( m_status == IBS_INIT_EMIT ) // emit
        {
            LightMapRemoveZero lmRem;
            lmRem.filter( lms->initEmit, _getTexelBlackLumin, 25.0f/255.0f, 1.0f/255.0f, false );

            LightMapPostFill lmFill;
            lmFill.fill( lms->initEmit, 6 );
        }
        else  if ( m_status == IBS_NEXT ) // next
        {
            // directional light map
            if ( m_useDirectional ) // ���������ǰ����indIrrNext������֮ǰ��������Ϣ
            {
                LightMapRemoveZero lmRem;
                lmRem.setUserData( lms->indIrrNext ); // ��Ϊ�ο�
                lmRem.filter( lms->bentNormalMap, _getTexelDLMBlackLumin, 25.0f/255.0f, 1.0f/255.0f, false );

                LightMapPostFill lmFill;
                lmFill.fill( lms->bentNormalMap, 6 );
            }

            // common light map
            {
                LightMapRemoveZero lmRem;
                lmRem.filter( lms->indIrrNext, _getTexelBlackLumin, 25.0f/255.0f, 1.0f/255.0f, false );

                LightMapPostFill lmFill;
                lmFill.fill( lms->indIrrNext, 6 );
            }
        }
    }

    Vector3 IrrBake::_getTexelBlackLumin( LightMapRemoveZero* fiter, ILightMapSource* lmSrc, int x, int y )
    {
        SimpleLightMap* litMap = static_cast<SimpleLightMap*>(lmSrc);
        Color clr = litMap->getColor(x, y);
        return Vector3(clr.r, clr.g, clr.b);
    }

    Vector3 IrrBake::_getTexelDLMBlackLumin( LightMapRemoveZero* fiter, ILightMapSource* lmSrc, int x, int y )
    {
        SimpleLightMap* litMap = static_cast<SimpleLightMap*>(fiter->getUserData());
        Color clr = litMap->getColor(x, y);
        return Vector3(clr.r, clr.g, clr.b);
    }

    void IrrBake::_finishLitMapDone()
    {
        KHAOS_FOR_EACH( LitMapSetMap, m_litMapSet, it )
        {
            _saveResult( it->first, it->second );
        }
    }

    static void _saveLightMapFile( SimpleLightMap* litMapIndIrr, const String& resName, PixelFormat fmt )
    {
        if ( !litMapIndIrr )
            return;

        if ( resName.empty() )
            return;

        String resFullFileName = g_fileSystem->getFullFileName(resName);

        // дlightmap�����ļ�
        litMapIndIrr->saveFile( fmt, resFullFileName.c_str() ); // ���������ļ�
        TexCfgSaver::saveSimple( resName, TEXTYPE_2D, fmt, TEXADDR_MIRROR, TEXF_LINEAR, TEXF_NONE, false, 1 ); // �������������ļ�
    }

    void IrrBake::_saveResult( SceneNode* node, LitMapSet* lms )
    {
#if 0 // test direct light
        lms->indIrrNext->addLightMap( lms->initEmit, Color::WHITE, false );
        _saveLightMapFile( lms->indIrrNext, lms->outLightMapA, PIXFMT_A16B16G16R16F );
        _saveLightMapFile( lms->indIrrNext, lms->outLightMapB, PIXFMT_A16B16G16R16F );
        return;
#endif
        // ����Ҫ�������գ�����
        if ( m_isAmbSet )
        {
            lms->indIrrNext->addLightMap( lms->initAmb, m_clrAmb, false );
        }

        // ���浽�ļ�
        _saveLightMapFile( lms->indIrrNext, lms->outLightMapA, PIXFMT_A16B16G16R16F );

        _saveLightMapFile( lms->bentNormalMap, lms->outLightMapB, PIXFMT_A16B16G16R16F );

        _outputDebugStr( "IrrBake: node[%s] <= %d\n", node->getName().c_str(), lms->lightMapID );
    }

    void IrrBake::_readAllIndIrrLitMaps()
    {
        KHAOS_FOR_EACH( LitMapSetMap, m_litMapSet, it )
        {
            _readResult( it->first, it->second );
        }
    }

    void IrrBake::_readResult( SceneNode* node, LitMapSet* lms )
    {
        _readTextureToLitMap( lms->indIrrNext, lms->outLightMapA );
    }

    void IrrBake::_readTextureToLitMap( SimpleLightMap* litMap, const String& file )
    {
        TexturePtr obj( TextureManager::getOrCreateTexture( file ) );
        obj->load( false ); // ȷ������
        obj->_fetchReadData();

        int width = obj->getWidth();
        int height = obj->getHeight();

        for ( int y = 0; y < height; ++y )
        {
            for ( int x = 0; x < width; ++x )
            {
                const Color& clr = obj->_readTex2DPix( x, y );
                litMap->setColor( x, y, clr );
            }
        }
    }

    void IrrBake::onPrepareMesh( SceneNode* node, const NodeBakeInfo* info )
    {
        MeshNode* entity = static_cast<MeshNode*>(node);
        Mesh* mesh = entity->getMesh();

        // bvh����
        m_bakeSys.getSG()->getBVH()->addSceneNode( entity );

        // Ӧ���еڶ���uv
        if ( !mesh->getSubMesh(0)->getVertexBuffer()->hasElement( VFM_TEX2 ) )
        {
            khaosAssert(0);
            khaosLogLn( KHL_L2, "IrrBake::onPrepareMesh: %s no uv2", mesh->getName().c_str() );
        }

        if ( m_useDirectional )
        {
            if ( !mesh->getSubMesh(0)->getVertexBuffer()->hasElement( VFM_TAN ) )
            {
                khaosAssert(0);
                khaosLogLn( KHL_L2, "IrrBake::onPrepareMesh: %s no tangent", mesh->getName().c_str() );
            }
        }

        // ������lightmap���ԣ��ر���
        if ( entity->getLightMapID() >= 0 )
        {
            m_bakeSys.getSG()->getInstSharedDataMgr().removeLightmap( entity->getLightMapID() );
            entity->setLightMapID( InvalidInstSharedDataID );
        }

        // ׼�����ʶ�ȡ����
        for ( int i = 0; i < entity->getSubEntityCount(); ++i )
        {
            entity->getSubEntity(i)->getMaterial()->_prepareRead();
        }

        // ������Դ
        LitMapSet* lms = _getLitMapSet( node );
        lms->createLitMap( info->textureWidth, info->textureHeight, m_useDirectional, m_isAmbSet );

        char resNameA[1024] = {};
        sprintf( resNameA, m_resBaseAName.c_str(), m_litmapNextID );

        char resNameB[1024] = {};
        sprintf( resNameB, m_resBaseBName.c_str(), m_litmapNextID );

        lms->setOutMaps( resNameA, resNameB, m_litmapNextID );

        if ( m_prebuildDataPath.size() )
        {
            char preName[1024] = {};
            sprintf( preName, m_prebuildDataPath.c_str(), m_litmapNextID );
            lms->preFileName = preName;
        }
        
        ++m_litmapNextID;
    }

    void IrrBake::onPrepareLight( SceneNode* node )
    {
    }

    void IrrBake::onBeginBakeNode( int idx, SceneNode* node, const NodeBakeInfo* info )
    {
        if ( m_status == IBS_PREBUILD )
            m_debugTimer = KHAOS_NEW DebugTestTimer( "IrrBake: prebuild " + node->getName() + " finish" );

        _outputDebugStr( "IrrBake: bake node - %s[%d/%d]\n", 
            node->getName().c_str(), idx, m_bakeSys.getMeshNodeList().size() );

        m_currNode      = node;
        m_currLitMapSet = _getLitMapSet(node);

        // Ԥ����決
        bool needPreFile = false;
        bool onlyRead = false;

        if ( m_status == IBS_PREBUILD )
        {
            needPreFile = true;
            onlyRead = false;
        }

        // �決�׶�֧��Ԥ���ļ�
        if ( (m_status == IBS_INIT_AMB || m_status == IBS_NEXT) && m_onlyUse )
        {
            needPreFile = true;
            onlyRead = true;
        }

        // Ԥ���ļ���д����
        if ( needPreFile )
        {
            khaosAssert( !m_preHeadFile );
            int thread_count = m_bakeSys.getThreadsCount();
            int sample_count = m_randHemi.getNumSamples();
            int w = m_currLitMapSet->indIrrPre->getWidth();
            int h = m_currLitMapSet->indIrrPre->getHeight();
            _createPreBuildFiles( m_preHeadFile, m_preFile, m_currLitMapSet->preFileName, w, h, 0,
                thread_count, sample_count, onlyRead );
        }
    }

    void IrrBake::onEndBakeNode( SceneNode* node, const NodeBakeInfo* info )
    {
        if ( m_preHeadFile )
        {
            int count = m_bakeSys.getThreadsCount();
            for ( int i = 0; i < count; ++i )
            {
                KHAOS_DELETE m_preFile[i];
                m_preFile[i] = 0;
            }

            KHAOS_DELETE m_preHeadFile;
            m_preHeadFile = 0;
        }

        if ( m_debugTimer )
        {
            KHAOS_DELETE m_debugTimer;
            m_debugTimer = 0;
        }
    }

    void IrrBake::onSetupLightMapProcess( LightMapProcess* lmProcess )
    {
        m_lmProcess = lmProcess;
        m_lmProcess->setNeedTangent( m_useDirectional ); // directional lightmap��Ҫ������Ϣ

        bool needFaceTangent = true;

        switch ( m_status )
        {
        case IBS_INIT_EMIT:
        case IBS_VOLPROB:
            needFaceTangent = false;
        }

        m_lmProcess->setNeedFaceTanget( needFaceTangent ); // ��Ҫƽ���пռ�
    }

    void IrrBake::onBakePerSubMeshBegin( SubMesh* subMesh, int subIdx )
    {
        m_currSubIdx = subIdx;

        MeshNode* meshNode = static_cast<MeshNode*>(m_currNode);
        khaosAssert( subIdx < meshNode->getSubEntityCount() );
        SubEntity* subEntity = meshNode->getSubEntity( subIdx );
        m_currMtr = subEntity->getMaterial();

        //_outputDebugStr( "node[%s] sub[%d] mtr[%s]\n",
        //    meshNode->getName().c_str(),
        //    subIdx,
        //    m_currMtr->getName().c_str()
        //    );
        // ������ԣ�����submeshû�Ӧ�ô�m_currNode��ȡ������Ϣ
        //MaterialManager::getOrCreateMaterial( subMesh->getMaterialName() );
    }

    void IrrBake::onBakePerSubMeshEnd( SubMesh* subMesh )
    {
        m_currMtr = 0;
    }

    void IrrBake::onBakePerFaceBegin( int threadID, int faceIdx, const Vector3& faceTangent, const Vector3& faceBinormal, const Vector3& faceNormal ) 
    {
        switch ( m_status )
        {
        case IBS_INIT_EMIT:
        case IBS_VOLPROB:
            return;
        }

        m_currFaceIdx[threadID] = faceIdx;
        //m_currFaceTangentWorld[threadID] = faceTangent;
        //m_currFaceBinormalWorld[threadID] = faceBinormal;
        //m_currFaceNormalWorld[threadID] = faceNormal;

        // ���������Ϸ�����ת���淨��
        // ע������ȡ�淨�߶����ǲ�ֵ�㷨��
        m_sphMappers[threadID]->resetNormalFrom( faceNormal, m_sphInitMapper );
    }

    int IrrBake::onBakeDiscardTexel( int threadID, int xp, int yp ) 
    {
        if ( m_status == IBS_INIT_AMB || m_status == IBS_NEXT )
        {
            // �Ƿ���Ҫ�決�������
            int faceIdx = m_currFaceIdx[threadID];
            return m_preFile[threadID]->canReadItem( xp, yp, m_currSubIdx, faceIdx ) ? 1 : 0;
        }

        return -1;
    }

    void IrrBake::onBakeRegisterTexel( int threadID, int xp, int yp ) 
    {
        if ( m_status == IBS_PREBUILD )
        {
            // ע������
            int faceIdx = m_currFaceIdx[threadID];
            m_preFile[threadID]->registerCurrTexelItem( xp, yp, m_currSubIdx, faceIdx );
        }
    }

    void IrrBake::onBakePerTexel( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv )
    {
        if ( m_status == IBS_PREBUILD )
        {
            _bakePerTexel_Prebuild( threadID, xp, yp, pos, norm, tang, uv );
        }
        else if ( m_status == IBS_INIT_AMB )
        {
            _bakePerTexel_InitAmb( threadID, xp, yp, pos, norm, tang, uv );
        }
        else if ( m_status == IBS_INIT_EMIT )
        {
            _bakePerTexel_InitEmit( threadID, xp, yp, pos, norm, tang, uv );
        }
        else if ( m_status == IBS_NEXT )
        {
             _bakePerTexel_Irr( threadID, xp, yp, pos, norm, tang, uv );
        }
    }

    void IrrBake::_bakePerTexel_Prebuild( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv )
    {
        LightMapPreBuildFile* preFile = m_preFile[threadID];
        LightMapPreBuildFile::TexelItem* texelItem = preFile->setCurrTexelItem( xp, yp );
        
        // ���߳��п��ܲ�һ����Ҳ������ʱ�������һ����ʱ�ķ���������
        //LightMapPreBuildHeadFile::ItemHeadInfo* headInfo = m_preHeadFile->getItemInfo( xp, yp );
        //khaosAssert( headInfo->subIdxInMesh == m_currSubIdx );
        //khaosAssert( headInfo->faceIdxInMesh == m_currFaceIdx[threadID] );
        //khaosAssert( headInfo->threadID == threadID );

        // ��¼��ӹ�����Ҫ��ÿ���ߵĲ���
        SphereSampleMapper* curr_sphereMapper = m_sphMappers[threadID];

        for ( int i = 0; i < curr_sphereMapper->getNumSamples(); ++i )
        {
            const SphereSampleMapper::Sample& smpl = curr_sphereMapper->getSample(i);

            float dot = smpl.dir.dotProduct( norm );

            if ( dot > 1e-6f ) // ��Ȼ�������򣬵��Ƕ��㷨�ߺ�ƽ�淨�߿��ܲ�һ�£�����˵������ֻ���ǰ�����Ӽ�
            { 

            Ray ray( pos + smpl.dir * e_ray_off + norm * e_norm_off, smpl.dir ); // �����߷����ȥһЩ�����������

            SceneBVH::Result result;

            if ( m_bakeSys.getSG()->getBVH()->intersect( ray, result ) ) // �ͳ����ཻ
            {
                // ����ĶԷ�����Ϣ
                const SBVHFaceInfo* faceInfo = m_bakeSys.getSG()->getBVH()->getFaceInfo( result.faceID );

                // �����Ľڵ�
                SceneNode* nodeOther = m_bakeSys.getSG()->getBVH()->getSceneNode( faceInfo->instanceID );
                MeshNode* meshNodeOther = static_cast<MeshNode*>(nodeOther);

                // ��������mesh
                SubMesh* subMeshOther = meshNodeOther->getMesh()->getSubMesh( faceInfo->subIndex );

                // �����ľ���ķ���
                Vector3  normalOther = subMeshOther->getTriNormal( faceInfo->faceIdx, result.gravity );
                normalOther = meshNodeOther->getDerivedMatrix().transformAffineNormal( normalOther );

                // ���һ������
                LightMapPreBuildFile::RayInfo* rayInfo = texelItem->addRayInfo( i );
                rayInfo->globalFaceID = result.faceID; // ��¼ȫ����id

                if ( normalOther.dotProduct(smpl.dir) < 0 ) // ������ԣ�������������
                {
                    rayInfo->flag = 0; // ����
                }
                else // ��Ȼ�Ǳ��棬��������Ҳ��¼��������
                {
                    rayInfo->flag = LightMapPreBuildFile::RIFLAG_OTHER_BACK_SURFACE; // ��ʾ����
                }
            }
            
            } // end of dot > 0
        }

        // ���һ�����صļ�¼������д����֮��ɾ��֮����Ϊ�ڴ治��ֱ�Ӵ����ô����Ϣ
        preFile->writeCurrTexelItem();
    }

    void IrrBake::_tempContextBindPreFile( TempContext& tc, int threadID, int xp, int yp )
    {
        khaosAssert( m_onlyUse ); // ���Զ�ȡԤ�����ļ�

        tc.baker = this;
        tc.preFile = m_preFile[threadID];
        tc.texelItem = tc.preFile->readNextTexelItem(xp, yp);

        khaosAssert( tc.texelItem );
    }

    float IrrBake::_calcAOWithPre( const SphereSamples::Sample& smpl, TempContext* tc )
    {
        const LightMapPreBuildFile::TexelItem* texelItem = tc->texelItem;
        const LightMapPreBuildFile::RayInfo* rayInfo = texelItem->getRayInfo( smpl.idx );

        if ( rayInfo ) // �ڱ�
            return smpl.dir.dotProduct( tc->norm );
        else // ����
            return 0;
    }

    void IrrBake::_calcAOStatic( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt )
    {
        TempContext* tc = (TempContext*)(context);
        *vals = tc->baker->_calcAOWithPre( smpl, tc );
    }

    void IrrBake::_bakePerTexel_InitAmb( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv )
    {
        // ����ao
        // WARNING: �����Ƕ��߳�����
        TempContext tc(pos, norm, 0);
        _tempContextBindPreFile( tc, threadID, xp, yp );

        float aor = 0;

        SphereSamples theSS;
        theSS.setSamples( m_sphMappers[threadID] );
        theSS.integrate( _calcAOStatic, &tc, &aor, 1 );

        aor /= Math::PI; // for diffuse brdf
        aor = Math::maxVal(1.0f - aor, 0.0f); // ��Ԥ�����ļ�����ô�������������ڱε�������1-���ڱ�����Ϊ�ܿ��ŵ���

        // �����ɫ
        m_currLitMapSet->initAmb->setColor( xp, yp, Color(aor, aor, aor) );
        m_currLitMapSet->initAmb->setUsed( xp, yp, true );
    }

    void IrrBake::_bakePerTexel_InitEmit( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv )
    {
        // �õ�ĳ�ʼ����ͼ
        SimpleLightMap* initEmit = m_currLitMapSet->initEmit;

        Color totalVal;

        // �����������Է���
        totalVal = m_currMtr->_readEmissiveColor( uv );

        // �ۼƼ���ÿ���ƹ�����������䲢ת�����Է���
        const BakeSystem::SceneNodeList& litNodes = this->m_bakeSys.getLightNodeList();
        KHAOS_FOR_EACH_CONST( BakeSystem::SceneNodeList, litNodes, it )
        {
            LightNode* litNode = static_cast<LightNode*>(*it);
            
            // �Ƿ�û���ڵ������߿ɴ
            if ( litNode->isEnabled() &&
                 litNode->getLight()->_canArrivePos( m_bakeSys.getSG(), pos ) )
            {
                Color clrIrr;
                litNode->getLight()->_calcPointIrradiance( pos, norm, clrIrr );
                totalVal += clrIrr * m_currMtr->_readDiffuseColor( uv );
            }
        }

        // �������ֱ�ӹ���ת��Ϊ�Է���
        if ( m_isAmbSet ) 
        {
            Color clrAmbLight = m_currLitMapSet->initAmb->getColor( xp, yp ) * m_clrAmb;
            totalVal += clrAmbLight * m_currMtr->_readDiffuseColor( uv );
        }

        initEmit->setColor( xp, yp, totalVal );
        initEmit->setUsed( xp, yp, true ); // ��ʹû���߿ɴ����0���䣬Ҳ��ʹ�ù��õ�
    }

    void IrrBake::_calcIndIrrWithPre( const SphereSamples::Sample& smpl, TempContext* tc, float* vals )
    {
        Color* irrNexts = (Color*)(vals); // �Է�������

        const LightMapPreBuildFile::TexelItem* texelItem = tc->texelItem;
        const LightMapPreBuildFile::RayInfo* rayInfo = texelItem->getRayInfo( smpl.idx );

        if ( !rayInfo || rayInfo->isOtherBackFace() )
        {
            // ������,�������ཻ���߱���
            irrNexts[0] = Color::BLACK;
            if ( m_currLastLightMapPass )
                irrNexts[1] = Color::BLACK;
            return;
        }

        Ray ray( tc->pos + smpl.dir * e_ray_off + tc->norm * e_norm_off, smpl.dir ); // �����߷����ȥһЩ�����������

        SceneBVH::Result result;
        m_bakeSys.getSG()->getBVH()->intersectTriAlways( ray, rayInfo->globalFaceID, result ); // �ض��ʹ��������ཻ

        const SBVHFaceInfo* faceInfo = m_bakeSys.getSG()->getBVH()->getFaceInfo( rayInfo->globalFaceID );

        int subIdx = faceInfo->subIndex;
        SceneNode* node = m_bakeSys.getSG()->getBVH()->getSceneNode( faceInfo->instanceID );

        MeshNode* meshNode = static_cast<MeshNode*>(node);
        SubMesh*  subMesh  = meshNode->getMesh()->getSubMesh(subIdx);

        Material*  mtrOther = meshNode->getSubEntity(subIdx)->getMaterial();
        LitMapSet* lmsOther = _getLitMapSet(node); // ȡ����ͼ��

        Vector2 uv1 = subMesh->getTriUV( faceInfo->faceIdx, result.gravity ); // ���㴦��uv1
        Vector2 uv2 = subMesh->getTriUV2( faceInfo->faceIdx, result.gravity ); // ���㴦��uv2

        // irr = (other emit + other diffuse) * NdotL
        Color otherOutLight = lmsOther->initEmit->getColorByUV(uv2) + // other emit
            lmsOther->indIrrPre->getColorByUV(uv2) * mtrOther->_readDiffuseColor( uv1 ); // other diffuse
        
        // stand light map
        irrNexts[0] = otherOutLight * Math::maxVal( smpl.dir.dotProduct( tc->norm ), 0.0f );

        // ���һ�����������ǲż���directional��Ϣ
        if ( m_currLastLightMapPass )
        {
            // hl2 light map, here we only use lumin
            float luminIrr = otherOutLight.getLuminA();

            irrNexts[1].r = luminIrr * Math::maxVal( smpl.dir.dotProduct( tc->basisHLWorld[0] ), 0.0f );
            irrNexts[1].g = luminIrr * Math::maxVal( smpl.dir.dotProduct( tc->basisHLWorld[1] ), 0.0f );
            irrNexts[1].b = luminIrr * Math::maxVal( smpl.dir.dotProduct( tc->basisHLWorld[2] ), 0.0f );
            irrNexts[1].a = 1.0f;
        }
    }

    void IrrBake::_calcIndIrrStatic( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt )
    {
        TempContext* tc = (TempContext*)(context);
        tc->baker->_calcIndIrrWithPre( smpl, tc, vals );
    }

    inline void _makeTangentSpace( const Vector3& norm, const Vector4& tang, Matrix3& matTangentToWorld )
    {
        Vector3 binorm = norm.crossProduct( tang.asVec3() ) * tang.w;
        matTangentToWorld.fromAxes( tang.asVec3(), binorm, norm );
    }

    void IrrBake::_bakePerTexel_Irr( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv )
    {
        // �����ӹ���
        // WARNING: �����Ƕ��߳�����
        Vector3 basisHLWorld[3];

        // direct lightmap��Ҫ��Ϣ
        if ( m_useDirectional && m_currLastLightMapPass )
        {
            Matrix3 matTangentToWorld;
            //matTangentToWorld.fromAxes( 
            //m_currFaceTangentWorld[threadID],
            //m_currFaceBinormalWorld[threadID],
            //m_currFaceNormalWorld[threadID] );
            _makeTangentSpace( norm, tang, matTangentToWorld );

            basisHLWorld[0] = matTangentToWorld * HLMath::getHLBasis(0);
            basisHLWorld[1] = matTangentToWorld * HLMath::getHLBasis(1);
            basisHLWorld[2] = matTangentToWorld * HLMath::getHLBasis(2);
        }

        TempContext tc( pos, norm, basisHLWorld );
        _tempContextBindPreFile( tc, threadID, xp, yp );

        Color indirrs[2]; // init zero

        SphereSamples theSS;
        theSS.setSamples( m_sphMappers[threadID] );
        theSS.integrate( _calcIndIrrStatic, &tc, indirrs[0].ptr(), 8 );

        indirrs[0] /= Math::PI;
        
        // �����ɫ
        m_currLitMapSet->indIrrNext->setColor( xp, yp, indirrs[0] );
        m_currLitMapSet->indIrrNext->setUsed( xp, yp, true );

        if ( m_currLastLightMapPass )
        {
            indirrs[1] /= Math::PI;

            // ��һ����������֤���޷�����ͼʱ��ֵһ��
            // ͬʱ������������ܲ�����������ӷ촦���ղ���������
            float total_ori = indirrs[0].getLuminA();
            float total_hl = (indirrs[1].r + indirrs[1].g + indirrs[1].b) / 3.0f;
            float total_scale = total_ori / Math::maxVal(total_hl, 1e-6f);

            indirrs[1].r *= total_scale;
            indirrs[1].g *= total_scale;
            indirrs[1].b *= total_scale;

            m_currLitMapSet->bentNormalMap->setColor( xp, yp, indirrs[1] );
            m_currLitMapSet->bentNormalMap->setUsed( xp, yp, true );
        }
    }
}

