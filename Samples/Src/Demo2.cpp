#include "Demo2.h"
#include <KhaosRoot.h>
#include <KhaosGI.h>
#include <KhaosSHUtil.h>
#include <KhaosSampleUtil.h>

using namespace Khaos;

#ifdef _DEMO2_ACTIVE
    SampleFrame* createSampleFrame()
    {
        return KHAOS_NEW Demo2Impl;
    }
#endif

GISystem* g_giSys = 0;

Demo2Impl::Demo2Impl()
{
}

Demo2Impl::~Demo2Impl()
{
}

void Demo2Impl::_onDestroyScene()
{
    KHAOS_DELETE g_giSys;
}

bool Demo2Impl::_onCreateScene()
{
    _testSH();

    SceneGraph* sg = g_root->createSceneGraph( "demo" );

    // area
    AreaNode* area = sg->createRootNode( "root" );
    float h = 10000.0f;
    AxisAlignedBox aabb( -h,-h,-h, h,h,h );
    area->getArea()->initOctree( aabb, 8 );

    //////////////////////////////////////////////////////////////////////////
    // material
    MaterialPtr mtrDiff( MaterialManager::createMaterial( "mtr_diff" ) );
    mtrDiff->setNullCreator();
    mtrDiff->useAttrib<BaseColorAttrib>()->setValue( Color(0.89f, 0.88f, 0.87f, 1.0f) );

    MaterialPtr mtrRed( MaterialManager::createMaterial( "mtr_red" ) );
    mtrRed->setNullCreator();
    mtrRed->useAttrib<BaseColorAttrib>()->setValue( Color(0.7f, 0.7f, 0.7f, 1.0f) );

    MaterialPtr mtrGreen( MaterialManager::createMaterial( "mtr_green" ) );
    mtrGreen->setNullCreator();
    mtrGreen->useAttrib<BaseColorAttrib>()->setValue( Color(0.7f, 0.7f, 0.7f, 1.0f) );

    MaterialPtr mtrVtxClr( MaterialManager::createMaterial( "mtr_vtxclr" ) );
    mtrVtxClr->setNullCreator();
    mtrVtxClr->enableCommState( MtrCommState::VTXCLR, true );

    //////////////////////////////////////////////////////////////////////////
    // mesh
    MeshPtr meshBox( MeshManager::createBox( "mesh_box", 20, 20, 20, 6, 6, 6, true ) );
    meshBox->load( false );
    meshBox->setMaterialName( "mtr_diff" );

    MeshPtr meshBoxA( MeshManager::createBox( "mesh_box_a", 5, 5, 5, 4, 4, 4 ) );
    meshBoxA->load( false );
    meshBoxA->setMaterialName( "mtr_red" );

    MeshPtr meshBoxB( MeshManager::createBox( "mesh_box_b", 3.0f, 3.0f, 3.0f, 4, 4, 4 ) );
    meshBoxB->load( false );
    meshBoxB->setMaterialName( "mtr_green" );

    //////////////////////////////////////////////////////////////////////////
    // mesh node
    MeshNode* objBox = sg->createSceneNode<MeshNode>("objBox");
    objBox->setMesh( "mesh_box" );
    objBox->setPosition( 0, objBox->getWorldAABB().getHalfSize().y, 0 );
    area->addChild( objBox );

    MeshNode* objBoxA = sg->createSceneNode<MeshNode>("objBoxA");
    objBoxA->setMesh( "mesh_box_a" );
    objBoxA->setPosition( -2, objBoxA->getWorldAABB().getHalfSize().y, 2 );
    objBoxA->yaw( Math::toRadians(45) );
    area->addChild( objBoxA );

    MeshNode* objBoxB = sg->createSceneNode<MeshNode>("objBoxB");
    objBoxB->setMesh( "mesh_box_b" );
    objBoxB->setPosition( 3.7f, objBoxB->getWorldAABB().getHalfSize().y, 3.2f );
    area->addChild( objBoxB );

    // test gi mesh
    g_giSys = KHAOS_NEW GISystem;
    g_giSys->addNode( objBox );
    g_giSys->addNode( objBoxA );
    g_giSys->addNode( objBoxB );
    g_giSys->completeAddNode();
    g_giSys->getMesh()->testDirLit( Vector3(0.0f, 1.0f, 0.0f),  Color(0.9f, 0.9f, 0.9f) );

    for ( int i = 0; i < 1; ++i )
    {
        g_giSys->getMesh()->calcIndirectLit();
    }
    
    g_giSys->getMesh()->updateOutMesh();

    Mesh* meshGIOut = g_giSys->getMesh()->getOutMesh();
    meshGIOut->setMaterialName( "mtr_vtxclr" );

    MeshNode* objGI = sg->createSceneNode<MeshNode>("objGI");
    objGI->setMesh( meshGIOut->getName() );
    objGI->setPosition( 25, 0, 0 );
    area->addChild( objGI );

    //////////////////////////////////////////////////////////////////////////
    // light node
    LightNode* lit1 = sg->createSceneNode<LightNode>("lit1");
    lit1->lookAt( Vector3(0, 0, 0), Vector3(0.0f, 1.0f, 0.0f), Vector3::UNIT_Y );
    lit1->setLightType( LT_DIRECTIONAL );
    lit1->getLight()->setDiffuse( Color(0.9f, 0.9f, 0.9f) );

    area->addChild( lit1 );

    //////////////////////////////////////////////////////////////////////////
    // render setting
    RenderSettings* mainSettings = g_renderSettingManager->createRenderSettings( "mainSettings" );
    mainSettings->setRenderMode( RM_FORWARD );

    //////////////////////////////////////////////////////////////////////////
    // camera
    CameraNode* cam1 = sg->createSceneNode<CameraNode>("mainCamera");
    cam1->lookAt( Vector3(0, 7, 20), Vector3(0,0,0), Vector3::UNIT_Y );
    cam1->getCamera()->setPerspective( Math::toRadians(60), 4.0f/3.0f, 0.5f, 500.0f );
    cam1->getCamera()->setRenderSettings( mainSettings );
    area->addChild( cam1 );

    g_root->addActiveCamera( cam1 );

    //////////////////////////////////////////////////////////////////////////
    // operation
    m_samOp.init( cam1, 10.0f, 0.01f );

    return true;
}

