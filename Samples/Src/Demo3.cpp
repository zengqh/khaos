#include "Demo3.h"
#include <KhaosPRT.h>
#include <KhaosMeshUtil.h>
#include <KhaosLightMapUtil.h>
#include <KhaosBakeUtil.h>
#include <KhaosAOBake.h>
#include <KhaosIrrBake.h>
#include <KhaosObjImport.h>

using namespace Khaos;

#ifdef _DEMO3_ACTIVE
    SampleFrame* createSampleFrame()
    {
        return KHAOS_NEW Demo3Impl;
    }
#endif


static const String preName = "/BakeTest/";

#define TEST_SCENE_BASIC    1
#define TEST_RAY            0
#define USE_SCENE_BVH       1
#define TEST_BAKE_TYPE      1

Demo3Impl::Demo3Impl()
{
}

Demo3Impl::~Demo3Impl()
{
}

void Demo3Impl::_onDestroyScene()
{
}

void Demo3Impl::_setupMaterials( map<String, MaterialPtr>::type& mtrs )
{
    const bool en_vtxclr    = false;
    const bool en_wireframe = false;

//#define _useclr(r,g,b) Color(1,1,1)
#define _useclr(r,g,b) Color(r,g,b)
#define _addmtr(m) mtrs[#m] = m

    const int _rich_color = 0;

    //////////////////////////////////////////////////////////////////////////
    // material
    MaterialPtr mtrDiff( MaterialManager::createMaterial( preName+"mtr_diff.mtr" ) );
    mtrDiff->setNullCreator();
    mtrDiff->useAttrib<BaseColorAttrib>()->setValue( _useclr(0.78f, 0.72f, 0.7f) );
    mtrDiff->useAttrib<DSpecularAttrib>()->setValue( 0.3f );
    if ( _rich_color )
    {
        mtrDiff->useAttrib<BaseColorAttrib>()->setValue( _useclr(1.f, 1.f, 1.f) );
        mtrDiff->useAttrib<BaseMapAttrib>()->setTexture( "mud.jpg" );
    }
    mtrDiff->useAttrib<NormalMapAttrib>()->setTexture( "mud_n.jpg" );
    mtrDiff->enableCommState( MtrCommState::VTXCLR, en_vtxclr );
    mtrDiff->setWireframe( en_wireframe );
    _addmtr(mtrDiff);


    MaterialPtr mtrRed( MaterialManager::createMaterial( preName+"mtr_red.mtr" ) );
    mtrRed->setNullCreator();
    mtrRed->useAttrib<BaseColorAttrib>()->setValue( _useclr(0.7f, 0.25f, 0.3f) );
    mtrRed->useAttrib<DSpecularAttrib>()->setValue( 0.3f );
    if ( _rich_color )
    {
        mtrRed->useAttrib<BaseColorAttrib>()->setValue( _useclr(0.97f, 0.25f, 0.15f) );
        mtrRed->useAttrib<BaseMapAttrib>()->setTexture( "wall2.dds" );
    }
    mtrRed->useAttrib<NormalMapAttrib>()->setTexture( "wall2_n.tga" );
    mtrRed->enableCommState( MtrCommState::VTXCLR, en_vtxclr );
    mtrRed->setWireframe( en_wireframe );
    _addmtr(mtrRed);


    MaterialPtr mtrGreen( MaterialManager::createMaterial( preName+"mtr_green.mtr" ) );
    mtrGreen->setNullCreator();
    mtrGreen->useAttrib<BaseColorAttrib>()->setValue( _useclr(0.34f, 0.7f, 0.25f) );
    mtrGreen->useAttrib<DSpecularAttrib>()->setValue( 0.3f );
    if ( _rich_color )
    {
        mtrGreen->useAttrib<BaseColorAttrib>()->setValue( _useclr(0.34f, 0.92f, 0.35f) );
        mtrGreen->useAttrib<BaseMapAttrib>()->setTexture( "mud.jpg" );
    }
    mtrGreen->useAttrib<NormalMapAttrib>()->setTexture( "mud_n.jpg" );
    mtrGreen->enableCommState( MtrCommState::VTXCLR, en_vtxclr );
    mtrGreen->setWireframe( en_wireframe );
    _addmtr(mtrGreen);


    MaterialPtr mtrBlue( MaterialManager::createMaterial( preName+"mtr_blue.mtr" ) );
    mtrBlue->setNullCreator();
    mtrBlue->useAttrib<BaseColorAttrib>()->setValue( _useclr(0.25f, 0.3f, 0.74f) );
    mtrBlue->useAttrib<DSpecularAttrib>()->setValue( 0.3f );
    if ( _rich_color )
    {
        mtrBlue->useAttrib<BaseColorAttrib>()->setValue( _useclr(0.15f, 0.2f, 0.94f) );
        mtrBlue->useAttrib<BaseMapAttrib>()->setTexture( "wall2.dds" );
    }
    mtrBlue->useAttrib<NormalMapAttrib>()->setTexture( "wall2_n.tga" );
    mtrBlue->enableCommState( MtrCommState::VTXCLR, en_vtxclr );
    mtrBlue->setWireframe( en_wireframe );
    _addmtr(mtrBlue);

    // test white
    MaterialPtr mtrWhite( MaterialManager::createMaterial( preName+"mtr_white.mtr" ) );
    mtrWhite->setNullCreator();
    mtrWhite->useAttrib<BaseColorAttrib>()->setValue( Color(1.0f, 1.0f, 1.0f) );
    _addmtr(mtrWhite);
}

