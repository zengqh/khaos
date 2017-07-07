#include "KhaosPreHeaders.h"
#include "KhaosRenderTarget.h"
#include "KhaosRenderDevice.h"
#include "KhaosCamera.h"


namespace Khaos
{
    RenderTarget::RenderTarget() : m_depth(0), m_restoreFlag(RF_ONLY_MULTI)
    {
        KHAOS_CLEAR_ARRAY( m_rtt );
    }

    RenderTarget::~RenderTarget()
    {
        clearAllViewports();
    }

    Viewport* RenderTarget::createViewport( int zOrder )
    {
        if ( m_viewports.find(zOrder) != m_viewports.end() )
        {
            khaosLogLn( KHL_L2, "RenderTarget::createViewport: %d exist", zOrder );
            return 0;
        }

        Viewport* vp = KHAOS_NEW Viewport;
        vp->_setParent( this );
        vp->_setZOrder( zOrder );
        m_viewports[zOrder] = vp;
        return vp;
    }

    Viewport* RenderTarget::getOrCreateViewport( int zOrder )
    {
        Viewport*& vp = m_viewports[zOrder];
        if ( vp )
            return vp;

        vp = KHAOS_NEW Viewport;
        vp->_setParent( this );
        vp->_setZOrder( zOrder );
        return vp;
    }

    Viewport* RenderTarget::getViewport( int zOrder ) const
    {
        ViewportMap::const_iterator it = m_viewports.find(zOrder);
        if ( it != m_viewports.end() )
            return it->second;
        return 0;
    }

    void RenderTarget::destroyViewport( int zOrder )
    {
        ViewportMap::const_iterator it = m_viewports.find(zOrder);
        if ( it != m_viewports.end() )
        {
            Viewport* vp = it->second;
            KHAOS_DELETE vp;
            m_viewports.erase( it );
        }
    }

    void RenderTarget::clearAllViewports()
    {
        KHAOS_FOR_EACH( ViewportMap, m_viewports, it )
        {
            Viewport* vp = it->second;
            KHAOS_DELETE vp;
        }

        m_viewports.clear();
    }

    int RenderTarget::getWidth( int level ) const
    {
        return m_rtt[0] ? m_rtt[0]->getSurface(level)->getWidth() : 0;
    }

    int RenderTarget::getHeight( int level ) const
    {
        return m_rtt[0] ? m_rtt[0]->getSurface(level)->getHeight() : 0;
    }

    void RenderTarget::linkRTT( TextureObj* rtt )
    {
        linkRTT(0, rtt );
    }

    void RenderTarget::linkRTT( int idx, TextureObj* rtt )
    {
        khaosAssert( 0 <= idx && idx < MAX_MULTI_RTT_NUMS );
        m_rtt[idx] = rtt;
    }

    TextureObj* RenderTarget::getRTT( int idx ) const
    {
        khaosAssert( 0 <= idx && idx < MAX_MULTI_RTT_NUMS );
        return m_rtt[idx];
    }

    void RenderTarget::linkDepth( SurfaceObj* depth )
    {
        m_depth = depth;
    }

    void RenderTarget::beginRender( int level )
    {
        for ( int i = 0; i < MAX_MULTI_RTT_NUMS; ++i )
        {
            TextureObj* rtt = m_rtt[i];
            if ( rtt )
                g_renderDevice->setRenderTarget( i, rtt->getSurface( level ) );
            else
                break;
        }
        
        if ( m_depth )
            g_renderDevice->setDepthStencil( m_depth );
    }

