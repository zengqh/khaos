#include "Demo4.h"
#include <KhaosRoot.h>
#include <KhaosObjImport.h>
#include <KhaosMeshFile.h>
#include <KhaosGlossyTool.h>

using namespace Khaos;

#ifdef _DEMO4_ACTIVE
    SampleFrame* createSampleFrame()
    {
        return KHAOS_NEW Demo4Impl;
    }
#endif


void Demo4Impl::_setupCamera()
{
    SceneGraph* sg = g_root->getSceneGraph( "demo" );
    AreaNode* area = sg->getRootNode( "root" );

    //////////////////////////////////////////////////////////////////////////
    // render setting
    RenderSettings* mainSettings = g_renderSettingManager->createRenderSettings( "mainSettings" );
    mainSettings->setRenderMode( /*RM_FORWARD*/RM_DEFERRED_SHADING );

    SSAORenderSetting* ssaoSetting = mainSettings->createSetting<SSAORenderSetting>();
    ssaoSetting->setDiskRadius( 1.5f );
    ssaoSetting->setSmallRadiusRatio( 0.5f );
    ssaoSetting->setLargeRadiusRatio( 1.5f );
    ssaoSetting->setAmount( 2.5f );
    ssaoSetting->setEnabled( false );

    HDRSetting* hdrSetting = mainSettings->createSetting<HDRSetting>();
    //hdrSetting->setEnabled( false );

    AntiAliasSetting* aaSetting = mainSettings->createSetting<AntiAliasSetting>();
    //aaSetting->setEnabled( false );
    //aaSetting->setEnableMain( false );
    //aaSetting->setEnableTemporal( false );

    //////////////////////////////////////////////////////////////////////////
    // camera
    CameraNode* cam1 = sg->createSceneNode<CameraNode>("mainCamera");
    cam1->lookAt( Vector3(0, 1.6f, 6), Vector3(0,0,0), Vector3::UNIT_Y );
    cam1->getCamera()->setPerspective( Math::toRadians(60), 4.0f/3.0f, 0.1f, 100.0f );
    cam1->getCamera()->setRenderSettings( mainSettings );
    area->addChild( cam1 );

    g_root->addActiveCamera( cam1 );

    //////////////////////////////////////////////////////////////////////////
    // operation
    m_samOp.init( cam1, 5.0f, 0.01f );
}

bool Demo4Impl::_onCreateScene()
{
    SceneGraph* sg = g_root->createSceneGraph( "demo" );

    // area
    AreaNode* area = sg->createRootNode( "root" );
    float h = 10000.0f;
    AxisAlignedBox aabb( -h,-h,-h, h,h,h );
    area->getArea()->initOctree( aabb, 8 );

    // probe
    InstanceSharedDataMgr& isdm = sg->getInstSharedDataMgr();

    EnvProbeData* envProbe = isdm.createEnvProbe(0);
    envProbe->setType( EnvProbeData::STATIC_SPECULAR | EnvProbeData::STATIC_DIFFUSE );
    envProbe->setSpecularName( "test_spec_ibl.dds" );
    envProbe->setDiffuseName( "test_diff_ibl.dds" );

    // mtr
#if 0
    {
        ObjMtlResImporter omri;
        omri.setMethod( ObjMtlResImporter::IMT_PBRMTR );
        omri.setResBase( "/Pistol" );
        omri.import( "Pistol/pistol.mtl" );

        // glossy tool test
        RsCmdPtr cmd(KHAOS_NEW GlossyAABatchCmd);
        //g_renderSystem->addPostEffRSCmd( cmd );
    }
#endif

    MaterialPtr mtr( MaterialManager::createMaterial( "/Pistol/Cerberus00_FixedSG.mtr" ) );
    mtr->load( false );
    //DSpecularAttrib* dspec = mtr->getAttrib<DSpecularAttrib>();
    //dspec->setValue( 0.5f );
    //RoughnessAttrib* roug = mtr->getAttrib<RoughnessAttrib>();
    //roug->setValue( 0.2f );

    // node
    {
        SceneNode* objPistol = sg->createSceneNode<SceneNode>( "PistolGroup" );
        objPistol->setPosition( Vector3(0, 0, 0) );
        objPistol->yaw( Math::HALF_PI * -0.5f );
        objPistol->setScale( 0.1f );
        area->addChild( objPistol );
         
#if 0
        ObjSceneImporter osi;
        osi.setMeshPreName( "/Pistol/" );
        osi.setDefaultMaterialName( "mtr_comm" );
        osi.setResBase( "/Pistol/" );
        osi.setShadow( false, false );
        osi.import( "Pistol/Pistol.obj", objPistol );
#endif

        //Math::setGramSchmidtOrthogonalizeFlip( true );

        for ( int i = 0; i < 1; ++i )
        {
            String meshName = "/Pistol/" + intToString(i) + ".mesh";
            MeshNode* node = sg->createSceneNode<MeshNode>( meshName );
            node->setMesh( meshName );

#if 0 // º∆À„«–œﬂ
            for ( int j = 0; j < node->getSubEntityCount(); ++j )
            {
                if ( NormalMapAttrib* attr = node->getSubEntity(j)->getMaterial()->getAttrib<NormalMapAttrib>() )
                {
                    khaosAssert( attr->getTextureName().size() );
                    node->getMesh()->generateTangents( true );

                    MeshExporter exp;
                    exp.exportMesh( node->getMesh()->getResFileName(), node->getMesh() );
                    break;
                }
            }
#endif

            //node->setCastShadow( true );
            //node->setReceiveShadow( true );
            node->setEnvProbeID( 0 );

            objPistol->addChild( node );
        }

        Math::setGramSchmidtOrthogonalizeFlip( false );
    }

    //////////////////////////////////////////////////////////////////////////
    // light node
    LightNode* lit1 = sg->createSceneNode<LightNode>("lit1");
    lit1->lookAt( Vector3(0, 0, 0), Vector3(/*-0.01f*/-0.5f, -1.0f, -0.35f), Vector3::UNIT_Y );
    lit1->setLightType( LT_DIRECTIONAL );
    lit1->getLight()->setDiffuse( Color(2.95f, 2.95f, 2.95f) );
    lit1->getLight()->setShadowType( ST_PSSM );
    lit1->getLight()->setCascadesCount( 3 );
    lit1->getLight()->setShadowResolution( 2048 );
    lit1->getLight()->setShadowTapGroups( 4 );
    lit1->getLight()->setShadowVisableRange( 50.0f );
    lit1->getLight()->setShadowFadeRange( 45.0f );
    lit1->getLight()->setShadowSplitSchemeWeight( 0.5f );
    lit1->getLight()->setShadowBlurSize( 1.2f );
    lit1->getLight()->setShadowStrength( 1.0f );
    lit1->getLight()->setShadowRndSmplCovers( 16 );
    //lit1->getLight()->setShadowEnabled( false );
    //area->addChild( lit1 );

    // sky
    SkyEnv& sky = sg->getSkyEnv();
    sky.setAmbConstClr( Color(0.02f, 0.02f, 0.02f) );
    //sky.setAmbUpperClr( Color(0.1f, 0.1f, 0.2f) );
    //sky.setAmbLowerClr( Color(0.1f, 0.2f, 0.1f) );
    //sky.setEnabled( SkyEnv::SKY_SIMPLEAMB );

    // setup camera and config
    _setupCamera();

    return true;
}

void Demo4Impl::onKeyUp( int key ) 
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

