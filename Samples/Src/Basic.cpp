#include "Basic.h"
#include <KhaosRoot.h>
#include <KhaosObjImport.h>
#include <KhaosDist2Alpha.h>

using namespace Khaos;

#ifdef _BASIC_ACTIVE
    SampleFrame* createSampleFrame()
    {
        return KHAOS_NEW SampleImpl;
    }
#endif

typedef Khaos::vector<MaterialPtr>::type MtrList;
typedef Khaos::vector<MeshPtr>::type MeshList;

SampleImpl::SampleImpl()
{
}

SampleImpl::~SampleImpl()
{
}

void _buildMtrs( MtrList& mtrList )
{
    MaterialPtr mtrComm( MaterialManager::createMaterial( "mtr_comm" ) );
    mtrComm->setNullCreator();
    mtrComm->useAttrib<BaseColorAttrib>()->setValue( Color(0.1f, 0.1f, 0.1f) );
    mtrComm->useAttrib<MetallicAttrib>()->setValue( 0.0f );
    mtrComm->useAttrib<DSpecularAttrib>()->setValue( 0.3f );
    mtrComm->useAttrib<RoughnessAttrib>()->setValue( 0.25f );
    mtrComm->useAttrib<BaseMapAttrib>()->setTexture( "wall.jpg" );
    mtrComm->useAttrib<NormalMapAttrib>()->setTexture( "wall_n.dds" );
    mtrList.push_back( mtrComm );

    MaterialPtr mtrDiff( MaterialManager::createMaterial( "mtr_diff" ) );
    mtrDiff->setNullCreator();
    mtrDiff->useAttrib<BaseColorAttrib>()->setValue( Color::WHITE );
    mtrDiff->useAttrib<BaseMapAttrib>()->setTexture( "grass_1024.jpg" );
    mtrList.push_back( mtrDiff );

    MaterialPtr mtrReflect( MaterialManager::createMaterial( "mtr_reflect" ) );
    mtrReflect->setNullCreator();
    mtrReflect->useAttrib<BaseColorAttrib>()->setValue( Color(0.6f, 0.6f, 0.6f) );
    mtrReflect->useAttrib<BaseMapAttrib>()->setTexture( "wall.jpg" );
    //mtrReflect->useAttrib<EnvironmentMapAttrib>()->setTexture( "church.dds" );
    //mtrReflect->useAttrib<EnvironmentMapAttrib>()->setEnvColor( Color(0.9f, 0.9f, 0.9f) );
    mtrList.push_back( mtrReflect );

    MaterialPtr mtrReflectMove( MaterialManager::createMaterial( "mtr_reflect_move" ) );
    mtrReflectMove->setNullCreator();
    mtrReflectMove->useAttrib<BaseColorAttrib>()->setValue( Color(0.6f, 0.6f, 0.6f) );
    //mtrReflectMove->useAttrib<EnvironmentMapAttrib>()->setTexture( "envrt" );
    //mtrReflectMove->useAttrib<EnvironmentMapAttrib>()->setEnvColor( Color(0.9f, 0.9f, 0.9f) );
    //mtrReflectMove->useAttrib<EnvironmentMapAttrib>()->setProbe( "objEnvProbe" );
    mtrList.push_back( mtrReflectMove );

    // test distance alpha
    MaterialPtr mtr_at1( MaterialManager::createMaterial( "mtr_at1" ) );
    mtr_at1->setNullCreator();
    mtr_at1->useAttrib<BaseColorAttrib>()->setValue( Color(1,0,0) );
    mtr_at1->useAttrib<OpacityMapAttrib>()->setTexture( "wo_sm1.jpg" );
    mtr_at1->useAttrib<AlphaTestAttrib>()->setValue( 0.5f );
    mtrList.push_back( mtr_at1 );

#if 0
    Texture* tex = TextureManager::getOrCreateTexture("wo.jpg");
    tex->load( false );
  
    Dist2AlphaConfig cfg;
    cfg.spread = 10.0f;
    cfg.flag = Dist2AlphaConfig::R_ENABLED;
    cfg.destTexFmt = PIXFMT_L8;
    cfg.destWidth = 64;
    cfg.destHeight = 64;
    cfg.mipmapGenMinSize = 64;
    cfg.mipmapReadSize = TexObjLoadParas::MIP_FROMFILE;
    cfg.mipmapFilter = TEXF_NONE;
    cfg.sRGBWrite = 0;
  
    dist2Alpha( tex, "/Texture/wo_sm2.jpg", cfg );
#endif

    MaterialPtr mtr_at2( MaterialManager::createMaterial( "mtr_at2" ) );
    mtr_at2->setNullCreator();
    mtr_at2->useAttrib<BaseColorAttrib>()->setValue( Color(1,0,0) );
    mtr_at2->useAttrib<OpacityMapAttrib>()->setTexture( "wo_sm2.jpg" );
    mtr_at2->useAttrib<AlphaTestAttrib>()->setValue( 0.5f );
    mtrList.push_back( mtr_at2 );
}

