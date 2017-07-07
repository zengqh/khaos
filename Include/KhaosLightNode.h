#pragma once
#include "KhaosSceneNode.h"
#include "KhaosLight.h"
#include "KhaosRenderable.h"

namespace Khaos
{
    class LightNode;

    class VolumeRenderable : public Renderable
    {
    public:
        VolumeRenderable() : m_node(0), m_mtr(0) {}

        void _setNode( LightNode* n ) { m_node = n; }
        LightNode* getNode() const{ return m_node; }

        void _setImmMtr( Material* mtr ) { m_mtr = mtr; }
        virtual Material* _getImmMaterial() { return m_mtr; }

        virtual const Matrix4& getImmMatWorld();
        virtual const AxisAlignedBox& getImmAABBWorld();

        virtual void render();

    private:
        LightNode* m_node;
        Material*  m_mtr;
    };

    class LightNode : public SceneNode
    {
        KHAOS_DECLARE_RTTI(LightNode)

    public:
        LightNode();
        virtual ~LightNode();

    public:
        void   setLightType( LightType type );
        LightType getLightType() const{ return m_light->getLightType(); }

        Light* getLight() const { return m_light; }

        template<class T>
        T*    getLightAs() const { return static_cast<T*>(m_light); }

        bool  intersects( const AxisAlignedBox& aabb ) const;
        float squaredDistance( const Vector3& pos ) const;

        VolumeRenderable* getVolumeRenderable() { return &m_volRenderable; }

    private:
        virtual bool _checkUpdateTransform();
        virtual void _makeWorldAABB();

    private:
        Light*  m_light;
        VolumeRenderable m_volRenderable;
    };
}