void Demo3Impl::_setupOriginal( SceneGraph* sg )
{
    map<String, MaterialPtr>::type mtrs;
    _setupMaterials( mtrs );

    //////////////////////////////////////////////////////////////////////////
    // mesh
    MeshPtr meshPlane( MeshManager::createPlane( preName+"mesh_plane.mesh", 20.0f, 20.0f, 2, 2 ) );
    meshPlane->load( false );
    meshPlane->generateTangents( true );
    meshPlane->setMaterialName( mtrs["mtrDiff"]->getName() );

    MeshPtr meshBoxA( MeshManager::createBox( preName+"mesh_box_a.mesh", 5.0f, 5.0f, 5.0f, 2, 2, 2 ) );
    meshBoxA->load( false );
    meshBoxA->generateTangents( true );
    meshBoxA->setMaterialName( mtrs["mtrRed"]->getName() );

    MeshPtr meshBoxB( MeshManager::createBox( preName+"mesh_box_b.mesh", 3.0f, 3.0f, 3.0f, 2, 2, 2 ) );
    meshBoxB->load( false );
    meshBoxB->generateTangents( true );
    meshBoxB->setMaterialName( mtrs["mtrGreen"]->getName() );

    MeshPtr meshBoxC( MeshManager::createBox( preName+"mesh_box_c.mesh", 1.0f, 1.0f, 1.0f, 2, 2, 2 ) );
    meshBoxC->load( false );
    meshBoxC->generateTangents( true );

    MeshPtr meshSphereA( MeshManager::createSphere( preName+"mesh_sphere_a.mesh", 3.0f, 20 ) );
    meshSphereA->load( false );
    meshSphereA->generateTangents( true );
    meshSphereA->setMaterialName( mtrs["mtrBlue"]->getName() );

    MeshPtr meshTeaA( MeshManager::getOrCreateMesh( preName+"teapot_0.mesh" ) );
    meshTeaA->load( false );
    meshTeaA->setMaterialName( mtrs["mtrBlue"]->getName() );

    MeshPtr meshTeaB( MeshManager::getOrCreateMesh( preName+"teapot_1.mesh" ) );
    meshTeaB->load( false );
    meshTeaB->setMaterialName( mtrs["mtrBlue"]->getName() );

    //////////////////////////////////////////////////////////////////////////
    // mesh node
    map<String, String>::type meshNames;

    meshNames["objPlane"]   = meshPlane->getName();
    meshNames["objBoxA"]    = meshBoxA->getName();
    meshNames["objBoxB"]    = meshBoxB->getName();
    
    meshNames["objBoxC_Left"]  = meshBoxC->getName();
    meshNames["objBoxC_Right"] = meshBoxC->getName();
    meshNames["objBoxC_Front"] = meshBoxC->getName();
    meshNames["objBoxC_Top"]   = meshBoxC->getName();

    meshNames["objSphereA"] = meshSphereA->getName();
    meshNames["objTeaA"]    = meshTeaA->getName();
    meshNames["objTeaB"]    = meshTeaB->getName();

    _buildNodes( sg, meshNames );
}

