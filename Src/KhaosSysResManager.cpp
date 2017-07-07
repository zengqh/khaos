#include "KhaosPreHeaders.h"
#include "KhaosSysResManager.h"
#include "KhaosMaterialManager.h"
#include "KhaosTextureManager.h"
#include "KhaosMeshManager.h"
#include "KhaosRenderDevice.h"
#include "KhaosMeshManualCreator.h"
#include "KhaosCamera.h"

namespace Khaos
{
    SysResManager* g_sysResManager = 0;

    SysResManager::SysResManager()
    {
        khaosAssert( !g_sysResManager );
        g_sysResManager = this;
    }

    SysResManager::~SysResManager()
    {
        g_sysResManager = 0;
    }

    void SysResManager::init()
    {
        g_resourceManager->registerGroup( KHAOS_CLASS_TYPE(Sys_Material), MaterialManager::_createFunc,
            KHAOS_NEW MaterialResAutoCreator, g_defaultResourceScheduler );

        g_resourceManager->registerGroup( KHAOS_CLASS_TYPE(Sys_Texture), TextureManager::_createFunc,
            KHAOS_NEW TextureResAutoCreator, g_defaultResourceScheduler );

        g_resourceManager->registerGroup( KHAOS_CLASS_TYPE(Sys_Mesh), MeshManager::_createFunc,
            KHAOS_NEW MeshResAutoCreator, g_defaultResourceScheduler );
    }

    void SysResManager::shutdown()
    {
    }

    Material* SysResManager::getDefaultMaterial()
    {
        if ( m_defaultMaterial )
            return m_defaultMaterial.get();

        m_defaultMaterial.attach( 
            static_cast<Material*>( createResource<Sys_Material>( "$Default" ) )
        );

        m_defaultMaterial->setNullCreator();
        return m_defaultMaterial.get();
    }

    Material* SysResManager::getMtrPointTmp()
    {
        if ( m_mtrPointTmp )
            return m_mtrPointTmp.get();

        m_mtrPointTmp.attach( 
            static_cast<Material*>( createResource<Sys_Material>( "$MtrPointTmp" ) )
        );

        m_mtrPointTmp->setNullCreator();
        BaseMapAttrib* attr = m_mtrPointTmp->useAttrib<BaseMapAttrib>();
        attr->setTexture( getTexPointTmp() );

        return m_mtrPointTmp.get();
    }

    Material* SysResManager::getMtrFrontDrawAdd()
    {
        if ( m_mtrFrontDrawAdd )
            return m_mtrFrontDrawAdd.get();

        m_mtrFrontDrawAdd.attach( 
            static_cast<Material*>( createResource<Sys_Material>( "$MtrFrontDrawAdd" ) )
        );

        m_mtrFrontDrawAdd->setNullCreator();
        m_mtrFrontDrawAdd->setMaterialState( MaterialStateSet::FRONT_DRAW );
        m_mtrFrontDrawAdd->setBlendState( BlendStateSet::ADD );

        return m_mtrFrontDrawAdd.get();
    }

    Material* SysResManager::getMtrBackDrawAdd()
    {
        if ( m_mtrBackDrawAdd )
            return m_mtrBackDrawAdd.get();

        m_mtrBackDrawAdd.attach( 
            static_cast<Material*>( createResource<Sys_Material>( "$MtrBackDrawAdd" ) )
        );

        m_mtrBackDrawAdd->setNullCreator();
        m_mtrBackDrawAdd->setMaterialState( MaterialStateSet::BACK_DRAW );
        m_mtrBackDrawAdd->setBlendState( BlendStateSet::ADD );

        return m_mtrBackDrawAdd.get();
    }

    Texture* SysResManager::_getTex( TexturePtr& ptr, pcstr name )
    {
        if ( ptr )
            return ptr.get();

        ptr.attach( 
            static_cast<Texture*>( createResource<Sys_Texture>( name ) )
            );

        ptr->load( false );
        return ptr.get();
    }

    Texture* SysResManager::getTexRandom()
    {
        return _getTex( m_texRandom, "/System/random.png" );
    }

    Texture* SysResManager::getTexRandomB()
    {
        return _getTex( m_texRandomB, "/System/randomB.png" );
    }

    Texture* SysResManager::getTexNormalFitting()
    {
        return _getTex( m_texNormalFitting, "/System/NormalsFitting.dds" );
    }

    Texture* SysResManager::getTexArea()
    {
        return _getTex( m_texArea, "/System/AreaTex.dds" );
    }

    Texture* SysResManager::getTexSearch()
    {
        return _getTex( m_texSearch, "/System/SearchTex.dds" );
    }