void _buildMeshs( MeshList& meshList )
{
    MeshPtr meshPlane( MeshManager::createPlane( "mesh_plane", 200, 160, 30, 30 ) );
    meshPlane->load( false );
    meshPlane->setMaterialName( "mtr_diff" );
    meshList.push_back( meshPlane );

    MeshPtr meshCube( MeshManager::createCube( "mesh_cube", 1.5f ) );
    meshCube->load( false );
    meshCube->setMaterialName( "mtr_comm" );
    meshCube->generateTangents( false );
    meshList.push_back( meshCube );

    MeshPtr meshSphere( MeshManager::createSphere( "mesh_sphere", 2.0f, 30 ) );
    meshSphere->load( false );
    meshSphere->generateTangents( false );
    meshList.push_back( meshSphere );

    MeshPtr meshCone( MeshManager::createCone( "mesh_cone", 2.0f, 1.732f, 30, 30 ) );
    meshCone->load( false );
    meshCone->setMaterialName( "mtr_diff" );
    meshList.push_back( meshCone );

    MeshPtr meshCone2( MeshManager::createCone( "mesh_cone2", 2.0f, 1.732f, 30, 30, true ) );
    meshCone2->load( false );
    meshCone2->setMaterialName( "mtr_diff" );
    meshList.push_back( meshCone2 );

    MeshPtr meshConeLit( MeshManager::createCone( "mesh_cone_lit", 2.0f, 1.0f, 20, 2, true ) );
    meshConeLit->load( false );
    meshConeLit->setMaterialName( "mtr_diff" );
    meshList.push_back( meshConeLit );

    MeshPtr meshWo( MeshManager::createPlane( "mesh_wo", 1, 1, 3, 3 ) );
    meshWo->load( false );
    meshList.push_back( meshWo );
}

void _buildInstSharedData( SceneGraph* sg )
{
    InstanceSharedDataMgr& isdm = sg->getInstSharedDataMgr();

    EnvProbeData* envProbe = isdm.createEnvProbe(0);
    envProbe->setType( EnvProbeData::STATIC_SPECULAR | EnvProbeData::STATIC_DIFFUSE );
    envProbe->setSpecularName( "test_spec_ibl.dds" );
    envProbe->setDiffuseName( "test_diff_ibl.dds" );
}

