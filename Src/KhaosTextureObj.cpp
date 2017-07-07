#include "KhaosPreHeaders.h"
#include "KhaosTextureObj.h"
#include "KhaosRenderDevice.h"

namespace Khaos
{
    void SamplerState::_init()
    {
        m_filter        = TextureFilterSet::TRILINEAR;
        m_address       = TextureAddressSet::WRAP;
        m_borderClr     = Color::ZERO;
        m_mipLodBias    = 0;
        m_mipMaxLevel   = 0;
        m_maxAnisotropy = 1;
        m_isSRGB        = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    void TextureObj::_createSurfaceObjArray()
    {
        khaosAssert( !m_surface );
        int count = _getSurfaceCountPerLevel() * m_levels;
        m_surface = KHAOS_NEW_T(SurfaceObjArray)(count);
    }

    void TextureObj::_destroySurfaceObjArray()
    {
        if ( m_surface )
        {
            for ( size_t i = 0; i < m_surface->size(); ++i )
            {
                SurfaceObj* sur = (*m_surface)[i];
                KHAOS_DELETE sur;
            }

            KHAOS_DELETE_T(m_surface);
            m_surface = 0;
        }
    }

    int TextureObj::_getSurfaceCountPerLevel() const
    {
        switch ( m_type )
        {
        case TEXTYPE_2D:
        case TEXTYPE_VOLUME:
            return 1;
        case TEXTYPE_CUBE:
            return 6;
        }

        khaosAssert(0);
        return 1;
    }

    SurfaceObj*& TextureObj::_getSurface( int i, int level ) 
    {
        int pos = _getSurfaceCountPerLevel() * level + i; 
        return (*m_surface)[pos];
    }

    SurfaceObj* TextureObj::getSurface( int level ) const
    {
        return const_cast<TextureObj*>(this)->_getSurface( 0, level );
    }

    SurfaceObj* TextureObj::getSurface( CubeMapFace face, int level ) const 
    {
        return const_cast<TextureObj*>(this)->_getSurface( face, level );
    }

    void TextureObj::fillData( int level, const IntRect* rect, const void* data, int dataPitch )
    {
        const IntRect rectAll( 0, 0, getLevelWidth(level), getLevelHeight(level) );
        if ( !rect )
            rect = &rectAll;

        LockedRect lockInfo;

        if ( !lock( level, TEXACC_WRITE, &lockInfo, rect ) )
            return;

        PixelFillOp::fillData( m_format, lockInfo, rect, data, dataPitch );

        unlock( level );
    }

    void TextureObj::fillConvertData( int level, const IntRect* rect, const float* rgba, int dataPitch, bool sRGBWrite )
    {
        const IntRect rectAll( 0, 0, getLevelWidth(level), getLevelHeight(level) );
        if ( !rect )
            rect = &rectAll;

        LockedRect lockInfo;

        if ( !lock( level, TEXACC_WRITE, &lockInfo, rect ) )
            return;

        PixelFillOp::fillConvertData( m_format, lockInfo, rect, rgba, dataPitch, sRGBWrite );

        unlock( level );
    }

    void TextureObj::fillCubeData( CubeMapFace face, int level, const IntRect* rect, const void* data, int dataPitch )
    {
        const IntRect rectAll( 0, 0, getLevelWidth(level), getLevelHeight(level) );
        if ( !rect )
            rect = &rectAll;

        LockedRect lockInfo;

        if ( !lockCube( face, level, TEXACC_WRITE, &lockInfo, rect ) )
            return;

        PixelFillOp::fillData( m_format, lockInfo, rect, data, dataPitch );

        unlockCube( face, level );
    }

    void TextureObj::fillVolumeData( int level, int depth, const IntRect* rect, const void* data, int dataPitch )
    {
        const IntRect rectAll( 0, 0, getLevelWidth(level), getLevelHeight(level) );
        if ( !rect )
            rect = &rectAll;

        LockedBox lockInfoVol;

        const IntBox box( rect->left, rect->top, rect->right, rect->bottom, depth, depth+1 );

        if ( !lockVolume( level, TEXACC_WRITE, &lockInfoVol, &box ) )
            return;

        LockedRect lockInfo2D;
        lockInfo2D.bits  = lockInfoVol.bits;
        lockInfo2D.pitch = lockInfoVol.rowPitch;
        
        PixelFillOp::fillData( m_format, lockInfo2D, rect, data, dataPitch );

        unlockVolume( level );
    }

    //////////////////////////////////////////////////////////////////////////
    TextureObjUnit::~TextureObjUnit()
    {
        freeTextureObj();
        KHAOS_FREE( m_cpuData );
    }

    void TextureObjUnit::bindTextureObj( TextureObj* texObj )
    {
        khaosAssert( !m_managed );
        m_texObj = texObj;
    }

    TextureObj* TextureObjUnit::createTextureObj()
    {
        if ( m_texObj )
            return m_texObj;

        m_texObj = g_renderDevice->createTextureObj();
        m_managed = true;
        return m_texObj;
    }

    void TextureObjUnit::freeTextureObj()
    {
        if ( m_managed && m_texObj )
            KHAOS_DELETE m_texObj;

        m_managed = false;
        m_texObj  = 0;
    }

    void TextureObjUnit::_fetchReadData()
    {
        if ( m_cpuData )
            return;

        // 将数据拷贝到一个离屏页面
        int width = m_texObj->getWidth();
        int height = m_texObj->getHeight();

        m_texObj->fetchSurface();
        SurfaceObj* surSrc = m_texObj->getSurface(0);

        SurfaceObj* surTemp = g_renderDevice->createSurfaceObj();
        surTemp->createOffscreenPlain( width, height, PIXFMT_A32B32G32R32F, 0 );

        g_renderDevice->readSurfaceToCPU( surSrc, surTemp );

        // 读取数据
        bool isSRGB = this->isSRGB();
        const float gamma = 2.2f;

        m_cpuData = KHAOS_MALLOC_ARRAY_T( float, width * height * 4 );

        LockedRect lockRect;
        surTemp->lock( TEXACC_READ, &lockRect, 0 );
        
        for ( int y = 0; y < height; ++y )
        {
            float* lineStart = (float*)((uint8*)lockRect.bits + lockRect.pitch * y);
            float* destStart = m_cpuData + 4 * width * y;

            for ( int x = 0; x < width; ++x )
            {
                const float* clrSrc  = lineStart + x * 4;
                float*       clrDest = destStart + x * 4;

                if ( isSRGB ) // srgb, gamma = 2.2
                {
                    clrDest[0] = Math::pow( clrSrc[0], gamma );
                    clrDest[1] = Math::pow( clrSrc[1], gamma );
                    clrDest[2] = Math::pow( clrSrc[2], gamma );
                }
                else // already linear space, copy directly
                {
                    clrDest[0] = clrSrc[0];
                    clrDest[1] = clrSrc[1];
                    clrDest[2] = clrSrc[2];
                }

                clrDest[3] = clrSrc[3];
            }
        }

        // 完成
        surTemp->unlock();
        KHAOS_DELETE surTemp;
    }

    float TextureObjUnit::_adjustUV( float uv, int addr ) const
    {
        switch ( addr )
        {
        case TEXADDR_WRAP:
            uv = Math::modF( uv, 1.0f ); 
            if ( uv < 0.0f )
                uv += 1.0f;
            break;

        case TEXADDR_CLAMP:
            //uv = Math::clamp( uv, 0.0f, 1.0f );
            break;

        default:
            khaosAssert(0);
            break;
        }

        return uv;
    }

    Color TextureObjUnit::_readTex2D( float u, float v )
    {
        u = _adjustUV( u, getAddress().addrU );
        v = _adjustUV( v, getAddress().addrV );

        // 简单点，快速采样
        int width = getWidth();
        int height = getHeight();

        int iu = Math::clamp( int(u*width), 0, width-1 );
        int iv = Math::clamp( int(v*height), 0, height-1 );

        khaosAssert( 0 <= iu && iu < width );
        khaosAssert( 0 <= iv && iv < height );

        float* clr = &m_cpuData[(iv * width + iu) * 4];
        return *(Color*)clr;
    }

    const Color& TextureObjUnit::_readTex2DPix( int x, int y ) const
    {
        khaosAssert( 0 <= x && x < getWidth() );
        khaosAssert( 0 <= y && y < getHeight() );

        float* clr = &m_cpuData[(y * getWidth() + x) * 4];
        return *(Color*)clr;
    }
}

