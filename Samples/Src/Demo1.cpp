#include "Demo1.h"
#include <KhaosRoot.h>
#include <KhaosObjImport.h>
#include <KhaosMeshFile.h>
#include <KhaosGlossyTool.h>
#include <KhaosBakeUtil.h>
#include <KhaosAOBake.h>
#include <KhaosIrrBake.h>
#include <KhaosDist2Alpha.h>

using namespace Khaos;

#ifdef _DEMO1_ACTIVE
    SampleFrame* createSampleFrame()
    {
        return KHAOS_NEW Demo1Impl;
    }
#endif

//////////////////////////////////////////////////////////////////////////
static const String preName = "/sponza/";

Demo1Impl::Demo1Impl()
{
}

Demo1Impl::~Demo1Impl()
{
}

void Demo1Impl::_setupCamera()
{
    SceneGraph* sg = g_root->getSceneGraph( "demo" );
    AreaNode* area = sg->getRootNode( "root" );

    //////////////////////////////////////////////////////////////////////////
    // render setting

#define INIT_FEATURE_DISABLE 0

    RenderSettings* mainSettings = g_renderSettingManager->createRenderSettings( "mainSettings" );
    mainSettings->setRenderMode( /*RM_FORWARD*/RM_DEFERRED_SHADING );

#if INIT_FEATURE_DISABLE
    //mainSettings->setEnableNormalMap( false );
#endif

#if 1
    SSAORenderSetting* ssaoSetting = mainSettings->createSetting<SSAORenderSetting>();
    ssaoSetting->setDiskRadius( 1.5f );
    ssaoSetting->setSmallRadiusRatio( 0.5f );
    ssaoSetting->setLargeRadiusRatio( 1.5f );
    ssaoSetting->setAmount( 2.5f );
#if INIT_FEATURE_DISABLE
    ssaoSetting->setEnabled( false );
#endif

#endif

    HDRSetting* hdrSetting = mainSettings->createSetting<HDRSetting>();
    //hdrSetting->setBrightLevel( 2 );
    //hdrSetting->setBrightThreshold( 5.0f );
#if INIT_FEATURE_DISABLE
    hdrSetting->setEnabled( false );
#endif

    AntiAliasSetting* aaSetting = mainSettings->createSetting<AntiAliasSetting>();
#if INIT_FEATURE_DISABLE
    aaSetting->setEnabled( false );
#endif

    //////////////////////////////////////////////////////////////////////////
    // camera
    CameraNode* cam1 = sg->createSceneNode<CameraNode>("mainCamera");
    //cam1->lookAt( Vector3(0, 1.6f, 20), Vector3(0,0,0), Vector3::UNIT_Y );
    cam1->lookAt( Vector3(8.63329029f, 6.76517773f, -0.184850454f), 
        Vector3(-90.3899307f, -7.17270947f, 0.729985476f),
        Vector3::UNIT_Y );
    cam1->getCamera()->setPerspective( Math::toRadians(60), 4.0f/3.0f, 0.05f, 50.0f );
    cam1->getCamera()->setRenderSettings( mainSettings );
    area->addChild( cam1 );

    g_root->addActiveCamera( cam1 );

    //////////////////////////////////////////////////////////////////////////
    // operation
    m_samOp.init( cam1, 5.0f, 0.01f );
}


static void _createSimpleRefScene( SceneGraph* sg, AreaNode* area )
{
    //////////////////////////////////////////////////////////////////////////
    // material
    MaterialPtr mtrDiff( MaterialManager::createMaterial( "mtr_diff" ) );
    mtrDiff->setNullCreator();
    mtrDiff->useAttrib<BaseColorAttrib>()->setValue( Color(0.88f, 0.78f, 0.6f, 1.0f) );
    mtrDiff->useAttrib<BaseMapAttrib>()->setTexture( "grass_1024.jpg" );

    //////////////////////////////////////////////////////////////////////////
    // mesh
    MeshPtr meshPlane( MeshManager::createPlane( "mesh_plane", 200, 200, 30, 30 ) );
    meshPlane->load( false );
    meshPlane->setMaterialName( "mtr_diff" );

    //////////////////////////////////////////////////////////////////////////
    // mesh node
    MeshNode* objPlane = sg->createSceneNode<MeshNode>("objPlane");
    objPlane->setMesh( "mesh_plane" );
    objPlane->setPosition( 0, 0, 0 );
    objPlane->setReceiveShadow( true );
    area->addChild( objPlane );
}