void _buildTestMtrScene( SceneGraph* sg, AreaNode* area )
{
    const int count = 10;

    Color clrBase[3] =
    {
        Color(0.2f, 0.0f, 0.0f),
        Color(0.560f, 0.570f, 0.580f),
        Color(1.000f, 0.766f, 0.336f)
    };

    float metallics[3] = { 0.0f, 0.5f, 1.0f };
    float specular = 0.5f;

    float startRoughness = 0.02f;
    float stepRoughness  = (1.0f - startRoughness) / (count-1);

    Vector3 startPos(-3, 11, 0);

    for ( int j = 0; j < 3; ++j )
    {
        float curMetal = metallics[j];

        for ( int i = 0; i < count; ++i )
        {
            float curRoughness = stepRoughness * i + startRoughness;

            String name_mtr = "mtr_t_" + intToString(j) + "_" + intToString(i);
            MaterialPtr mtrComm( MaterialManager::createMaterial( name_mtr ) );

            mtrComm->setNullCreator();
            mtrComm->useAttrib<BaseColorAttrib>()->setValue( clrBase[j] );
            mtrComm->useAttrib<MetallicAttrib>()->setValue( curMetal );
            mtrComm->useAttrib<DSpecularAttrib>()->setValue( specular );
            mtrComm->useAttrib<RoughnessAttrib>()->setValue( curRoughness );

            //mtrComm->useAttrib<BaseMapAttrib>()->setTexture( "grass_1024.jpg" );
            //mtrComm->useAttrib<SpecularMapAttrib>()->setTexture( "grass_1024.jpg" );
            //mtrComm->useAttrib<SpecularMapAttrib>()->setMetallicChannel( SpecularMapAttrib::RChannel );
            //mtrComm->useAttrib<SpecularMapAttrib>()->setDSpecularChannel( SpecularMapAttrib::GChannel );
            //mtrComm->useAttrib<SpecularMapAttrib>()->setRoughnessChannel( SpecularMapAttrib::BChannel );

            //mtrComm->useAttrib<EnvironmentMapAttrib>()->setTexture( "church1.dds" );
            //mtrComm->useAttrib<EnvironmentMapAttrib>()->setEnvColor( Color(0.0f, 0.0f, 0.0f) );

            String name_mesh = "obj_t_" + name_mtr;
            MeshNode* objSphere1 = sg->createSceneNode<MeshNode>( name_mesh );
            objSphere1->setMesh( "mesh_sphere" );
            objSphere1->setMaterial( name_mtr );
            objSphere1->setEnvProbeID( 0 );

            Vector3 curPos = startPos;
            curPos.x += 3 * i;
            curPos.y -= 3 * j;
            objSphere1->setPosition( curPos  );
            area->addChild( objSphere1 );
        }
    }
   
  
}

