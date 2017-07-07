#include "KhaosPreHeaders.h"
#include "KhaosCamera.h"
#include "KhaosRenderDevice.h"
#include "KhaosViewport.h"

namespace Khaos
{
    int Camera::getViewportWidth() const
    {
        return m_viewport ? m_viewport->getWidth() : g_renderDevice->getWindowWidth();
    }

    int Camera::getViewportHeight() const
    {
        return m_viewport ? m_viewport->getHeight() : g_renderDevice->getWindowHeight();
    }

    Ray Camera::getRayByViewport( float sx, float sy ) const
    {
        Matrix4 vpInv = getViewProjMatrix().inverse();

        float nx = (2.0f * sx) - 1.0f;
        float ny = 1.0f - (2.0f * sy);

        Vector3 nearPoint(nx, ny, -1.f);

        // Use midPoint rather than far point to avoid issues with infinite projection
        Vector3 midPoint(nx, ny,  0.0f);

        // Get ray origin and ray target on near plane in world space
        Vector3 rayOrigin, rayTarget;

        rayOrigin = vpInv * nearPoint;
        rayTarget = vpInv * midPoint;

        Vector3 rayDirection = rayTarget - rayOrigin;
        rayDirection.normalise();

        return Ray( rayOrigin, rayDirection );
    }
}

