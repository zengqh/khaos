#include "KhaosPreHeaders.h"
#include "KhaosDeferredRendering.h"
#include "KhaosRenderSystem.h"
#include "KhaosRenderFeature.h"
#include "KhaosCamera.h"
#include "KhaosLightMgr.h"
#include "KhaosImageProcess.h"
#include "KhaosEffectContext.h"
#include "KhaosRenderDevice.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class ImgDepthBoundInit : public ImageProcess
    {
    public:
        ImgDepthBoundInit()
        {
            m_pin = _createImagePin( ET_DEPTHBOUNDINIT );
            m_pin->setOwnOutputEx( 0 );
            m_pin->setInputFilterEx( TextureFilterSet::NEAREST );
            _setRoot( m_pin );
        }

        void setOutput( TextureObj* texOut )
        {
            m_pin->getOutput()->linkRTT( texOut );
        }

        virtual void process( EffSetterParams& params )
        {
            ImageProcess::process( params );
        }

    private:
        ImagePin*   m_pin;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImgDepthBoundNext : public ImageProcess
    {
    public:
        ImgDepthBoundNext()
        {
            m_pin = _createImagePin( ET_DEPTHBOUNDNEXT );
            m_pin->setOwnOutputEx( 0 );
            m_pin->setInputFilterEx( TextureFilterSet::NEAREST );
            _setRoot( m_pin );
        }

        void setInput( TextureObj* texOut )
        {
            m_pin->setInput( texOut );
        }

        void setOutput( TextureObj* texOut )
        {
            m_pin->getOutput()->linkRTT( texOut );
        }

        virtual void process( EffSetterParams& params )
        {
            ImageProcess::process( params );
        }

    private:
        ImagePin*   m_pin;
    };

    //////////////////////////////////////////////////////////////////////////
    TBDR::TBDR() :
        m_cam(0),
        m_vpWidth(0), m_vpHeight(0), m_xgrids(0), m_ygrids(0), 
        m_litBuff(0),
        m_procDepthBoundInit(0), m_procDepthBoundNext(0)
    {

    }

    TBDR::~TBDR()
    {

    }

    void TBDR::init()
    {
        m_procDepthBoundInit = KHAOS_NEW ImgDepthBoundInit;
        m_procDepthBoundNext = KHAOS_NEW ImgDepthBoundNext;
    }

    void TBDR::shutdown()
    {
        KHAOS_DELETE m_procDepthBoundNext;
        KHAOS_DELETE m_procDepthBoundInit;
    }

    void TBDR::prepare( Camera* cam )
    {
        m_cam = cam;
        m_vpWidth = cam->getViewportWidth();
        m_vpHeight = cam->getViewportHeight();
        m_xgrids = m_vpWidth / TILE_GRID_SIZE;
        m_ygrids = m_vpHeight / TILE_GRID_SIZE;
    }

    void TBDR::generalLightBuffer( EffSetterParams& params )
    {
        RenderBufferPool::ItemTemp bufBound;
        _buildDepthBound( params, bufBound );
        _buildLightBuffer( params, bufBound );
    }

    void TBDR::_buildDepthBound( EffSetterParams& params, RenderBufferPool::ItemTemp& bufBound )
    {
        // 生成tile的包围体
        LightManager* litMgr = g_renderSystem->_getLitMgr();

        if ( !litMgr->hasDeferredLights() ) // 没有任何延迟灯光，啥都不用干了
            return;

        // step1: tile统计32->16
        // 期望的原始尺寸，tile_grid_size对齐
        int desiredSrcWidth  = (m_vpWidth + TILE_GRID_SIZE - 1) / TILE_GRID_SIZE * TILE_GRID_SIZE;
        int desiredSrcHeight = (m_vpHeight + TILE_GRID_SIZE - 1) / TILE_GRID_SIZE * TILE_GRID_SIZE;
        int destWidth  = desiredSrcWidth / 2;
        int destHeight = desiredSrcHeight / 2;

        bufBound.attach( g_renderSystem->_getRTTBufferTemp( PIXFMT_G16R16F, destWidth, destHeight ) );

        m_procDepthBoundInit->setOutput( bufBound );
        m_procDepthBoundInit->process( params );

        // step2: tile 16->8
        int curTileSize = TILE_GRID_SIZE / 2;

        while ( curTileSize > 1 )
        {
            destWidth /= 2;
            destHeight /= 2;

            RenderBufferPool::ItemTemp newDest = 
                g_renderSystem->_getRTTBufferTemp( PIXFMT_G16R16F, destWidth, destHeight );

            m_procDepthBoundNext->setInput( bufBound );
            m_procDepthBoundNext->setOutput( newDest );
            m_procDepthBoundNext->process( params );

            bufBound.swap( newDest ); // 新的dest作为下一次的源， 而且释放旧的源
        }
    }

    void TBDR::_buildLightBuffer( EffSetterParams& params, RenderBufferPool::ItemTemp& bufBound )
    {
        const PixelFormat litBufFmt = PIXFMT_A16B16G16R16F;

        // 绑定light-buffer
        khaosAssert(0);
        RenderTarget rttLitBuff ;//= g_renderSystem->_getLiBuf();
        m_litBuff = g_renderSystem->_getRTTBufferTemp( litBufFmt, m_vpWidth, m_vpHeight );
        SurfaceObj* surDepth = g_renderDevice->getMainDepthStencil()->getSurface(0);

        rttLitBuff.linkRTT( 0, m_litBuff->rtt );
        rttLitBuff.linkDepth( surDepth );

        // 累加各光线贡献
        params.clearAndStampGlobal();
        rttLitBuff.beginRender(0);

        Viewport* vpMain = rttLitBuff.getViewport(0);
        vpMain->setRectByRtt(0);
        vpMain->linkCamera( m_cam );
        vpMain->apply();


        rttLitBuff.endRender();
    }

    void TBDR::retrieveLightBuffer()
    {
        // 归还这个light-buffer
        if ( m_litBuff )
        {
            m_litBuff->setUnused();
            m_litBuff = 0;
        }
    }
}