    void RenderTarget::endRender()
    {
        if ( m_restoreFlag == RF_NONE )
        {
            return;
        }
        else if ( m_restoreFlag == RF_ONLY_MULTI )
        {
            for ( int i = 1; i < MAX_MULTI_RTT_NUMS; ++i )
            {
                if ( m_rtt[i] )
                    g_renderDevice->restoreMainRenderTarget(i);
                else
                    break;
            }
        }
        else if ( m_restoreFlag == RF_ALL )
        {
            for ( int i = 0; i < MAX_MULTI_RTT_NUMS; ++i )
            {
                if ( m_rtt[i] )
                    g_renderDevice->restoreMainRenderTarget(i);
                else
                    break;
            }

            if ( m_depth )
                g_renderDevice->restoreMainDepthStencil();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    RenderTargetCube::RenderTargetCube() : m_innerCamera(0), m_rtt(0), m_depth(0), m_restoreFlag(RF_NONE)
    {
    }

    RenderTargetCube::~RenderTargetCube()
    {
        KHAOS_DELETE []m_innerCamera;
    }

    Viewport* RenderTargetCube::getViewport( CubeMapFace face ) const
    {
        return const_cast<Viewport*>(&m_viewports[face]);
    }

    void RenderTargetCube::setSize( int size )
    {
        for ( int i = 0; i < 6; ++i )
        {
            m_viewports[i].setRect( 0, 0, size, size );
        }
    }

    int RenderTargetCube::getSize() const
    {
        return m_viewports->getWidth();
    }

    void RenderTargetCube::setClearFlag( uint32 flags )
    {
        for ( int i = 0; i < 6; ++i )
        {
            m_viewports[i].setClearFlag( flags );
        }
    }

    void RenderTargetCube::setClearColor( const Color& clr )
    {
        for ( int i = 0; i < 6; ++i )
        {
            m_viewports[i].setClearColor( clr );
        }
    }

    void RenderTargetCube::linkCamera( Camera* cameras )
    {
        for ( int i = 0; i < 6; ++i )
        {
            m_viewports[i].linkCamera( cameras + i );
        }
    }

    void RenderTargetCube::createInnerCamera()
    {
        m_innerCamera = KHAOS_NEW Camera[6];
        linkCamera( m_innerCamera );
    }

    void RenderTargetCube::setCamPos( const Vector3& pos )
    {
        static const Vector3 targetPos[6] =
        {
            Vector3::UNIT_X,
            Vector3::NEGATIVE_UNIT_X,

            Vector3::UNIT_Y,
            Vector3::NEGATIVE_UNIT_Y,

            Vector3::NEGATIVE_UNIT_Z, // d3d flip z
            Vector3::UNIT_Z
        };

        static const Vector3 upDir[6] =
        {
            Vector3::UNIT_Y,
            Vector3::UNIT_Y,

            Vector3::UNIT_Z,
            Vector3::NEGATIVE_UNIT_Z,

            Vector3::UNIT_Y,
            Vector3::UNIT_Y
        };

        for ( int i = 0; i < 6; ++i )
        {
            Camera* cam = m_viewports[i].getCamera();
            if ( cam )
                cam->setTransform( pos, pos+targetPos[i], upDir[i] );
        }
    }

    void RenderTargetCube::setNearFar( float zn, float zf )
    {
        for ( int i = 0; i < 6; ++i )
        {
            Camera* cam = m_viewports[i].getCamera();
            if ( cam )
                cam->setPerspective( Math::HALF_PI, 1.0f, zn, zf );
        }
    }

    void RenderTargetCube::linkRTT( TextureObj* rtt )
    {
        m_rtt = rtt;
    }

    void RenderTargetCube::linkDepth( SurfaceObj* depth )
    {
        m_depth = depth;
    }

    void RenderTargetCube::beginRender( CubeMapFace face, int level )
    {
        g_renderDevice->setRenderTarget( 0, m_rtt->getSurface(face, level) );

        if ( m_depth )
            g_renderDevice->setDepthStencil( m_depth );

        m_viewports[face].apply();
    }

    void RenderTargetCube::endRender()
    {
        if ( m_restoreFlag == RF_NONE )
        {
            return;
        }
        else if ( m_restoreFlag == RF_ALL )
        {
            g_renderDevice->restoreMainRenderTarget(0);

            if ( m_depth )
                g_renderDevice->restoreMainDepthStencil();
        }
    }
}

