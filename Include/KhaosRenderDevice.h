#pragma once
#include "KhaosVertexIndexBuffer.h"
#include "KhaosTextureObj.h"
#include "KhaosShader.h"
#include "KhaosRect.h"
#include "KhaosRenderDeviceDef.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    struct RenderDeviceCreateContext
    {
        RenderDeviceCreateContext() : handleWindow(0), windowWidth(0), windowHeight(0) {}

        void*       handleWindow;
        int         windowWidth;
        int         windowHeight;
    };

    //////////////////////////////////////////////////////////////////////////
    class RenderDevice : public AllocatedObject
    {
    public:
        RenderDevice();
        virtual ~RenderDevice();

    public:
        virtual void init( const RenderDeviceCreateContext& context ) = 0;
        virtual void shutdown() = 0;

        virtual VertexBuffer*       createVertexBuffer() = 0;
        virtual VertexDeclaration*  createVertexDeclaration() = 0;
        virtual IndexBuffer*        createIndexBuffer() = 0;
        virtual TextureObj*         createTextureObj() = 0;
        virtual SurfaceObj*         createSurfaceObj() = 0;
        virtual Effect*             createEffect() = 0;

        virtual bool isTextureFormatSupported( TextureUsage usage, TextureType type, PixelFormat fmt ) = 0;

        virtual void beginRender() = 0;
        virtual void endRender() = 0;

        virtual void _setRenderTarget( int idx, SurfaceObj* surTarget ) = 0;
        virtual void _setDepthStencil( SurfaceObj* surDepthStencil ) = 0;
        virtual void setViewport( const IntRect& rect ) = 0;
        virtual void clear( uint32 flags, const Color* clr, float z, uint stencil ) = 0;

        virtual void _setEffect( Effect* effect ) = 0;
        virtual void setVertexBuffer( VertexBuffer* vb ) = 0;
        virtual void setIndexBuffer( IndexBuffer* ib ) = 0;
        virtual void drawIndexedPrimitive( PrimitiveType type, int startIndex, int primitiveCount ) = 0;
        virtual void drawPrimitive( PrimitiveType type, int startVertex, int primitiveCount ) = 0;

        // color
        virtual void enableColorWriteenable( int rtIdx, bool enR, bool enG, bool enB, bool enA ) = 0;
        virtual void enableSRGBWrite( bool en ) = 0;

        // depth
        virtual void _enableZTest( bool en ) = 0;
        virtual void _enableZWrite( bool en ) = 0;
        virtual void _setZFunc( CmpFunc func ) = 0;

        virtual void setDepthBias( float sca, float off ) = 0;

        // stencil
        virtual void _enableStencil( bool en ) = 0;

        virtual void _setStencil_refVal( uint8 refVal ) = 0;
        virtual void _setStencil_cmpFunc( CmpFunc cmpFunc ) = 0;
        virtual void _setStencil_cmpMask( uint8 cmpMask ) = 0;
        virtual void _setStencil_writeMask( uint8 writeMask ) = 0;

        virtual void _setStencil_stencilFailOp( StencilOp stencilFailOp ) = 0;
        virtual void _setStencil_zFailOp( StencilOp zFailOp ) = 0;
        virtual void _setStencil_bothPassOp( StencilOp bothPassOp ) = 0;

        // blend
        virtual void _enableBlend( bool en ) = 0;
        virtual void _setBlendOp( BlendOp op ) = 0;
        virtual void _setSrcBlend( BlendVal val ) = 0;
        virtual void _setDestBlend( BlendVal val ) = 0;

        // mtr
        virtual void _setCullMode( CullMode cm ) = 0;
        virtual void _setWireframe( bool en ) = 0;

        // sample
        virtual void _setTexture( int sampler, TextureObj* texObj ) = 0;

        virtual void _setMagFilter( int sampler, TextureFilter tf ) = 0;
        virtual void _setMinFilter( int sampler, TextureFilter tf ) = 0;
        virtual void _setMipFilter( int sampler, TextureFilter tf ) = 0;

        virtual void _setTexAddrU( int sampler, TextureAddress addr ) = 0;
        virtual void _setTexAddrV( int sampler, TextureAddress addr ) = 0;
        virtual void _setTexAddrW( int sampler, TextureAddress addr ) = 0;

        virtual void _setBorderColor( int sampler, const Color& clr ) = 0;

        virtual void _setMipLodBias( int sampler, float mipLodBias ) = 0;
        virtual void _setMipMaxLevel( int sampler, int mipMaxLevel ) = 0;
        virtual void _setMaxAnisotropy( int sampler, int maxAnisotropy ) = 0;

        virtual void _setSRGB( int sampler, bool en ) = 0;

        // misc
        virtual void toDeviceViewMatrix( Matrix4& matView ) = 0;
        virtual void toDeviceProjMatrix( Matrix4& matProj ) = 0;
        virtual bool isDepthAcceptSize( int depthWidth, int depthHeight, int rttWidth, int rttHeight ) = 0;

        const Vector2& getHalfTexelOffset() const;
        void makeMatrixProjToTex( Matrix4& mat, float zoff, int width, int height, bool needOffsetTexel );
        void makeMatrixViewportToProj( Matrix3& mat, int width, int height, bool needOffsetTexel );

        virtual void readSurfaceToCPU( SurfaceObj* surSrc, SurfaceObj* surDestOffscreen ) = 0;

    public:
        // À©Õ¹
        void setRenderTarget( int idx, SurfaceObj* surTarget );
        void setDepthStencil( SurfaceObj* surDepthStencil );

        void setEffect( Effect* effect );

        void setDepthStateSet( DepthStateSet state );
        void setStencilStateSet( StencilStateSet state );
        void enableStencil( bool en );
        bool isStencilEnabled() const { return m_currStencilEnable == (int)true; }

        void setMaterialStateSet( MaterialStateSet state );
        
        void enableBlendState( bool en );
        void setBlendStateSet( BlendStateSet state );
        bool isBlendStateEnabled() const { return m_currBlendEnable == (int)true; }
        
        void setSolidState( bool solid );

        void setSamplerState( int sampler, const SamplerState& state );
        void setTexture( int sampler, TextureObj* texObj );
        void enableSRGB( bool en );

        void restoreMainRenderTarget( int idx );
        void restoreMainDepthStencil();
        void restoreMainViewport();
        void restoreMainRTT();

    public:
        int getWindowWidth()  const { return m_windowWidth; }
        int getWindowHeight() const { return m_windowHeight; }

        SurfaceObj* getMainRenderTarget() const { return m_mainRenderTarget; }
        TextureObj* getMainDepthStencil() const { return m_mainDepthStencil; }

    protected:
        void _resetState();

    protected:
        int         m_windowWidth;
        int         m_windowHeight;
        SurfaceObj* m_mainRenderTarget;
        TextureObj* m_mainDepthStencil;

    private:
        SurfaceObj* m_currRenderTarget[MAX_MULTI_RTT_NUMS];
        SurfaceObj* m_currDepthStencil;

        Effect*             m_currEffect;
        
        DepthStateSet       m_currDepthState;
        StencilStateSet     m_currStencilState;
        int                 m_currStencilEnable;

        MaterialStateSet    m_currMaterialState;
        
        BlendStateSet       m_currBlendState;
        int                 m_currBlendEnable;

        SamplerState        m_currSamplerState[MAX_SAMPLER_NUMS];
        TextureObj*         m_currTextureObj[MAX_SAMPLER_NUMS];

        bool                m_enabledSRGB;
    };

    //////////////////////////////////////////////////////////////////////////
    extern RenderDevice* g_renderDevice;

    RenderDevice* createRenderDevice();
}