void Demo3Impl::_setupNew( Khaos::SceneGraph* sg )
{
    map<String, MaterialPtr>::type mtrs;
    _setupMaterials( mtrs );

    map<String, String>::type meshNames;

    meshNames["objPlane"]   = preName + "mesh_plane_0.mesh";
    meshNames["objBoxA"]    = preName + "mesh_box_a_0.mesh";
    meshNames["objBoxB"]    = preName + "mesh_box_b_0.mesh";
    
    meshNames["objBoxC_Left"]  = preName + "mesh_box_c_0.mesh";
    meshNames["objBoxC_Right"] = preName + "mesh_box_c_1.mesh";
    meshNames["objBoxC_Front"] = preName + "mesh_box_c_2.mesh";
    meshNames["objBoxC_Top"]   = preName + "mesh_box_c_3.mesh";

    meshNames["objSphereA"] = preName + "mesh_sphere_a_0.mesh";
    meshNames["objTeaA"]    = preName + "teapot_0_0.mesh";
    meshNames["objTeaB"]    = preName + "teapot_1_0.mesh";

    _buildNodes( sg, meshNames );
}

void Demo3Impl::_buildNodes( Khaos::SceneGraph* sg, map<String, String>::type& meshNames )
{
    AreaNode* area = sg->getRootNode("root");

#if TEST_RAY
    sg->initSceneBVH();
    SceneBVH* bvh = sg->getBVH();
    #define _add_bvh(x) bvh->addSceneNode( x )
#else
    #define _add_bvh(x) (void)(0)
#endif

#define _create_mesh_node(var, castshad, receshad) \
    MeshNode* var = sg->createSceneNode<MeshNode>(#var); \
    var->setMesh( meshNames[#var] ); \
    var->getMesh()->buildBVH(false); \
    var->setCastShadow( castshad ); \
    var->setReceiveShadow( receshad ); \
    area->addChild( var ); \
    _add_bvh( var );

#if TEST_SCENE_BASIC 
    // 大平面
    _create_mesh_node( objPlane, false, true );
    objPlane->setPosition( 0, 0, 0 );

    // 基础方盒子
    _create_mesh_node( objBoxA, true, true );
    objBoxA->setPosition( -2, objBoxA->getWorldAABB().getHalfSize().y, 2 );
    objBoxA->yaw( Math::toRadians(45) );

    _create_mesh_node( objBoxB, true, true );
    objBoxB->setPosition( 3.7f, objBoxB->getWorldAABB().getHalfSize().y, 3.2f );

    // 简单遮蔽体
    _create_mesh_node( objBoxC_Left, true, true );
    objBoxC_Left->setMaterial( preName+"mtr_blue.mtr" );
    objBoxC_Left->setScale( 0.5f, 4.0f, 6.0f );
    objBoxC_Left->setPosition( 1.0f, objBoxC_Left->getWorldAABB().getHalfSize().y, -5.0f );

    _create_mesh_node( objBoxC_Right, true, true );
    objBoxC_Right->setMaterial( preName+"mtr_blue.mtr" );
    objBoxC_Right->setScale( objBoxC_Left->getScale() );
    objBoxC_Right->setPosition( objBoxC_Left->getDerivedMatrix().getTrans().x + 4.0f
        , objBoxC_Left->getDerivedMatrix().getTrans().y, objBoxC_Left->getDerivedMatrix().getTrans().z );

    _create_mesh_node( objBoxC_Front, true, true );
    objBoxC_Front->setMaterial( preName+"mtr_blue.mtr" );
    objBoxC_Front->setScale( 4.5f, 4.0f, 0.5f );
    objBoxC_Front->setPosition( objBoxC_Left->getDerivedMatrix().getTrans().x + 2.0f
        , objBoxC_Left->getDerivedMatrix().getTrans().y, objBoxC_Left->getDerivedMatrix().getTrans().z + 3.25f );

    _create_mesh_node( objBoxC_Top, true, true );
    objBoxC_Top->setMaterial( preName+"mtr_blue.mtr" );
    objBoxC_Top->setScale( 4.5f, 0.5f, objBoxC_Left->getScale().z+objBoxC_Front->getScale().z );
    objBoxC_Top->setPosition( objBoxC_Left->getDerivedMatrix().getTrans().x + 2.0f
        , objBoxC_Left->getWorldAABB().getSize().y + 0.25f, objBoxC_Left->getDerivedMatrix().getTrans().z + 0.25f );

    // 球类
#if 1
    _create_mesh_node( objSphereA, true, true );
    objSphereA->setPosition( -6.7f, objSphereA->getWorldAABB().getHalfSize().y, 4.2f );
#endif

    // 茶壶测试
#if 0 
    MeshPtr meshTeaA( MeshManager::getOrCreateMesh( meshNames["objTeaA"] ) );
    meshTeaA->load( false );
    MeshPtr meshTeaB( MeshManager::getOrCreateMesh( meshNames["objTeaB"] ) );
    meshTeaB->load( false );

    AxisAlignedBox teaBox = meshTeaA->getAABB();
    teaBox.merge( meshTeaB->getAABB() );

    SceneNode* objTeaGroup = sg->createSceneNode<SceneNode>("objTeaGroup");
    objTeaGroup->setPosition( 2.2f, teaBox.getHalfSize().y, 6.5f );
    area->addChild( objTeaGroup );

    MeshNode* objTeaA = sg->createSceneNode<MeshNode>("objTeaA");
    objTeaA->setMesh( meshNames["objTeaA"] );
    objTeaA->getMesh()->buildBVH(false);
    objTeaA->setPosition( Vector3::ZERO );
    objTeaA->setCastShadow( true );
    objTeaA->setReceiveShadow( true );
    objTeaGroup->addChild( objTeaA );
    _add_bvh( objTeaA );

    MeshNode* objTeaB = sg->createSceneNode<MeshNode>("objTeaB");
    objTeaB->setMesh( meshNames["objTeaB"] );
    objTeaB->getMesh()->buildBVH(false);
    objTeaB->setPosition( Vector3::ZERO );
    objTeaB->setCastShadow( true );
    objTeaB->setReceiveShadow( true );
    objTeaGroup->addChild( objTeaB );
    _add_bvh( objTeaB );
#endif

#endif // end of TEST_SCENE_BASIC

    // 灯光
    {
        LightNode* lit1 = sg->createSceneNode<LightNode>("lit1");

        lit1->lookAt( Vector3(0, 0, 0), Vector3(0.7f, -1, 0.5f), Vector3::UNIT_Y );
        lit1->setLightType( LT_DIRECTIONAL );

        lit1->getLight()->setDiffuse( Color(0.5f, 0.5f, 0.5f) );

        lit1->getLight()->setShadowType( ST_PSSM );
        lit1->getLight()->setShadowResolution( 2048 );
        lit1->getLight()->setShadowTapGroups( 4 );
        lit1->getLight()->setShadowVisableRange( 200.0f );
        lit1->getLight()->setShadowFadeRange( 180.0f );
        lit1->getLight()->setShadowSplitSchemeWeight( 0.7f );
        lit1->getLight()->setShadowBlurSize( 3.0f );
        lit1->getLight()->setShadowStrength( 0.99f );
        lit1->getLight()->setShadowRndSmplCovers( 8 );

        area->addChild( lit1 );
    }

    // volume probes
    _buildVolumeProbes( sg );

    // update first
    area->_updateDerived();

    // bvh build
#if TEST_RAY
    bvh->build();
#endif 
}

void Demo3Impl::_buildVolumeProbes( Khaos::SceneGraph* sg )
{
    AreaNode* area = sg->getRootNode("root");

    VolumeProbeNode* vpn = sg->createSceneNode<VolumeProbeNode>("volProbTest");
    VolumeProbe* vp = vpn->getProbe();
    vp->setResolution( 15, 6, 15 );

    vpn->setScale( 20.0f, 6.2f, 20.0f );
    vpn->_updateDerived();

    float bottomProbePosY = vpn->getScale().y * -0.5f + vp->getGridSize().y * 0.5f;
    float bottomNeedPosY = 0.01f;
    vpn->setPosition( 0.0f, bottomNeedPosY-bottomProbePosY, 0.0f );

    area->addChild( vpn );
    vpn->_updateDerived();


#if 0
    // 显示光探球
    MeshPtr meshProbe( MeshManager::createSphere( "ballprobe", 1.0f, 20 ) );
    meshProbe->load( false );

    int k = 0;

    for ( int z = 0; z < vp->getResolutionZ(); ++z )
    {
        for ( int y = 0; y < vp->getResolutionY(); ++y )
        {
            for ( int x = 0; x < vp->getResolutionX(); ++x )
            {
                Vector3 pos = vp->getCenterWorldPos( x, y, z );

                String name = "probeshow" + intToString(k++);

                MeshNode* shownode = sg->createSceneNode<MeshNode>(name);
                shownode->setMesh( "ballprobe" );
                shownode->setMaterial( preName + "mtr_white.mtr" );
                shownode->setPosition( pos );
                shownode->setScale( Vector3(1.0f/3.0f) );

                area->addChild( shownode );
            }
        }
    }
#endif
}

void Demo3Impl::_generalUV2( SceneGraph* sg )
{
    // 开始
    AreaNode* area = sg->getRootNode("root");
    //SceneNode* objBoxTest = sg->getSceneNode("objBoxTest");
    //objBoxTest->setEnabled( false ); // boxtest不做计入

    //  唯一化场景，并生成uv2
    SceneBakeUniquify sbu;
    sbu.process( sg, SceneBakeUniquify::SBU_NEED_UV2 /*| SceneBakeUniquify::SBU_UNIQ_MTR*/ );
    sbu.save( preName+"demo3_sbu.data", SceneBakeUniquify::SBU_SAVE_ALL );

    // 结束
    //objBoxTest->setEnabled( true );
}

void Demo3Impl::_bakeTest( SceneGraph* sg )
{
    // load uniquify info
    SceneBakeUniquify sbu;
    sbu.load( preName+"demo3_sbu.data" );

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

    // bake
    //SphereSamples::setSampleCount( 1800 ); // 2500 1800 1512 1225

#if TEST_BAKE_TYPE == 0
    AOBake aoBake;
    //aoBake.setMaxRayLen( 2.0f );
    //aoBake.setFallOff( 0.25f );
    aoBake.setResPreName( "/BakeTest/test_ao_%d.dds", 0 );
    aoBake.general( sg, &sbid );
#elif TEST_BAKE_TYPE == 1
    IrrBake irrBake;
    irrBake.useDirectional( true );
    irrBake.setPreBVHName( "e:/_baketmp/BakeTest/test.pbvh" );
    irrBake.setPreBuildName( "e:/_baketmp/BakeTest/test_irr_%d.pb", 0 );
    irrBake.setPreVolBuildName( "e:/_baketmp/BakeTest/test_vol_%d.vpb", 0 );
    irrBake.setResPreName( "/BakeTest/test_irra_%d.dds", "/BakeTest/test_irrb_%d.dds", 0 );
    irrBake.setResVolName( "/BakeTest/test_volume_%d_%s.dds", 0 );
    irrBake.setPhase( IrrBake::PHASE_LITMAP_ONLY );

    irrBake.general( sg, &sbid, 2 );
#endif
}

void Demo3Impl::_buildOther( Khaos::SceneGraph* sg )
{
#if TEST_RAY
    MaterialPtr mtrYellow( MaterialManager::createMaterial( preName+"mtr_yellow.mtr" ) );
    mtrYellow->setNullCreator();
    mtrYellow->useAttrib<BaseColorAttrib>()->setValue( Color(1.0f, 1.0f, 0.0f) );

    MeshPtr meshBoxTest( MeshManager::createBox( preName+"mesh_box_test.mesh", 1.0f, 1.0f, 1.0f, 3, 3, 3 ) );
    meshBoxTest->load( false );
    meshBoxTest->buildBVH( false );
    meshBoxTest->setMaterialName( mtrYellow->getName() );

    AreaNode* area = sg->getRootNode("root");
    MeshNode* objBoxTest = sg->createSceneNode<MeshNode>("objBoxTest"); // 碰撞标识的调试用box
    objBoxTest->setMesh( meshBoxTest->getName() );
    objBoxTest->setPosition( 6.7f, objBoxTest->getWorldAABB().getHalfSize().y, 5.2f );
    objBoxTest->setScale( 0.2f );
    area->addChild( objBoxTest );
#endif
}

bool Demo3Impl::_onCreateScene()
{
    //////////////////////////////////////////////////////////////////////////
    // render setting
    RenderSettings* mainSettings = g_renderSettingManager->createRenderSettings( "mainSettings" );
    mainSettings->setRenderMode( RM_DEFERRED_SHADING );

    HDRSetting* hdrSetting = mainSettings->createSetting<HDRSetting>();
    hdrSetting->setEnabled( false );

    AntiAliasSetting* aaSetting = mainSettings->createSetting<AntiAliasSetting>();
    //aaSetting->setEnabled( false );

    //////////////////////////////////////////////////////////////////////////
    // create area
    SceneGraph* sg = g_root->createSceneGraph( "demo" );

    AreaNode* area = sg->createRootNode( "root" );
    const float h = 10000.0f;
    AxisAlignedBox aabb( -h,-h,-h, h,h,h );
    area->getArea()->initOctree( aabb, 8 );

    //////////////////////////////////////////////////////////////////////////
    // 处理
#if TEST_RAY
    const int  setupStep = 0;
    const bool setup_lightmap = 0;
    const bool setup_volmap = 0;
    const bool setup_directlit = 1;
#else
    const int  setupStep = 3;
    const bool setup_lightmap = 1;
    const bool setup_volmap = 0;
    const bool setup_directlit = 1;
#endif

    bool baked_run = false;

    if ( setupStep == 0 ) // 原始场景
    {
        _setupOriginal( sg );
    }
    else if ( setupStep == 1 ) // 利用原始场景生成第二套uv
    {
        _setupOriginal( sg );
        _generalUV2( sg );
    }
    else if ( setupStep == 2 ) // 执行烘焙场景
    {
        _setupNew( sg );
        _bakeTest( sg );
        baked_run = true;
    }
    else if ( setupStep == 3 ) // 查看烘焙过的场景
    {
        _setupNew( sg );
        baked_run = true;
    }

    _buildOther( sg );

    //////////////////////////////////////////////////////////////////////////
    // bake setup
    if ( baked_run && setup_lightmap )
    {
        char buf[1024] = {};

        for ( int i = 0; i < 10; ++i )
        {
            LightmapItem* item = sg->getInstSharedDataMgr().createLightmap(i);

#if TEST_BAKE_TYPE == 0
            item->setType( LightmapItem::LMT_AO );
            sprintf_s( buf, "/BakeTest/test_ao_%d.dds", i );
            item->getMap().setName( buf );
#elif TEST_BAKE_TYPE == 1
            item->setType( LightmapItem::LMT_FULL );
            sprintf_s( buf, "/BakeTest/test_irra_%d.dds", i );
            item->getMap().setName( buf );
            sprintf_s( buf, "/BakeTest/test_irrb_%d.dds", i );
            item->getMapB().setName( buf );
#endif
        }

#define _set_light_map_id(name, id) \
        if ( sg->getSceneNode(name) ) static_cast<MeshNode*>(sg->getSceneNode(name))->setLightMapID( id );

        _set_light_map_id( "objPlane", 0 );
        _set_light_map_id( "objBoxA", 1 );
        _set_light_map_id( "objBoxB", 2 );

        _set_light_map_id( "objBoxC_Left", 3 );
        _set_light_map_id( "objBoxC_Right", 4 );
        _set_light_map_id( "objBoxC_Front", 5 );
        _set_light_map_id( "objBoxC_Top", 6 );

        _set_light_map_id( "objSphereA", 7 );

#if TEST_BAKE_TYPE == 0
        sg->getSkyEnv().setAmbConstClr( Color(1.0f, 1.0f, 1.0f) ); // 为了体现ao
        sg->getSkyEnv().setEnabled( SkyEnv::SKY_SIMPLEAMB );
#endif
    }

    if ( baked_run && setup_volmap )
    {
        VolumeProbeNode* vpn = static_cast<VolumeProbeNode*>(sg->getSceneNode("volProbTest"));
        vpn->getProbe()->load( 0, "/BakeTest/test_volume_0_%s.dds" );
    }

    LightNode* lit1 = static_cast<LightNode*>( sg->getSceneNode("lit1") );
    lit1->setEnabled( setup_directlit );

    //////////////////////////////////////////////////////////////////////////
    // camera
    CameraNode* cam1 = sg->createSceneNode<CameraNode>("mainCamera");
    cam1->lookAt( Vector3(0, 7, 20), Vector3(0, 0, 0), Vector3::UNIT_Y );
    cam1->getCamera()->setPerspective( Math::toRadians(45), 4.0f/3.0f, 0.5f, 500.0f );
    cam1->getCamera()->setRenderSettings( mainSettings );
    area->addChild( cam1 );

    g_root->addActiveCamera( cam1 );

    //////////////////////////////////////////////////////////////////////////
    // operation
    m_samOp.init( cam1, 10.0f, 0.01f );

    return true;
}

bool Demo3Impl::_onUpdate()
{
    if ( !SampleFrame::_onUpdate() )
        return false;

    SceneGraph* sg = g_root->getSceneGraph( "demo" );
    return true;
}

void Demo3Impl::onKeyUp( int key )
{
    _processKeyCommon( key );
}

void Demo3Impl::onMouseUp( int key, int x, int y )
{
#if TEST_RAY
    if ( key == MK_RBUTTON )
    {
        SceneGraph* sg = g_root->getSceneGraph( "demo" );
        CameraNode* cam = (CameraNode*)sg->getSceneNode("mainCamera");
        
        sg->getSceneNode( "objBoxTest" )->setPosition( Vector3(-10000,-10000,-10000) ); // hide it

        int vpWidth  = cam->getCamera()->getViewportWidth();
        int vpHeight = cam->getCamera()->getViewportHeight();
        
        float sx = x / (float)vpWidth;
        float sy = y / (float)vpHeight;

        Ray ray = cam->getCamera()->getRayByViewport( sx, sy );
        //LimitRay ray(ray1, 20);

#if USE_SCENE_BVH
        float t = -10000;
        int   face = -1;
        SceneNode* retNode = 0;
        SceneBVH::Result result;
        if ( sg->getBVH()->intersect( ray, result ) )
        {
            t = result.distance;
            face = sg->getBVH()->getFaceInfo( result.faceID )->faceIdx;
            int instanceID = sg->getBVH()->getFaceInfo( result.faceID )->instanceID;   
            retNode = sg->getBVH()->getSceneNode( instanceID );
        }
#else
        float t;
        int   sub;
        int   face;
        SceneNode* retNode = rayIntersectSGDetail( sg, /*static_cast<LimitRay&>*/(ray), &sub, &face, &t );
        if ( !retNode )
            t = -10000;
#endif

        Vector3 pt = ray.getPoint( t );
        sg->getSceneNode( "objBoxTest" )->setPosition( pt );

        char msg[512] = {};
        sprintf_s( msg, "ray(%f,%f,%f)(%f,%f,%f) => %s[%d,%f]\n",
            ray.getOrigin().x, ray.getOrigin().y, ray.getOrigin().z,
            ray.getDirection().x, ray.getDirection().y, ray.getDirection().z,
            (retNode ? retNode->getName().c_str() : "null"), face, t );
        OutputDebugStringA( msg );
    }
#endif
}
