#include "KhaosPreHeaders.h"
#include "KhaosSurfaceObj.h"
#include "KhaosMath.h"
#include "KhaosColor.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    const void* PixelFillOp::_fill_argb8( uint8* dest, int col, const void* src )
    {
        uint32* destReal = (uint32*)(dest);
        const uint32* srcReal = (const uint32*)(src);

        destReal[col] = *srcReal;
        ++srcReal;

        return srcReal;
    }

    const void* PixelFillOp::_fillconv_argb8( uint8* dest, int col, const void* src, bool srgbWrite )
    {
        uint32* destReal = (uint32*)(dest);
        const Color* srcReal = (const Color*)(src);

        destReal[col] = srgbWrite ? srcReal->toGammaColor().getAsARGB() : srcReal->getAsARGB();
        ++srcReal;

        return srcReal;
    }

    const void* PixelFillOp::_fill_abgr16f( uint8* dest, int col, const void* src )
    {
        uint16* destReal = (uint16*)(dest);
        const float* srcReal = (const float*)(src);

        int off = col * 4; // 4个float16每像素

        for ( int k = 0; k < 4; ++k )
        {
            destReal[off] = Math::toFloat16(*srcReal);
            ++off;
            ++srcReal;
        }

        return srcReal;
    }

    const void* PixelFillOp::_fillconv_abgr16f( uint8* dest, int col, const void* src, bool srgbWrite )
    {
        uint16* destReal = (uint16*)(dest);
        const Color* srcReal = (const Color*)(src);

        int off = col * 4; // 4个float16每像素

        destReal[off] = Math::toFloat16(srcReal->r); ++off;
        destReal[off] = Math::toFloat16(srcReal->g); ++off;
        destReal[off] = Math::toFloat16(srcReal->b); ++off;
        destReal[off] = Math::toFloat16(srcReal->a);

        ++srcReal;
        return srcReal;
    }

    const void* PixelFillOp::_fill_abgr32f( uint8* dest, int col, const void* src )
    {
        float* destReal = (float*)(dest);
        const float* srcReal = (const float*)(src);

        int off = col * 4; // 4个float32每像素

        for ( int k = 0; k < 4; ++k )
        {
            destReal[off] = (*srcReal);
            ++off;
            ++srcReal;
        }

        return srcReal;
    }

    const void* PixelFillOp::_fillconv_abgr32f( uint8* dest, int col, const void* src, bool srgbWrite )
    {
        float* destReal = (float*)(dest);
        const Color* srcReal = (const Color*)(src);

        int off = col * 4; // 4个float32每像素

        destReal[off] = srcReal->r; ++off;
        destReal[off] = srcReal->g; ++off;
        destReal[off] = srcReal->b; ++off;
        destReal[off] = srcReal->a;

        ++srcReal;
        return srcReal;
    }

    const void* PixelFillOp::_fill_8b( uint8* dest, int col, const void* src )
    {
        const uint8* srcReal = (const uint8*)(src);

        dest[col] = (*srcReal);
        ++srcReal;

        return srcReal;
    }

    const void* PixelFillOp::_fillconv_8b_a( uint8* dest, int col, const void* src, bool srgbWrite )
    {
        const Color* srcReal = (const Color*)(src);

        dest[col] = Color::f2b(srcReal->a);
        ++srcReal;

        return srcReal;
    }

    const void* PixelFillOp::_fillconv_8b_l( uint8* dest, int col, const void* src, bool srgbWrite )
    {
        const Color* srcReal = (const Color*)(src);

        dest[col] = Color::f2b(srgbWrite ? srcReal->toGammaColor().r : srcReal->r); // 可能用Lumin更好，这里简单些，假定rgb都一样
        ++srcReal;

        return srcReal;
    }

    const void* PixelFillOp::_fill_16f( uint8* dest, int col, const void* src )
    {
        uint16* destReal = (uint16*)(dest);
        const float* srcReal = (const float*)(src);

        destReal[col] = Math::toFloat16(*srcReal);
        ++srcReal;

        return srcReal;
    }

    const void* PixelFillOp::_fillconv_16f( uint8* dest, int col, const void* src, bool srgbWrite )
    {
        uint16* destReal = (uint16*)(dest);
        const Color* srcReal = (const Color*)(src);

        destReal[col] = Math::toFloat16(srcReal->r); // 总是R
        ++srcReal;

        return srcReal;
    }

    const void* PixelFillOp::_fill_32f( uint8* dest, int col, const void* src )
    {
        float* destReal = (float*)(dest);
        const float* srcReal = (const float*)(src);

        destReal[col] = *srcReal;
        ++srcReal;

        return srcReal;
    }

    const void* PixelFillOp::_fillconv_32f( uint8* dest, int col, const void* src, bool srgbWrite )
    {
        float* destReal = (float*)(dest);
        const Color* srcReal = (const Color*)(src);

        destReal[col] = srcReal->r; // 总是R
        ++srcReal;

        return srcReal;
    }

    const void* PixelFillOp::_fill_g16r16f( uint8* dest, int col, const void* src )
    {
        uint16* destReal = (uint16*)(dest);
        const float* srcReal = (const float*)(src);

        int off = col * 2; // 2个float16每像素

        for ( int k = 0; k < 2; ++k )
        {
            destReal[off] = Math::toFloat16(*srcReal);
            ++off;
            ++srcReal;
        }

        return srcReal;
    }

    const void* PixelFillOp::_fillconv_g16r16f( uint8* dest, int col, const void* src, bool srgbWrite )
    {
        uint16* destReal = (uint16*)(dest);
        const Color* srcReal = (const Color*)(src);

        int off = col * 2; // 2个float16每像素

        destReal[off] = Math::toFloat16(srcReal->r); ++off;
        destReal[off] = Math::toFloat16(srcReal->g);

        ++srcReal;

        return srcReal;
    }

    void PixelFillOp::_fillDataLoop( const LockedRect& lockInfo, const IntRect* rect, 
        const void* data, int dataPitch, FillFunc func )
    {
        uint8* lineSrc = (uint8*)data;

        for ( int row = rect->top; row < rect->bottom; ++row )
        {
            if ( dataPitch > 0 ) // 有data pitch则处理行距，没有则是顺序读
                lineSrc  = (uint8*)data + dataPitch * row; // 数据源的行起始

            uint8* lineDest = (uint8*)lockInfo.bits + lockInfo.pitch * row; // 目标的行起始

            for ( int col = rect->left; col < rect->right; ++col )
            {
                lineSrc = (uint8*) func( lineDest, col, lineSrc );
            }
        }
    }

    void PixelFillOp::_fillConvDataLoop( const LockedRect& lockInfo, const IntRect* rect, 
        const void* data, int dataPitch, FillConvFunc func, bool srgbWrite )
    {
        uint8* lineSrc = (uint8*)data;

        for ( int row = rect->top; row < rect->bottom; ++row )
        {
            if ( dataPitch > 0 ) // 有data pitch则处理行距，没有则是顺序读
                lineSrc  = (uint8*)data + dataPitch * row; // 数据源的行起始

            uint8* lineDest = (uint8*)lockInfo.bits + lockInfo.pitch * row; // 目标的行起始

            for ( int col = rect->left; col < rect->right; ++col )
            {
                lineSrc = (uint8*) func( lineDest, col, lineSrc, srgbWrite );
            }
        }
    }

    void PixelFillOp::fillData( PixelFormat pf, const LockedRect& lockInfo, const IntRect* rect, const void* data, int dataPitch )
    {
        if ( pf == PIXFMT_A8R8G8B8 ) // argb8
        {
            _fillDataLoop( lockInfo, rect, data, dataPitch, _fill_argb8 );
        }
        else if ( pf == PIXFMT_A16B16G16R16F ) // abgr16f
        { 
            _fillDataLoop( lockInfo, rect, data, dataPitch, _fill_abgr16f );
        }
        else if ( pf == PIXFMT_A32B32G32R32F ) // abgr32f
        { 
            _fillDataLoop( lockInfo, rect, data, dataPitch, _fill_abgr32f );
        }
        else if ( pf == PIXFMT_A8 || pf == PIXFMT_L8 ) // a8, l8
        {
            _fillDataLoop( lockInfo, rect, data, dataPitch, _fill_8b );
        }
        else if ( pf == PIXFMT_16F ) // r16f
        {
            _fillDataLoop( lockInfo, rect, data, dataPitch, _fill_16f );
        }
        else if ( pf == PIXFMT_32F ) // r32f
        {
            _fillDataLoop( lockInfo, rect, data, dataPitch, _fill_32f );
        }
        else if ( pf == PIXFMT_G16R16F ) // g16r16f
        {
            _fillDataLoop( lockInfo, rect, data, dataPitch, _fill_g16r16f );
        }
        else
        {
            khaosAssert(0);
        }
    }

    void PixelFillOp::fillConvertData( PixelFormat pf, const LockedRect& lockInfo, const IntRect* rect, const float* rgba, int dataPitch, bool srgbWrite )
    {
        if ( pf == PIXFMT_A8R8G8B8 ) // argb8
        {
            _fillConvDataLoop( lockInfo, rect, rgba, dataPitch, _fillconv_argb8, srgbWrite );
        }
        else if ( pf == PIXFMT_A16B16G16R16F ) // abgr16f
        { 
            _fillConvDataLoop( lockInfo, rect, rgba, dataPitch, _fillconv_abgr16f, srgbWrite );
        }
        else if ( pf == PIXFMT_A32B32G32R32F ) // abgr32f
        { 
            _fillConvDataLoop( lockInfo, rect, rgba, dataPitch, _fillconv_abgr32f, srgbWrite );
        }
        else if ( pf == PIXFMT_A8 ) // a8, l8
        {
            _fillConvDataLoop( lockInfo, rect, rgba, dataPitch, _fillconv_8b_a, srgbWrite );
        }
        else if ( pf == PIXFMT_L8 ) // a8, l8
        {
            _fillConvDataLoop( lockInfo, rect, rgba, dataPitch, _fillconv_8b_l, srgbWrite );
        }
        else if ( pf == PIXFMT_16F ) // r16f
        {
            _fillConvDataLoop( lockInfo, rect, rgba, dataPitch, _fillconv_16f, srgbWrite );
        }
        else if ( pf == PIXFMT_32F ) // r32f
        {
            _fillConvDataLoop( lockInfo, rect, rgba, dataPitch, _fillconv_32f, srgbWrite );
        }
        else if ( pf == PIXFMT_G16R16F ) // g16r16f
        {
            _fillConvDataLoop( lockInfo, rect, rgba, dataPitch, _fillconv_g16r16f, srgbWrite );
        }
        else
        {
            khaosAssert(0);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void SurfaceObj::fillData( const IntRect* rect, const void* data, int dataPitch )
    {
        LockedRect lockInfo;

        const IntRect rectAll( 0, 0, m_width, m_height );
        if ( !rect )
            rect = &rectAll;

        if ( !lock( TEXACC_WRITE, &lockInfo, rect ) )
            return;

        PixelFillOp::fillData( this->m_format, lockInfo, rect, data, dataPitch );

        unlock();
    }

    void SurfaceObj::fillConvertData( const IntRect* rect, const float* rgba, int dataPitch, bool srgbWrite )
    {
        LockedRect lockInfo;

        const IntRect rectAll( 0, 0, m_width, m_height );
        if ( !rect )
            rect = &rectAll;

        if ( !lock( TEXACC_WRITE, &lockInfo, rect ) )
            return;

        PixelFillOp::fillConvertData( this->m_format, lockInfo, rect, rgba, dataPitch, srgbWrite );

        unlock();
    }
}