    Texture* SysResManager::getTexEnvLUT()
    {
        return _getTex( m_texEnvLUT, "/System/EnvLUT.dds" );
    }

    Texture* SysResManager::getTexPointTmp( int i )
    {
        if ( m_texPointTmp[i] )
            return m_texPointTmp[i].get();

        String name = "$TexPointTmp" + intToString(i);

        m_texPointTmp[i].attach( 
            static_cast<Texture*>( createResource<Sys_Texture>( name ) )
        );

        m_texPointTmp[i]->setNullCreator();
        m_texPointTmp[i]->getSamplerState().setFilter( TextureFilterSet::NEAREST );
        m_texPointTmp[i]->getSamplerState().setAddress( TextureAddressSet::CLAMP );
        return m_texPointTmp[i].get();
    }

    Texture* SysResManager::getTexLinearTmp( int i )
    {
        if ( m_texLinearTmp[i] )
            return m_texLinearTmp[i].get();

        String name = "$TexLinearTmp" + intToString(i);

        m_texLinearTmp[i].attach( 
            static_cast<Texture*>( createResource<Sys_Texture>( name ) )
            );

        m_texLinearTmp[i]->setNullCreator();
        m_texLinearTmp[i]->getSamplerState().setFilter( TextureFilterSet::BILINEAR );
        m_texLinearTmp[i]->getSamplerState().setAddress( TextureAddressSet::CLAMP );
        return m_texLinearTmp[i].get();
    }

    Texture* SysResManager::getTexCustomTmp()
    {
        if ( m_texCustomTmp )
            return m_texCustomTmp.get();

        String name = "$TexCustomTmp";

        m_texCustomTmp.attach( 
            static_cast<Texture*>( createResource<Sys_Texture>( name ) )
            );

        m_texCustomTmp->setNullCreator();
        return m_texCustomTmp.get();
    }

    Mesh* SysResManager::getDebugMeshRect( int x, int y, int w, int h )
    {
        // first init
        if ( !m_meshRect )
        {
            m_meshRect.attach( 
                static_cast<Mesh*>( createResource<Sys_Mesh>( "$MeshRect" ) )
            );

            m_meshRect->setNullCreator();

            SubMesh* subMesh = m_meshRect->createSubMesh();
            subMesh->setPrimitiveType( PT_TRIANGLELIST );

            VertexPT vts[4] = {};
            VertexBuffer* vb = subMesh->createVertexBuffer();
            vb->create( sizeof(vts), HBU_STATIC );
            vb->setDeclaration( g_vertexDeclarationManager->getDeclaration(VertexPT::ID) );

            ushort ids[] = { 0,1,2, 2,3,0 };
            IndexBuffer* ib = subMesh->createIndexBuffer();
            ib->create( sizeof(ids), HBU_STATIC, IET_INDEX16 );
            ib->fillData( ids );
        }

        // vb
        Matrix3 matViewportToProj;
        g_renderDevice->makeMatrixViewportToProj( matViewportToProj, 0, 0, true ); // 只支持全屏的视口

        Vector3 leftTop((float)x, (float)y, 1);
        Vector3 leftBottom((float)x, (float)(y+h), 1);
        Vector3 rightBottom((float)(x+w), (float)(y+h), 1);
        Vector3 rightTop((float)(x+w), (float)y, 1);

        leftTop     = matViewportToProj * leftTop;
        leftBottom  = matViewportToProj * leftBottom;
        rightBottom = matViewportToProj * rightBottom;
        rightTop    = matViewportToProj * rightTop;

        VertexPT vts[4] =
        {
            leftTop.x,     leftTop.y,     0.0f,   0.0f, 0.0f,
            leftBottom.x,  leftBottom.y,  0.0f,   0.0f, 1.0f,
            rightBottom.x, rightBottom.y, 0.0f,   1.0f, 1.0f,
            rightTop.x,    rightTop.y,    0.0f,   1.0f, 0.0f
        };

        SubMesh* subMesh = m_meshRect->getSubMesh(0);
        subMesh->getVertexBuffer()->fillData( vts );

        return m_meshRect.get();
    }

    Mesh* SysResManager::getMeshFullScreenDS()
    {
        if ( !m_meshFullScreenDS )
        {
            m_meshFullScreenDS.attach( 
                static_cast<Mesh*>( createResource<Sys_Mesh>( "$MeshFullScreenDS" ) )
            );

            m_meshFullScreenDS->setNullCreator();

            SubMesh* subMesh = m_meshFullScreenDS->createSubMesh();
            subMesh->setPrimitiveType( PT_TRIANGLELIST );

            const float z = 0.5f;

            VertexP vts[4] = 
            {
                -1,   1,  z, // project空间的点
                -1,  -1,  z,
                 1,  -1 , z,
                 1,   1,  z
            };
            
            VertexBuffer* vb = subMesh->createVertexBuffer();
            vb->create( sizeof(vts), HBU_STATIC );
            vb->setDeclaration( g_vertexDeclarationManager->getDeclaration(VertexP::ID) );
            vb->fillData( vts );

            ushort ids[] = { 0,1,2, 2,3,0 };
            IndexBuffer* ib = subMesh->createIndexBuffer();
            ib->create( sizeof(ids), HBU_STATIC, IET_INDEX16 );
            ib->fillData( ids );
        }

        return m_meshFullScreenDS.get();
    }