static void _materialImportTest()
{
    ObjMtlResImporter omri;
    omri.setSpecMapArbiVal( 1.0f * 2.5f );
    omri.setOpacityMapVal( 4, 0.5f );
    omri.setResBase( "/sponza" );
    omri.import( "sponza/sponza.mtl" );

    // glossy tool test
    RsCmdPtr cmd(KHAOS_NEW GlossyAABatchCmd);
    //g_renderSystem->addPostEffRSCmd( cmd );
}

static void _meshImportTest( SceneNode* objSponza )
{
    ObjSceneImporter osi;
    osi.setMeshPreName( "/sponza/" );
    osi.setDefaultMaterialName( "mtr_comm" );
    osi.setResBase( "/sponza/" );
    osi.setShadow( true, true );
    osi.import( "sponza/sponza.obj", objSponza );
}

static void _maskTextureTest()
{
    static const char* oriname[] = 
    {
        "chain_texture_mask.tga",
        "sponza_thorn_mask.tga",
        "vase_plant_mask.tga"
    };

    for ( int i = 0; i < KHAOS_ARRAY_SIZE(oriname); ++i )
    {
        String oldName = String("/sponza/textures/_ori_") + oriname[i];
        String newName = String("/sponza/textures/") + oriname[i];

        Texture* tex = TextureManager::getOrCreateTexture(oldName);
        tex->load( false );

        Dist2AlphaConfig cfg;
        cfg.spread = 10.0f;
        cfg.flag = Dist2AlphaConfig::R_ENABLED;
        cfg.destTexFmt = PIXFMT_L8;
        cfg.destWidth = tex->getWidth();
        cfg.destHeight = tex->getHeight();
        cfg.mipmapGenMinSize = 16;
        cfg.mipmapReadSize = TexObjLoadParas::MIP_FROMFILE;
        cfg.mipmapFilter = TEXF_LINEAR;
        cfg.sRGBWrite = 0;

        dist2Alpha( tex, newName, cfg );
    }
}

static void _generalUV2( SceneGraph* sg )
{
    // 开始
    AreaNode* area = sg->getRootNode("root");

    //  唯一化场景，并生成uv2
    SceneBakeUniquify sbu;
    sbu.process( sg, SceneBakeUniquify::SBU_NEED_UV2 );
    sbu.save( preName+"sponza_sbu.data", SceneBakeUniquify::SBU_SAVE_ALL );
}

static void _bakeScene( SceneGraph* sg, SceneBakeUniquify& sbu )
{
    // build bake info
    SimpleBakeInputData sbid;

    KHAOS_FOR_EACH_CONST( SceneBakeUniquify::NewMeshInfoMap, sbu.getNewMeshInfoMap(), it )
    {
        const String& nodeName = it->first;
        const SceneBakeUniquify::NewMeshInfo& info = it->second;

        NodeBakeInfo nbi;
        nbi.textureWidth  = info.bakeTextureWidth;
        nbi.textureHeight = info.bakeTextureHeight;

        sbid.addBakeInfo( sg->getSceneNode(nodeName), nbi );
    }

#if 0
    AOBake aoBake;
    aoBake.setResPreName( preName+"textures/dlms/ao_%d.dds", 0 );
    aoBake.general( sg, &sbid );
#endif

#if 1
    IrrBake irrBake;
    irrBake.useDirectional( true );
    irrBake.setPreBVHName( "e:/_baketmp"+preName+"sponza.pbvh" );
    irrBake.setPreBuildName( "e:/_baketmp"+preName+"irr_%d.pb", 1 );
    irrBake.setResPreName( preName+"textures/dlms/irr_%da.dds", preName+"textures/dlms/irr_%db.dds", 0 );
    irrBake.setPhase( IrrBake::PHASE_LITMAP_ONLY );
    irrBake.general( sg, &sbid, 2 );
#endif
}

