#pragma once
#include "KhaosViewport.h"
#include "KhaosIterator.h"
#include "KhaosRenderDeviceDef.h"

namespace Khaos
{
    class TextureObj;
    class SurfaceObj;

    enum RestoreFlag
    {
        RF_NONE,
        RF_ONLY_MULTI,
        RF_ALL
    };

    class RenderTarget : public AllocatedObject
    {
    public:
        typedef map<int, Viewport*>::type ViewportMap;
        typedef RangeIterator<ViewportMap> Iterator;

    public:
        RenderTarget();
        ~RenderTarget();

    public:
        Viewport* createViewport( int zOrder );
        Viewport* getOrCreateViewport( int zOrder );
        Viewport* getViewport( int zOrder ) const;

        void      destroyViewport( int zOrder );
        void      clearAllViewports();

        Iterator  getViewports() { return Iterator(m_viewports); }
        int       getViewportsCount() const { return (int)m_viewports.size(); }

    public:
        int getWidth( int level ) const;
        int getHeight( int level ) const;

        TextureObj* getRTT( int idx = 0 ) const;

    public:
        void linkRTT( TextureObj* rtt );
        void linkRTT( int idx, TextureObj* rtt );
        void linkDepth( SurfaceObj* depth );

        void setRestoreFlag( RestoreFlag rf ) { m_restoreFlag = rf; }

        void beginRender( int level );
        void endRender();

    private:
        ViewportMap     m_viewports;
        TextureObj*     m_rtt[MAX_MULTI_RTT_NUMS];
        SurfaceObj*     m_depth;
        RestoreFlag     m_restoreFlag;
    };


    //////////////////////////////////////////////////////////////////////////
    class RenderTargetCube : public AllocatedObject
    {
    public:
        RenderTargetCube();
        ~RenderTargetCube();

    public:
        Viewport* getViewport( CubeMapFace face ) const;

    public:
        void setSize( int size );
        int  getSize() const;

        void setClearFlag( uint32 flags );
        void setClearColor( const Color& clr );

        void linkCamera( Camera* cameras );
        void createInnerCamera();

    public:
        void setCamPos( const Vector3& pos );
        void setNearFar( float zn, float zf );

    public:
        void linkRTT( TextureObj* rtt );
        void linkDepth( SurfaceObj* depth );

        void setRestoreFlag( RestoreFlag rf ) { m_restoreFlag = rf; }

        void beginRender( CubeMapFace face, int level );
        void endRender();

    private:
        void _init();

    private:
        Viewport        m_viewports[6];
        Camera*         m_innerCamera;
        TextureObj*     m_rtt;
        SurfaceObj*     m_depth;
        RestoreFlag     m_restoreFlag;
    };
}

