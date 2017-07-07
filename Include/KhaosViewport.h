#pragma once
#include "KhaosRect.h"
#include "KhaosColor.h"
#include "KhaosRenderDeviceDef.h"

namespace Khaos
{
    class Camera;
    class RenderTarget;

    class Viewport : public AllocatedObject
    {
    public:
        Viewport() : m_parent(0), m_camera(0), m_zOrder(0), 
            m_clearFlags(RCF_TARGET|RCF_DEPTH), m_clearColor(Color::BLACK) {}

    public:
        void _setParent( RenderTarget* rt ) { m_parent = rt; }
        void _setZOrder( int zOrder ) { m_zOrder = zOrder; }

        RenderTarget* getParent() const { return m_parent; }
        int getZOrder() const { return m_zOrder; }

    public:
        void linkCamera( Camera* camera );
        Camera* getCamera() const { return m_camera; }

    public:
        void setRect( const IntRect& rect )
        {
            m_rect = rect;
        }

        void setRect( int l, int t, int r, int b )
        {
            m_rect.set( l, t, r, b );
        }

        void makeRect( int l, int t, int w, int h )
        {
            m_rect.make( l, t, w, h );
        }

        void setRectByRtt( int level );

        int getWidth()  const { return m_rect.getWidth(); }
        int getHeight() const { return m_rect.getHeight(); }

    public:
        void setClearFlag( uint32 flags ) { m_clearFlags = flags; }
        void setClearColor( const Color& clr ) { m_clearColor = clr; }

        uint32 getClearFlag() const { return m_clearFlags; }

        void apply();

    private:
        RenderTarget* m_parent;
        Camera*       m_camera;
        int           m_zOrder;
        IntRect       m_rect;

        uint32        m_clearFlags;
        Color         m_clearColor;
    };
}

