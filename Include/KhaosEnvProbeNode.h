#pragma once
#include "KhaosSceneNode.h"

namespace Khaos
{
    class RenderTargetCube;

    class EnvProbe : public SceneObject
    {
    public:
        EnvProbe();
        ~EnvProbe();

        void setResolution( int r );
        void setNear( float n );
        void setFar( float f );
        void setPos( const Vector3& pos );

        int getResolution() const { return m_resolution; }

        RenderTargetCube* getRenderTarget() const { return m_rtt; }
        const AxisAlignedBox& getAABB() const { return m_aabb; }

    private:
        void _updateAABB();

    private:
        int               m_resolution;
        float             m_zNear;
        float             m_zFar;
        Vector3           m_pos;
        RenderTargetCube* m_rtt;
        AxisAlignedBox    m_aabb;
    };

    class EnvProbeNode : public SceneNode
    {
        KHAOS_DECLARE_RTTI(EnvProbeNode)

    public:
        EnvProbeNode();
        virtual ~EnvProbeNode();

    public:
        EnvProbe* getEnvProbe() const { return m_probe; }

    private:
        virtual bool _checkUpdateTransform();
        virtual void _makeWorldAABB();

    private:
        EnvProbe* m_probe;
    };
}