bool Demo2Impl::_onUpdate()
{
    if ( !SampleFrame::_onUpdate() )
        return false;

    SceneGraph* sg = g_root->getSceneGraph( "demo" );
    return true;
}


//////////////////////////////////////////////////////////////////////////

Vector3 _testFunc1( float theta, float phi )
{
    return Vector3(
        2.0f,
        Math::cos(theta),
        3.0f * Math::cos(theta) * Math::cos(theta) + 0.2f
        );
}

void _testFunc1_Sample( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt )
{
    *(Vector3*)vals = _testFunc1(smpl.theta, smpl.phi);
}

Vector3 _testFunc2( const Vector3& dir )
{
    Vector3 lit(0,1,0);

    return Vector3(
        1.0f,
        Math::maxVal( lit.dotProduct( dir ), 0.0f ), // cos(t)
        2 * lit.dotProduct( dir ) * lit.dotProduct( dir ) + lit.dotProduct( dir ) + 0.2f
        );
}

void _testFunc2_Sample( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt )
{
    //*vals = _testFunc2(smpl.dir).y;
    *(Vector3*)vals = _testFunc2(smpl.dir);
}

void test_bent( SphereSamples* theSS, const SphereSamples::Sample& smpl, 
                   void* context, float* vals, int groupCnt )
{
    Vector3& ret = *(Vector3*)vals;

    if ( smpl.dir.x < 0 )
    {
        ret = Vector3::ZERO;
        //return;
    }

    ret = smpl.dir;
}

void Demo2Impl::_testSH()
{
    {
        RandSamples rands;
        rands.general( 1000, RandSamples::RT_Hammersley );

        SphereSampleMapper mapper;
        mapper.setRandsDistribution( &rands, SphereSampleMapper::HemiSphereCos );
        mapper.setDiffuseBRDFParas( Vector3::UNIT_Y );
        mapper.setCommConfig( true, false );
        mapper.general();

        SphereSamples theSS;
        theSS.setSamples( &mapper );

        Vector3 bent(Vector3::ZERO);
        theSS.integrate( test_bent, 0, bent.ptr(), 3 );

        bent /= Math::PI;

        Vector3 bent_n = bent.normalisedCopy();

        int a;
        a = 0;
    }
    //return;

    RandSamples rands;
    rands.general( 2000, RandSamples::RT_Jitter );

    SphereSampleMapper mapper;
    mapper.setRandsDistribution( &rands, SphereSampleMapper::EntireSphereUniform );
    mapper.setCommConfig( true, true );
    mapper.general();

    SphereSamples theSS;
    theSS.setSamples( &mapper );

    //////////////////////////////////////////////////////////////////////////
    // test func1
    {
        float coffs[16*3] = {};
        theSS.projectSH( _testFunc1_Sample, 0, coffs, 16, 3 );
 
        float theta = Math::PI/3;
        float phi = 0;
        Vector3 dir = SHMath::toXYZ( theta, phi );

        Vector3 reta = _testFunc1( theta, phi );
        Vector3 retb;
        SHMath::reconstructFunctionN( coffs, 16, 3, dir.x, dir.y, dir.z, retb.ptr() );
        
        Vector3 dx = reta - retb;
        dx = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    // test func2
    {
        Vector3 inDir(1,1,1);
        inDir.normalise();

        Quaternion rot( Math::PI / 3, Vector3::UNIT_X );
        Vector3 inDirRot = rot.inverse() * inDir; // 旋转坐标系相当于逆变换
        Vector3 reta = _testFunc2( inDirRot );

        const int groups = 3;
        float zcoffs[4*groups] = {};
        float coffs[16*groups] = {};

        Vector3 ydir = rot * Vector3::UNIT_Y;
        theSS.prepareZHDir( _testFunc2_Sample, 0, zcoffs, 4, groups );
        theSS.projectZHDir( ydir, zcoffs, coffs, 4, groups );

        Vector3 retb;
        SHMath::reconstructFunctionN( coffs, 16, groups, inDir.x, inDir.y, inDir.z, retb.ptr() );
        //retb = retb[0];
        Vector3 dx = reta - retb;
        dx = 0;
    }
}

