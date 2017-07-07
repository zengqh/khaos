#pragma once
#include "KhaosResourceManager.h"
#include "KhaosMaterial.h"
#include "KhaosMesh.h"

namespace Khaos
{
    class Camera;

    class SysResManager : public AllocatedObject
    {
    public:
        struct Sys_Material { typedef Material ResType; KHAOS_DECLARE_RTTI(Sys_Material) };
        struct Sys_Texture  { typedef Texture ResType; KHAOS_DECLARE_RTTI(Sys_Texture) };
        struct Sys_Mesh     { typedef Mesh ResType; KHAOS_DECLARE_RTTI(Sys_Mesh) };

    public:
        SysResManager();
        ~SysResManager();

    public:
        void init();
        void shutdown();

        Material* getDefaultMaterial();
        Material* getMtrPointTmp();

        Material* getMtrFrontDrawAdd();
        Material* getMtrBackDrawAdd();

        Texture*  getTexRandom();
        Texture*  getTexRandomB();
        Texture*  getTexNormalFitting();
        Texture*  getTexArea();
        Texture*  getTexSearch();
        Texture*  getTexEnvLUT();

        Texture*  getTexPointTmp( int i = 0 );
        Texture*  getTexLinearTmp( int i = 0 );
        Texture*  getTexCustomTmp();

        Mesh*     getDebugMeshRect( int x, int y, int w, int h );
        Mesh*     getMeshFullScreenDS();
        Mesh*     getMeshFullScreenWPOS( Camera* cam, float z = 0 );

        Mesh*     getPointLitMesh();
        Mesh*     getSpotLitMesh();
        Mesh*     getVolProbeCubeMesh();

    public:
        template<class T>
        typename T::ResType* createResource( const String& name )
        {
            return static_cast<typename T::ResType*>( g_resourceManager->createResource( KHAOS_CLASS_TYPE(T), name ) );
        }

        template<class T>
        typename T::ResType* getResource( const String& name )
        {
            return static_cast<typename T::ResType*>( g_resourceManager->getResource( KHAOS_CLASS_TYPE(T), name ) );
        }

    private:
        Texture* _getTex( TexturePtr& ptr, pcstr name );

    private:
        MaterialPtr m_defaultMaterial;
        MaterialPtr m_mtrPointTmp;
        MaterialPtr m_mtrFrontDrawAdd;
        MaterialPtr m_mtrBackDrawAdd;

        TexturePtr  m_texRandom;
        TexturePtr  m_texRandomB;
        TexturePtr  m_texNormalFitting;
        TexturePtr  m_texArea;
        TexturePtr  m_texSearch;
        TexturePtr  m_texEnvLUT;

        TexturePtr  m_texPointTmp[8];
        TexturePtr  m_texLinearTmp[8];
        TexturePtr  m_texCustomTmp;

        MeshPtr     m_meshRect;
        MeshPtr     m_meshFullScreenDS;
        MeshPtr     m_meshFullScreenWPOS;

        MeshPtr     m_meshPointLit;
        MeshPtr     m_meshSpotLit;
        MeshPtr     m_meshVolProbeCube;
    };

    extern SysResManager* g_sysResManager;
}

