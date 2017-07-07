#pragma once
#include "KhaosSceneNode.h"
#include "KhaosRenderable.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class VolumeProbeNode;

    class VolProbRenderable : public Renderable
    {
    public:
        VolProbRenderable() : m_node(0), m_mtr(0) {}

        void _setNode( VolumeProbeNode* n ) { m_node = n; }
        VolumeProbeNode* getNode() const{ return m_node; }

        void _setImmMtr( Material* mtr ) { m_mtr = mtr; }
        virtual Material* _getImmMaterial() { return m_mtr; }

        virtual const Matrix4& getImmMatWorld();
        virtual const AxisAlignedBox& getImmAABBWorld();

        virtual void render();

    private:
        VolumeProbeNode* m_node;
        Material*  m_mtr;
    };

    //////////////////////////////////////////////////////////////////////////
    class VolumeProbe : public SceneObject
    {
    public:
        VolumeProbe( SceneNode* owner );
        ~VolumeProbe();

    public:
        void setVolID( int volID );
        int  getVolID() const { return m_volID; }

        void setResolution( int cx, int cy, int cz );
        int  getResolutionX() const { return m_resolutionX; }
        int  getResolutionY() const { return m_resolutionY; }
        int  getResolutionZ() const { return m_resolutionZ; }

        void load( const String& files );
        void load( int volID, const String& files );
        void unload();

        bool isPrepared() const;

    public:
        void updateMatrix();

        const Matrix4& getWorldMatrix() const;
        Vector3        getCenterWorldPos( int ix, int iy, int iz ) const;
        const Vector3& getGridSize() const;
        const Vector3& getVolumeSize() const;

    private:
        SceneNode*        m_owner;
        int               m_volID;
        int               m_resolutionX;
        int               m_resolutionY;
        int               m_resolutionZ;
        Vector3           m_volumeSize;
        Vector3           m_gridSize;
    };

    class VolumeProbeNode : public SceneNode
    {
        KHAOS_DECLARE_RTTI(VolumeProbeNode)

    public:
        VolumeProbeNode();
        virtual ~VolumeProbeNode();

    public:
        VolumeProbe* getProbe() const { return m_probe; }
        VolProbRenderable* getRenderable() { return &m_volProbeRenderable; }

    private:
        virtual bool _checkUpdateTransform();
        virtual void _makeWorldAABB();

        virtual void _onDestroyBefore();

    private:
        VolumeProbe* m_probe;
        VolProbRenderable m_volProbeRenderable;
    };
}

