#pragma once
#include "KhaosSceneNode.h"
#include "KhaosMesh.h"
#include "KhaosMaterial.h"
#include "KhaosRenderable.h"

namespace Khaos
{
    class MeshNode;

    //////////////////////////////////////////////////////////////////////////
    class SubEntity : public Renderable
    {
    public:
        SubEntity() : m_meshNode(0), m_subIndex(0), m_aabbWorldDirty(true) {}
        virtual ~SubEntity() {}

    public:
        void _setMeshNode( MeshNode* node ) { m_meshNode = node; }
        void _setSubIndex( int index ) { m_subIndex = index; }

        void setMaterial( const String& materialName );
        Material* getMaterial() const { return m_material.get(); }
        MaterialPtr getMaterialPtr() const { return m_material; }

        const AxisAlignedBox& getAABBWorld() const;

    public:
        void _addToRenderer();
        void _setAABBWorldDirty() { m_aabbWorldDirty = true; }

    public:
        // Renderable
        virtual const Matrix4&        getImmMatWorld();
        virtual const AxisAlignedBox& getImmAABBWorld();
        virtual Material*             _getImmMaterial();
        virtual LightsInfo*           getImmLightInfo();
        virtual bool                  isReceiveShadow();
        virtual RenderableSharedData* getRDSharedData();
        virtual void                  render();

    private:
        MeshNode*       m_meshNode;
        int             m_subIndex;
        AxisAlignedBox  m_aabbWorld;
        MaterialPtr     m_material;
        bool            m_aabbWorldDirty;
    };

    //////////////////////////////////////////////////////////////////////////
    class MeshNode : public RenderableSceneNode, public IResourceListener
    {
        KHAOS_DECLARE_RTTI(MeshNode)

    public:
        typedef vector<SubEntity*>::type SubEntityList;

    public:
        MeshNode();
        virtual ~MeshNode();

    public:
        void setMesh( const String& name );
        Mesh* getMesh() const { return m_mesh.get(); }

    public:
        void setMaterial( const String& materialName );

        SubEntity* getSubEntity( int i ) const { return m_subEntityList[i]; }
        int        getSubEntityCount() const { return (int)m_subEntityList.size(); }

    private:
        // SceneNode
        virtual void _addToRenderer();
        virtual void _setWorldAABBDirty();
        virtual void _makeWorldAABB();

        // IResourceListener
        virtual void onResourceInitialized( Resource* res );
        virtual void onResourceLoaded( Resource* res );
        virtual void onResourceUpdate( Resource* res ) ;

    private:
        void _clearSubEntity();
        void _initSubEntity();

    public:
        void _holdTempMtr( vector<MaterialPtr>::type& mtrs );

    private:
        MeshPtr       m_mesh;
        SubEntityList m_subEntityList;
    };
}

