#include "KhaosPreHeaders.h"
#include "KhaosViewport.h"
#include "KhaosCamera.h"
#include "KhaosRenderTarget.h"
#include "KhaosRenderDevice.h"


namespace Khaos
{
    void Viewport::linkCamera( Camera* camera )
    {
        m_camera = camera;
        m_camera->_setViewport( this );
    }

    void Viewport::setRectByRtt( int level )
    {
        m_rect.set( 0, 0, m_parent->getWidth(level), m_parent->getHeight(level) );
    }

    void Viewport::apply()
    {
        g_renderDevice->setViewport( m_rect );

        if ( m_clearFlags )
        {
            g_renderDevice->clear( m_clearFlags, &m_clearColor, 1.0f, 0 );
        }
    }
}