bool Demo1Impl::_onCreateScene()
{
    SceneGraph* sg = g_root->createSceneGraph( "demo" );

    // area
    AreaNode* area = sg->createRootNode( "root" );
    float h = 10000.0f;
    AxisAlignedBox aabb( -h,-h,-h, h,h,h );
    area->getArea()->initOctree( aabb, 8 );

    // 控制参数
    bool setup_simple_scene = 0;
    bool setup_material_import_test = 0;
    bool setup_mask_texture_fix = 0;
    bool setup_mesh_import_test = 0;
    bool setup_calc_tangent = 0;
    bool setup_genuv2 = 0;
    bool setup_bake = 0;
    bool setup_usenewmesh = 0;
    bool setup_enable_direct_light = 1;
    bool setup_enable_point_light = 1;
    bool setup_enable_amb_light = 1;
    bool setup_enable_lightmap = 0;

    int test_bake_phase = 0;

    if ( test_bake_phase == 1 )
    {
        setup_bake = 1;
        setup_enable_direct_light = 1;
        setup_enable_point_light = 0;
        setup_enable_amb_light = 0;
        setup_enable_lightmap = 0;
    }
    else if ( test_bake_phase == 2 )
    {
        setup_bake = 0;
        setup_enable_direct_light = 0;
        setup_enable_point_light = 0;
        setup_enable_amb_light = 0;
        setup_enable_lightmap = 1;
    }

    // 创建简单参照场景
    if ( setup_simple_scene )
        _createSimpleRefScene( sg, area );

    // 材质导入测试
    if ( setup_material_import_test )
        _materialImportTest();

    if ( setup_mask_texture_fix )
        _maskTextureTest();

    // 创建sponza场景
    SceneNode* objSponza = sg->createSceneNode<SceneNode>( "sponzaGroup" );
    objSponza->setPosition( Vector3(0, 1.2f, 0) );
    objSponza->setScale( 0.01f );
    area->addChild( objSponza );

    // 几何导入测试
    if ( setup_mesh_import_test )
        _meshImportTest( objSponza );

    // 场景加载
    SceneBakeUniquify sbu;

    if ( setup_usenewmesh ) // 是否新场景
    {
        sbu.load( preName+"sponza_sbu.data" );
    }
    
    int lightmapid = 0;

    for ( int i = 0; i < 382; ++i )
    {
        if ( i == 3 ) // 中间的布，忽略
            continue;

        String meshName = "/sponza/" + intToString(i) + ".mesh";
        MeshNode* node = sg->createSceneNode<MeshNode>( meshName );

        if ( setup_usenewmesh ) // 使用带第二套uv的新mesh
            meshName = sbu.getNewMeshInfoMap()[meshName].newMeshName;

        node->setMesh( meshName );

        if ( setup_calc_tangent )// 计算切线
        {   
            Math::setGramSchmidtOrthogonalizeFlip( true );
            node->getMesh()->generateTangents( true );
            Math::setGramSchmidtOrthogonalizeFlip( false );

            MeshExporter exp;
            exp.exportMesh( node->getMesh()->getResFileName(), node->getMesh() );
        }

        node->setCastShadow( true );
        node->setReceiveShadow( true );

        if ( setup_enable_lightmap )
        {
            char buf[1024];
            LightmapItem* item = sg->getInstSharedDataMgr().createLightmap(lightmapid);
#if 0
            item->setType( LightmapItem::LMT_BASIC );
            sprintf_s( buf, (preName+"textures/dlms/ao_%d.dds").c_str(), lightmapid );
            item->getMap().setName( buf );
#else
            item->setType( LightmapItem::LMT_FULL );
            sprintf_s( buf, (preName+"textures/dlms/irr_%da.dds").c_str(), lightmapid );
            item->getMap().setName( buf );
            sprintf_s( buf, (preName+"textures/dlms/irr_%db.dds").c_str(), lightmapid );
            item->getMapB().setName( buf );
#endif
            node->setLightMapID( lightmapid );
            ++lightmapid;
        }
        

        objSponza->addChild( node );
    }

    //////////////////////////////////////////////////////////////////////////
    // light node
    LightNode* lit1 = sg->createSceneNode<LightNode>("lit1");
    lit1->lookAt( Vector3(0, 0, 0), Vector3(0, -1, -0.2f), Vector3::UNIT_Y ); // -0.2f;-0.55f
    lit1->setLightType( LT_DIRECTIONAL );
    lit1->getLight()->setDiffuse( Color(2.95f, 2.95f, 2.95f) * 1 );
    lit1->getLight()->setShadowType( ST_PSSM );
    lit1->getLight()->setCascadesCount( 3 );
    lit1->getLight()->setShadowResolution( 1024 );
    lit1->getLight()->setShadowTapGroups( 4 );
    lit1->getLight()->setShadowVisableRange( 50.0f );
    lit1->getLight()->setShadowFadeRange( 45.0f );
    lit1->getLight()->setShadowSplitSchemeWeight( 0.5f );
    lit1->getLight()->setShadowBlurSize( 1.0f );
    lit1->getLight()->setShadowStrength( 1.0f );
    lit1->getLight()->setShadowRndSmplCovers( 8 );
    //lit1->getLight()->setShadowEnabled( false );
    lit1->setEnabled( setup_enable_direct_light );
    area->addChild( lit1 );

    LightNode* lit2 = sg->createSceneNode<LightNode>("lit2");
    lit2->setPosition( -8, 2, 0 );
    lit2->setLightType( LT_POINT );
    lit2->getLight()->setDiffuse( Color(0.41f, 0.42f, 0.39f) );
    lit2->getLightAs<PointLight>()->setRange( 10.0f );
    lit2->getLightAs<PointLight>()->setFadePower(1);
    lit2->setEnabled( setup_enable_point_light );
    area->addChild( lit2 );

    // sky
    SkyEnv& sky = sg->getSkyEnv();
    sky.setAmbConstClr( Color(0.04f, 0.04f, 0.04f) );
    //sky.setAmbUpperClr( Color(0.1f, 0.1f, 0.2f) );
    //sky.setAmbLowerClr( Color(0.1f, 0.2f, 0.1f) );
    sky.setEnabled( setup_enable_amb_light ? SkyEnv::SKY_SIMPLEAMB : SkyEnv::SKY_NULL );

    // 更新一次为bake准备
    area->_updateDerived();

    // 生成第二套uv
    if ( setup_genuv2 )
        _generalUV2( sg );

    // 场景烘焙
    if ( setup_bake )
        _bakeScene( sg, sbu );

    // setup camera and config
    _setupCamera();

    return true;
}

void Demo1Impl::onKeyUp( int key ) 
{
    _processKeyCommon( key );

    if ( key == 'Y' )
    {
         SceneGraph* sg = g_root->getSceneGraph( "demo" );
         CameraNode* cam1 = (CameraNode*)sg->getSceneNode("mainCamera");

         Vector3 eye = cam1->getPosition();
         Vector3 look = cam1->getOrientation() * Vector3::NEGATIVE_UNIT_Z;
         Vector3 target = eye + look * 100;

         sg = 0;
    }
}

bool Demo1Impl::_onUpdate()
{
    if ( !SampleFrame::_onUpdate() )
        return false;

    SceneGraph* sg = g_root->getSceneGraph( "demo" );
    return true;
}