    Mesh* SysResManager::getMeshFullScreenWPOS( Camera* cam, float z )
    {
        if ( !m_meshFullScreenWPOS )
        {
            m_meshFullScreenWPOS.attach( 
                static_cast<Mesh*>( createResource<Sys_Mesh>( "$MeshFullScreenWPOS" ) )
                );

            m_meshFullScreenWPOS->setNullCreator();

            SubMesh* subMesh = m_meshFullScreenWPOS->createSubMesh();
            subMesh->setPrimitiveType( PT_TRIANGLELIST );

            VertexBuffer* vb = subMesh->createVertexBuffer();
            vb->create( sizeof(VertexPNT) * 4, HBU_DYNAMIC );
            vb->setDeclaration( g_vertexDeclarationManager->getDeclaration(VertexPNT::ID) );

            ushort ids[] = { 0,1,2, 2,3,0 };
            IndexBuffer* ib = subMesh->createIndexBuffer();
            ib->create( sizeof(ids), HBU_STATIC, IET_INDEX16 );
            ib->fillData( ids );
        }

        // 构造全屏带camVec的矩形

        // for dx9 only
        int nTexWidth = cam->getViewportWidth();
        int nTexHeight = cam->getViewportHeight();

        Vector2 halfOffset = g_renderDevice->getHalfTexelOffset();

        float hu = halfOffset.x / (float)nTexWidth;
        float hV = halfOffset.y / (float)nTexHeight;  

        const Vector3* cv = cam->getCamVecs();

        VertexPNT pScreenQuad[] =
        {
            { 0, 0, z, cv[1].x, cv[1].y, cv[1].z,   hu,   hV }, // lt
            { 0, 1, z, cv[2].x, cv[2].y, cv[2].z,   hu, 1+hV }, // lb
            { 1, 1, z, cv[3].x, cv[3].y, cv[3].z, 1+hu, 1+hV }, // rb
            { 1, 0, z, cv[0].x, cv[0].y, cv[0].z, 1+hu,   hV }  // rt
        };

        m_meshFullScreenWPOS->getSubMesh(0)->getVertexBuffer()->fillData( pScreenQuad );

        return m_meshFullScreenWPOS.get();
    }

    Mesh* SysResManager::getPointLitMesh()
    {
        if ( !m_meshPointLit )
        {
            m_meshPointLit.attach( static_cast<Mesh*>(createResource<Sys_Mesh>("_PointLit")) );

            // 要求最小的半径和切分数
            const float minRadius = 1;
            const int   segments  = 15;

            float radius = minRadius / Math::cos( Math::PI / (segments - 1) );
            float size = radius * 2;
            m_meshPointLit->setResCreator( KHAOS_NEW MeshSphereCreatorImpl(size, segments) );
            m_meshPointLit->load( false );
        }

        return m_meshPointLit.get();
    }

    Mesh* SysResManager::getSpotLitMesh()
    {
        if ( !m_meshSpotLit )
        {
            m_meshSpotLit.attach( static_cast<Mesh*>(createResource<Sys_Mesh>("_SpotLit")) );

            // 要求最小的半径和切分数
            const float minRadius = 1;
            const float height    = 1;
            const int   wsegments = 20;
            const int   hsegments = 2;

            float radius = minRadius / Math::cos( Math::PI / (wsegments - 1) );
            float width = radius * 2;
            
            m_meshSpotLit->setResCreator( 
                KHAOS_NEW MeshConeCreatorImpl(width, height, wsegments, hsegments, true) 
                );

            m_meshSpotLit->load( false );
        }

        return m_meshSpotLit.get();
    }

    Mesh* SysResManager::getVolProbeCubeMesh()
    {
        if ( !m_meshVolProbeCube )
        {
            m_meshVolProbeCube.attach( static_cast<Mesh*>(createResource<Sys_Mesh>("_VolProbeCube")) );
            m_meshVolProbeCube->setResCreator( KHAOS_NEW MeshCubeCreatorImpl( 1.0f ) );
            m_meshVolProbeCube->load( false );
        }

        return m_meshVolProbeCube.get();
    }
}

