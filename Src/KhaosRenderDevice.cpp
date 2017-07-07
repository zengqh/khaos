#include "KhaosPreHeaders.h"
#include "KhaosRenderDevice.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    VertexDeclaration* _createVertexDeclaration( uint32 id )
    {
        // ’‚∏ˆapiΩˆŒ™±‹√‚“¿¿µπÿœµ
        VertexDeclaration* vd = g_renderDevice->createVertexDeclaration();
        vd->_setID(id);
        return vd;
    }

    //////////////////////////////////////////////////////////////////////////
    RenderDevice* g_renderDevice = 0;

    RenderDevice::RenderDevice() : 
        m_windowWidth(0), m_windowHeight(0), m_mainRenderTarget(0), m_mainDepthStencil(0),
        m_enabledSRGB(false)
    {
        khaosAssert( !g_renderDevice );
        g_renderDevice = this;
        _resetState();
    }

    RenderDevice::~RenderDevice()
    {
        g_renderDevice = 0;
    }

    void RenderDevice::setEffect( Effect* effect )
    {
        if ( m_currEffect == effect )
            return;

        _setEffect( effect );
        m_currEffect = effect;
    }

    void RenderDevice::setDepthStateSet( DepthStateSet state )
    {
        if ( m_currDepthState == state )
            return;

        if ( m_currDepthState.zwrite != state.zwrite )
            _enableZWrite( state.zwrite != 0 );

        if ( m_currDepthState.ztest != state.ztest )
            _enableZTest( state.ztest != 0 );

        if ( m_currDepthState.cmpfunc != state.cmpfunc )
            _setZFunc( (CmpFunc)state.cmpfunc );

        m_currDepthState = state;
    }

    void RenderDevice::setStencilStateSet( StencilStateSet state )
    {
        if ( m_currStencilEnable != (int)true )
            return;

        khaosAssert( state._allInvalid == 0 );

        if ( m_currStencilState == state )
            return;

        bool allInvalid = m_currStencilState._allInvalid != 0;

#define _DO_STENCIL_TEST(name) \
        if ( allInvalid || (m_currStencilState.##name != state.##name) ) \
            _setStencil_##name(state.##name);

#define _DO_STENCIL_TEST_CONV(t, name) \
        if ( allInvalid || (m_currStencilState.##name != state.##name) ) \
            _setStencil_##name((t)state.##name);

       _DO_STENCIL_TEST(refVal);
       _DO_STENCIL_TEST_CONV(CmpFunc, cmpFunc);
       _DO_STENCIL_TEST(cmpMask);
       _DO_STENCIL_TEST(writeMask);

       _DO_STENCIL_TEST_CONV(StencilOp, stencilFailOp);
       _DO_STENCIL_TEST_CONV(StencilOp, zFailOp);
       _DO_STENCIL_TEST_CONV(StencilOp, bothPassOp);

       m_currStencilState = state;
    }

    void RenderDevice::enableStencil( bool en )
    {
        if ( m_currStencilEnable == (int)en )
            return;

        _enableStencil( en );
        m_currStencilEnable = en;
    }

    void RenderDevice::setMaterialStateSet( MaterialStateSet state )
    {
        if ( m_currMaterialState == state )
            return;

        if ( m_currMaterialState.cullMode != state.cullMode )
            _setCullMode( (CullMode)state.cullMode );

        if ( m_currMaterialState.wireframe != state.wireframe )
            _setWireframe( state.wireframe != 0 );

        m_currMaterialState = state;
    }

    void RenderDevice::enableBlendState( bool en )
    {
        if ( m_currBlendEnable == (int)en )
            return;

        _enableBlend( en );
        m_currBlendEnable = en;
    }

    void RenderDevice::setBlendStateSet( BlendStateSet state )
    {
        if ( m_currBlendEnable != (int)true )
            return;

        if ( m_currBlendState == state )
            return;

        if ( m_currBlendState.op != state.op )
             _setBlendOp( (BlendOp)state.op );

        if ( m_currBlendState.srcVal != state.srcVal )
            _setSrcBlend( (BlendVal)state.srcVal );

        if ( m_currBlendState.destVal != state.destVal )
            _setDestBlend( (BlendVal)state.destVal );
        
        m_currBlendState = state;
    }

    void RenderDevice::setSolidState( bool solid )
    {
        setDepthStateSet( solid ? DepthStateSet::SOLID_DRAW : DepthStateSet::TRANS_DRAW );
        enableBlendState( !solid );
    }

    void RenderDevice::setSamplerState( int sampler, const SamplerState& state )
    {
        khaosAssert( 0 <= sampler && sampler < MAX_SAMPLER_NUMS );

        SamplerState& currSampleState = m_currSamplerState[sampler];
        
        // test filter
        TextureFilterSet currFilter = currSampleState.getFilter();
        TextureFilterSet filter = state.getFilter();

        if ( currFilter != filter )
        {
            if ( currFilter.tfMag != filter.tfMag )
                _setMagFilter( sampler, (TextureFilter)filter.tfMag );

            if ( currFilter.tfMin != filter.tfMin )
                _setMinFilter( sampler, (TextureFilter)filter.tfMin );

            if ( currFilter.tfMip != filter.tfMip )
                _setMipFilter( sampler, (TextureFilter)filter.tfMip );

            currSampleState.setFilter( filter );
        }

        // test address
        TextureAddressSet currAddr = currSampleState.getAddress();
        TextureAddressSet addr = state.getAddress();

        if ( currAddr != addr )
        {
            if ( currAddr.addrU != addr.addrU )
                _setTexAddrU( sampler, (TextureAddress)addr.addrU );

            if ( currAddr.addrV != addr.addrV )
                _setTexAddrV( sampler, (TextureAddress)addr.addrV );

            if ( currAddr.addrW != addr.addrW )
                _setTexAddrW( sampler, (TextureAddress)addr.addrW );

            currSampleState.setAddress( addr );
        }

        // borderClr
        if ( addr.addrU == TEXADDR_BORDER || addr.addrV == TEXADDR_BORDER || addr.addrW == TEXADDR_BORDER )
        {
            _setBorderColor( sampler, state.getBorderColor() );
        }
        
        // mip
        if ( filter.tfMip != TEXF_NONE )
        {
            // mipLodBias
            float fMipLodBias = state.getMipLodBias();
            if ( currSampleState.getMipLodBias() != fMipLodBias )
            {
                _setMipLodBias( sampler, fMipLodBias );
                currSampleState.setMipLodBias( fMipLodBias );
            }

            // mipMaxLevel
            int mipMaxLevel = state.getMipMaxLevel();
            if ( currSampleState.getMipMaxLevel() != mipMaxLevel )
            {
                _setMipMaxLevel( sampler, mipMaxLevel );
                currSampleState.setMipMaxLevel( mipMaxLevel );
            }
        }

        // maxAnisotropy
        if ( filter.tfMin == TEXF_ANISOTROPIC || filter.tfMag == TEXF_ANISOTROPIC )
        {
            int ma = state.getMaxAnisotropy();
            if ( currSampleState.getMaxAnisotropy() != ma )
            {
                _setMaxAnisotropy( sampler, ma );
                currSampleState.setMaxAnisotropy( ma );
            }
        }

        // srgb
        uint8 currSRGB = currSampleState._getSRGB();
        uint8 srgb = m_enabledSRGB ? state._getSRGB() : 0;

        if ( currSRGB != srgb )
        {
            bool srgb_en = srgb != 0;
            _setSRGB( sampler, srgb_en );
            currSampleState.setSRGB( srgb_en );
        }
    }

    void RenderDevice::enableSRGB( bool en )
    {
        if ( m_enabledSRGB == en )
            return;

        m_enabledSRGB = en;

        for ( int i = 0; i < MAX_SAMPLER_NUMS; ++i )
        {
            SamplerState& samplerState = m_currSamplerState[i];
            samplerState._invalidSRGB();
        }
    }

    void RenderDevice::setTexture( int sampler, TextureObj* texObj )
    {
         khaosAssert( 0 <= sampler && sampler < MAX_SAMPLER_NUMS );

         TextureObj*& currTexObj = m_currTextureObj[sampler];

         if ( currTexObj != texObj )
         {
             _setTexture( sampler, texObj );
             currTexObj = texObj;
         }
    }

    void RenderDevice::setRenderTarget( int idx, SurfaceObj* surTarget )
    {
        khaosAssert( 0 <= idx && idx < KHAOS_ARRAY_SIZE(m_currRenderTarget) );
        if ( m_currRenderTarget[idx] != surTarget )
        {
            m_currRenderTarget[idx] = surTarget;
            _setRenderTarget( idx, surTarget );
        }
    }

    void RenderDevice::setDepthStencil( SurfaceObj* surDepthStencil )
    {
        if ( m_currDepthStencil != surDepthStencil )
        {
            m_currDepthStencil = surDepthStencil;
            _setDepthStencil( surDepthStencil );
        }
    }

    void RenderDevice::restoreMainRenderTarget( int idx )
    {
        if ( idx == 0 )
            setRenderTarget( 0, m_mainRenderTarget );
        else
            setRenderTarget( idx, 0 );
    }

    void RenderDevice::restoreMainDepthStencil()
    {
        setDepthStencil( m_mainDepthStencil->getSurface(0) );
    }

    void RenderDevice::restoreMainViewport()
    {
        setViewport( IntRect(0, 0, m_mainRenderTarget->getWidth(), m_mainRenderTarget->getHeight()) );
    }

    void RenderDevice::restoreMainRTT()
    {
        restoreMainRenderTarget(0);
        restoreMainDepthStencil();
        restoreMainViewport();
    }

    void RenderDevice::_resetState()
    {
        SurfaceObj* UNKNOWN_SURFACE = (SurfaceObj*)(-1);

        const uint32 UNKNOWN_STATE = -1;
        const int    UNKNOWN_BOOL  = 3;
        const float  UNKNOWN_BIAS  = -999999.0f;
        const int    UNKNOWN_LEVEL = -999;
        const int    UNKNOWN_ANIS  = -999;
        
        static const TextureFilterSet  UNKNOWN_TEXFILTER(UNKNOWN_STATE);
        static const TextureAddressSet UNKNOWN_TEXADDR(UNKNOWN_STATE);

        // ÷ÿ÷√◊¥Ã¨
        for ( int i = 0; i < KHAOS_ARRAY_SIZE(m_currRenderTarget); ++i )
            m_currRenderTarget[i] = UNKNOWN_SURFACE;

        m_currDepthStencil = UNKNOWN_SURFACE;

        m_currEffect = 0;

        // render state
        m_currDepthState.data    = UNKNOWN_STATE;

        m_currStencilState._allInvalid = -1;
        m_currStencilEnable = UNKNOWN_BOOL;

        m_currMaterialState.data = UNKNOWN_STATE;
        
        m_currBlendState.data    = UNKNOWN_STATE;
        m_currBlendEnable        = UNKNOWN_BOOL;

        // sample state
        for ( int i = 0; i < MAX_SAMPLER_NUMS; ++i )
        {
            SamplerState& samplerState = m_currSamplerState[i];

            samplerState.setFilter( UNKNOWN_TEXFILTER );
            samplerState.setAddress( UNKNOWN_TEXADDR );
            samplerState.setMipLodBias( UNKNOWN_BIAS );
            samplerState.setMipMaxLevel( UNKNOWN_LEVEL );
            samplerState.setMaxAnisotropy( UNKNOWN_ANIS );
            samplerState._invalidSRGB();

            m_currTextureObj[i] = 0;
        }
    }

    const Vector2& RenderDevice::getHalfTexelOffset() const
    {
        static Vector2 hto(0.5f, 0.5f);
        return hto;
    }

    void RenderDevice::makeMatrixProjToTex( Matrix4& mat, float zoff, int width, int height, bool needOffsetTexel )
    {
        if ( width == 0 || height == 0 )
        {
            width  = m_windowWidth;
            height = m_windowHeight;
        }

        float halfX = 0;
        float halfY = 0;

        if ( needOffsetTexel ) // œÒÀÿ◊Û…œ∆´“∆µ»”⁄Œ∆Àÿ”“œ¬∆´“∆
        {
            halfX = getHalfTexelOffset().x / width;
            halfY = getHalfTexelOffset().y / height;
        }

        mat[0][0] = 0.5f; mat[0][1] = 0;     mat[0][2] = 0;    mat[0][3] = 0.5f + halfX;
        mat[1][0] = 0;    mat[1][1] = -0.5f; mat[1][2] = 0;    mat[1][3] = 0.5f + halfY;
        mat[2][0] = 0;    mat[2][1] = 0;     mat[2][2] = 1.0f; mat[2][3] = zoff;
        mat[3][0] = 0;    mat[3][1] = 0;     mat[3][2] = 0;    mat[3][3] = 1.0f;
    }

    void RenderDevice::makeMatrixViewportToProj( Matrix3& mat, int width, int height, bool needOffsetTexel )
    {
        // proj_x =   vp_x / vp_width  * 2 - 1
        // proj_y = - vp_y / vp_height * 2 + 1
        // proj_x_halfp =   (vp_x - 0.5) / vp_width  * 2 - 1 =   vp_x / vp_width  * 2 - 1 - 1 / vp_width
        // proj_y_halfp = - (vp_y - 0.5) / vp_height * 2 + 1 = - vp_y / vp_height * 2 + 1 + 1 / vp_height

        if ( width == 0 || height == 0 )
        {
            width  = m_windowWidth;
            height = m_windowHeight;
        }

        float sx = 2.0f / width;
        float sy = -2.0f / height;

        float halfX = 0;
        float halfY = 0;

        if ( needOffsetTexel )
        {
            halfX = -getHalfTexelOffset().x * 2 / width;
            halfY =  getHalfTexelOffset().y * 2 / height;
        }

        mat[0][0] = sx; mat[0][1] = 0;  mat[0][2] = -1.0f + halfX;
        mat[1][0] = 0;  mat[1][1] = sy; mat[1][2] =  1.0f + halfY;
        mat[2][0] = 0;  mat[2][1] = 0;  mat[2][2] =  1.0f;
    }
}