bool SampleImpl::_onCreateScene()
{
    SceneGraph* sg = g_root->createSceneGraph( "demo" );

    // area
    AreaNode* area = sg->createRootNode( "root" );
    float h = 10000.0f;
    AxisAlignedBox aabb( -h,-h,-h, h,h,h );
    area->getArea()->initOctree( aabb, 8 );

    MtrList mtrList;
    _buildMtrs(mtrList);

    MeshList meshList;
    _buildMeshs(meshList);

    _buildInstSharedData( sg );

#if 1
    //////////////////////////////////////////////////////////////////////////
    // mesh node
#if 1
    MeshNode* objPlane = sg->createSceneNode<MeshNode>("objPlane");
    objPlane->setMesh( "mesh_plane" );
    objPlane->setPosition( -10.0f, 0, 0 );
    objPlane->setOrientation( Vector3::UNIT_Y, Math::toRadians(20) );
    objPlane->setReceiveShadow( true );
    area->addChild( objPlane );
#endif

    MeshNode* objCube1 = sg->createSceneNode<MeshNode>("objCube1");
    objCube1->setMesh( "mesh_cube" );
    objCube1->setMaterial( "mtr_reflect" );
    objCube1->setPosition( Vector3(-20, 0.75f, 0) );
    objCube1->setCastShadow( true );
    objCube1->setReceiveShadow( true );
    area->addChild( objCube1 );

    MeshNode* objCube2 = sg->createSceneNode<MeshNode>("objCube2");
    objCube2->setMesh( "mesh_cube" );
    objCube2->lookAt( Vector3(5, 5, 1), Vector3(0, 0, 6), Vector3::UNIT_Y );
    objCube2->setScale( 3.5f );
    objCube2->setCastShadow( true );
    objCube2->setReceiveShadow( true );
    //objCube2->yaw( 19.0560017f, Node::TS_LOCAL );
    objCube1->addChild( objCube2 );


    MeshNode* objSphere1 = sg->createSceneNode<MeshNode>("objSphere1");
    objSphere1->setMesh( "mesh_sphere" );
    objSphere1->setMaterial( "mtr_comm" );
    objSphere1->setPosition( Vector3(3.5f, 1, 0) );
    objSphere1->setCastShadow( true );
    objSphere1->setReceiveShadow( true );
    area->addChild( objSphere1 );

    MeshNode* objSphere2 = sg->createSceneNode<MeshNode>("objSphere2");
    objSphere2->setMesh( "mesh_sphere" );
    objSphere2->setMaterial( "mtr_reflect" );
    objSphere2->setPosition( Vector3(0.0f, 1, 1.0f) );
    objSphere2->setCastShadow( true );
    area->addChild( objSphere2 );

    MeshNode* objReflMove = sg->createSceneNode<MeshNode>("objReflMove");
    objReflMove->setMesh( "mesh_sphere" );
    objReflMove->setMaterial( "mtr_reflect_move" );
    objReflMove->setPosition( Vector3(-7.0f, 2.75f, 0) );
    objReflMove->setScale( 1.0f );
    objReflMove->setCastShadow( true );
    area->addChild( objReflMove );

    //EnvProbeNode* probeNode = sg->createSceneNode<EnvProbeNode>("objEnvProbe");
    //probeNode->getEnvProbe()->setNear( objReflMove->getWorldAABB().getHalfSize().maxValue() + 0.001f );
    //probeNode->getEnvProbe()->setFar( 1000.0f );
    //probeNode->getEnvProbe()->setResolution( 128 );
    //objReflMove->addChild( probeNode );

    MeshNode* objCone1 = sg->createSceneNode<MeshNode>("objCone1");
    objCone1->setMesh( "mesh_cone" );
    objCone1->setPosition( Vector3(-4.0f, 0, 5.0f) );
    objCone1->setCastShadow( true );
    area->addChild( objCone1 );

    MeshNode* objCone2 = sg->createSceneNode<MeshNode>("objCone2");
    objCone2->setMesh( "mesh_cone2" );
    objCone2->setPosition( Vector3(-4.0f, 0.75f, 5.0f) );
    objCone2->setCastShadow( true );
    area->addChild( objCone2 );

#if 1
    {
        SceneNode* objFloor = sg->createSceneNode<SceneNode>( "floorGroup" );
        objFloor->setPosition( Vector3(14.0f, 1.4f, 2.0f) );
        area->addChild( objFloor );

#if 0
        ObjSceneImporter osi;
        osi.setMeshPreName( "floor/" );
        osi.setDefaultMaterialName( "mtr_comm" );
        osi.setResBase( "" );
        osi.setShadow( true, true );
        osi.import( "Model/floor.obj", objFloor );
#endif

        for ( int i = 0; i < 5; ++i )
        {
            String meshName = "floor/" + intToString(i) + ".mesh";
            MeshNode* node = sg->createSceneNode<MeshNode>( meshName );
            node->setMesh( meshName );
            node->setCastShadow( true );
            node->setReceiveShadow( true );
            node->getMesh()->generateTangents( false );
            node->setEnvProbeID( 0 );
            objFloor->addChild( node );
        }
    }
#endif


#if 1
    {
        SceneNode* objTeapot = sg->createSceneNode<SceneNode>( "teapotGroup" );
        objTeapot->setPosition( Vector3(14.0f, 1.4f, 2.0f) );
        area->addChild( objTeapot );

#if 0
        ObjSceneImporter osi;
        osi.setMeshPreName( "teapot/" );
        osi.setDefaultMaterialName( "mtr_comm" );
        osi.setResBase( "" );
        osi.setShadow( true, true );
        osi.import( "Model/teapot.obj", objTeapot );
#endif

        for ( int i = 0; i < 2; ++i )
        {
            String meshName = "teapot/" + intToString(i) + ".mesh";
            MeshNode* node = sg->createSceneNode<MeshNode>( meshName );
            node->setMesh( meshName );
            node->setCastShadow( true );
            node->setReceiveShadow( true );
            node->getMesh()->generateTangents( false );
            node->setEnvProbeID( 0 );
            objTeapot->addChild( node );
        }
    }
#endif

#endif


    {
        const float wo_height = 10.0f;
        const float wo_x = 6.0f;
        const float wo_z = -30.0f;
        const float wo_scale = 10.0f;

        MeshNode* obj_at1 = sg->createSceneNode<MeshNode>("obj_at1");
        obj_at1->setMesh( "mesh_wo" );
        obj_at1->setMaterial( "mtr_at1" );
        obj_at1->setPosition( -wo_x, wo_height, wo_z );
        obj_at1->setOrientation( Vector3::UNIT_X, Math::toRadians(90) );
        obj_at1->setScale( wo_scale );
        area->addChild( obj_at1 );

        MeshNode* obj_at2 = sg->createSceneNode<MeshNode>("obj_at2");
        obj_at2->setMesh( "mesh_wo" );
        obj_at2->setMaterial( "mtr_at2" );
        obj_at2->setPosition( wo_x, wo_height, wo_z );
        obj_at2->setOrientation( Vector3::UNIT_X, Math::toRadians(90) );
        obj_at2->setScale( wo_scale );
        area->addChild( obj_at2 );
    }

    _buildTestMtrScene( sg, area );

    //////////////////////////////////////////////////////////////////////////
    // light node
    LightNode* lit1 = sg->createSceneNode<LightNode>("lit1");
    lit1->lookAt( Vector3(0, 0, 0), Vector3(-1, -1, -1), Vector3::UNIT_Y );
    lit1->setLightType( LT_DIRECTIONAL );
    lit1->getLight()->setDiffuse( Color(0.75f, 0.75f, 0.75f) );
#if 1
    lit1->getLight()->setShadowType( ST_PSSM );
    lit1->getLight()->setShadowResolution( 1024 );
    lit1->getLight()->setShadowTapGroups( 4 );
    lit1->getLight()->setShadowVisableRange( 100.0f );
    lit1->getLight()->setShadowFadeRange( 85.0f );
    lit1->getLight()->setShadowSplitSchemeWeight( 0.5f );
    lit1->getLight()->setShadowBlurSize( 1.2f );
    lit1->getLight()->setShadowStrength( 0.99f );
    lit1->getLight()->setShadowRndSmplCovers( 16 );
#endif
    area->addChild( lit1 );

    LightNode* lit2 = sg->createSceneNode<LightNode>("lit2");
    lit2->setPosition( -25, 10, -1 );
    lit2->setLightType( LT_POINT );
    lit2->getLight()->setDiffuse( Color(0.5f, 0.1f, 0.1f) );
    lit2->getLightAs<PointLight>()->setRange( 40.0f );
    lit2->getLightAs<PointLight>()->setFadePower(1.0f);
    //area->addChild( lit2 );

    //MeshNode* lit2_obj = sg->createSceneNode<MeshNode>("lit2_obj");
    //lit2_obj->setMesh( "mesh_sphere" );
    //lit2_obj->setMaterial( "mtr_comm" );
    //lit2_obj->setPosition( lit2->getPosition() );
    //lit2_obj->setScale( lit2->getLightAs<PointLight>()->getRange() * 0.95f );
    //area->addChild( lit2_obj );

    LightNode* lit3 = sg->createSceneNode<LightNode>("lit3");
    lit3->lookAt( Vector3(-45, 40, -35), Vector3(0, 0, 0), Vector3::UNIT_Y );
    lit3->setLightType( LT_SPOT );
    SpotLight* lit3Spot = lit3->getLightAs<SpotLight>();
    lit3Spot->setOuterCone( Math::toRadians(30) );
    lit3Spot->setInnerCone( Math::toRadians(15) );
    lit3Spot->setDiffuse( Color(0.1f, 0.5f, 0.1f) );
    lit3Spot->setRange( 100.0f );
    lit3Spot->setFadePower( 2.0f );
    //area->addChild( lit3 );

    //lit3->_updateDerived();
    //Matrix4 mat = lit3Spot->getVolumeWorldMatrix();
    //mat = Matrix4::getScale( 0.98f, 0.98f, 0.98f ) * mat;
    //MeshNode* lit3_obj = sg->createSceneNode<MeshNode>("lit3_obj");
    //lit3_obj->setMesh( "mesh_cone_lit" );
    //lit3_obj->setMaterial( "mtr_comm" );
    //lit3_obj->setMatrix( mat );
    //area->addChild( lit3_obj );

    // sky
    SkyEnv& sky = sg->getSkyEnv();
    sky.setAmbConstClr( Color(0.02f, 0.02f, 0.02f) );
    //sky.setAmbUpperClr( Color(0.1f, 0.1f, 0.2f) );
    //sky.setAmbLowerClr( Color(0.1f, 0.2f, 0.1f) );
    sky.setEnabled( SkyEnv::SKY_SIMPLEAMB );

    //////////////////////////////////////////////////////////////////////////
    // render settings
    RenderSettings* mainSettings = g_renderSettingManager->createRenderSettings( "mainSettings" );
    mainSettings->setRenderMode( RM_FORWARD );

    SSAORenderSetting* ssaoSetting = mainSettings->createSetting<SSAORenderSetting>();
    ssaoSetting->setDiskRadius( 1.5f );
    ssaoSetting->setSmallRadiusRatio( 0.3f );
    ssaoSetting->setLargeRadiusRatio( 4.0f );
    ssaoSetting->setContrast( 1.0f );
    ssaoSetting->setAmount( 5.0f );
    ssaoSetting->setBrighteningMargin( 1.1f );
    ssaoSetting->setEnabled( false );

    HDRSetting* hdrSetting = mainSettings->createSetting<HDRSetting>();
    //hdrSetting->setEnabled( false );

    AntiAliasSetting* aaSetting = mainSettings->createSetting<AntiAliasSetting>();
    //aaSetting->setEnabled( false );

    //////////////////////////////////////////////////////////////////////////
    // camera
    CameraNode* cam1 = sg->createSceneNode<CameraNode>("mainCamera");
    cam1->lookAt( Vector3(0, 1.6f, 20), Vector3(0,0,0), Vector3::UNIT_Y );
    cam1->getCamera()->setPerspective( Math::toRadians(60), 4.0f/3.0f, 1.0f, 2000.0f );
    cam1->getCamera()->setRenderSettings( mainSettings );
    area->addChild( cam1 );

    g_root->addActiveCamera( cam1 );

    //////////////////////////////////////////////////////////////////////////
    // operation
    m_samOp.init( cam1, 50.0f, 0.01f );

    return true;
}

bool SampleImpl::_onUpdate()
{
    if ( !SampleFrame::_onUpdate() )
        return false;

    SceneGraph* sg = g_root->getSceneGraph( "demo" );
    SceneNode* objCube2 = sg->getSceneNode( "objCube2" );

    float angle = g_timerSystem.getElapsedTime() * 50;
    static float aa = 0;
    aa += angle;
    //objCube2->yaw( angle, Node::TS_LOCAL );
    return true;
}

void SampleImpl::onKeyUp( int key ) 
{
    _processKeyCommon( key );
}

