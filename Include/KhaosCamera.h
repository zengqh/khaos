#pragma once
#include "KhaosFrustum.h"
#include "KhaosRay.h"

namespace Khaos
{
    class Viewport;
    class RenderSettings;

    class Camera : public Frustum
    {
    public:
        Camera() : m_viewport(0),m_renderSettings(0) {}
        virtual ~Camera() {}
    
    public:
        void _setViewport( Viewport* vp ) { m_viewport = vp; }
        //const Viewport* _getViewport() const { return m_viewport; }

        int getViewportWidth() const;
        int getViewportHeight() const;

        void setRenderSettings( RenderSettings* settings ) { m_renderSettings = settings; }
        RenderSettings* getRenderSettings() const { return m_renderSettings; }

        Ray getRayByViewport( float sx, float sy ) const;

    protected:
        Viewport*       m_viewport;
        RenderSettings* m_renderSettings;
    };
}

