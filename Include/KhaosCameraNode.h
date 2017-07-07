#pragma once
#include "KhaosSceneNode.h"
#include "KhaosCamera.h"


namespace Khaos
{
    class CameraNode : public SceneNode
    {
        KHAOS_DECLARE_RTTI(CameraNode)

    public:
        CameraNode();
        virtual ~CameraNode();

    public:
        Camera* getCamera() const { return m_camera; }

        bool testVisibility( const AxisAlignedBox& bound ) const;
        Frustum::Visibility testVisibilityEx( const AxisAlignedBox& bound ) const;

    private:
        virtual bool _checkUpdateTransform();
        virtual void _makeWorldAABB();

    private:
        Camera* m_camera;
    };
}

