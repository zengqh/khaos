#pragma once
#include "KhaosStdTypes.h"
#include "KhaosScopedPtr.h"
#include "KhaosRenderDeviceDef.h"
#include "KhaosRect.h"

namespace Khaos
{
    class TextureObj;

    struct LockedRect
    {
        LockedRect() : bits(0), pitch(0) {}
        void* bits;
        int   pitch;
    };

    struct LockedBox
    {
        LockedBox() : bits(0), rowPitch(0), slicePitch(0) {}
        void* bits;
        int   rowPitch;
        int   slicePitch;
    };

    class PixelFillOp
    {
    public:
        static void fillData( PixelFormat pf, const LockedRect& lockInfo, const IntRect* rect, const void* data, int dataPitch );
        static void fillConvertData( PixelFormat pf, const LockedRect& lockInfo, const IntRect* rect, const float* rgba, int dataPitch, bool srgbWrite );

    private:
        typedef const void* (*FillFunc)( uint8*, int, const void* );
        typedef const void* (*FillConvFunc)( uint8*, int, const void*, bool );

        static void _fillDataLoop( const LockedRect& lockInfo, const IntRect* rect, const void* data, int dataPitch, FillFunc func );
        static void _fillConvDataLoop( const LockedRect& lockInfo, const IntRect* rect, const void* data, int dataPitch, FillConvFunc func, bool srgbWrite );

        static const void* _fill_argb8( uint8* dest, int col, const void* src );
        static const void* _fill_abgr16f( uint8* dest, int col, const void* src );
        static const void* _fill_abgr32f( uint8* dest, int col, const void* src );
        static const void* _fill_8b( uint8* dest, int col, const void* src );
        static const void* _fill_16f( uint8* dest, int col, const void* src );
        static const void* _fill_32f( uint8* dest, int col, const void* src );
        static const void* _fill_g16r16f( uint8* dest, int col, const void* src );

        static const void* _fillconv_argb8( uint8* dest, int col, const void* src, bool srgbWrite );
        static const void* _fillconv_abgr16f( uint8* dest, int col, const void* src, bool srgbWrite );
        static const void* _fillconv_abgr32f( uint8* dest, int col, const void* src, bool srgbWrite );
        static const void* _fillconv_8b_a( uint8* dest, int col, const void* src, bool srgbWrite );
        static const void* _fillconv_8b_l( uint8* dest, int col, const void* src, bool srgbWrite );
        static const void* _fillconv_16f( uint8* dest, int col, const void* src, bool srgbWrite );
        static const void* _fillconv_32f( uint8* dest, int col, const void* src, bool srgbWrite );
        static const void* _fillconv_g16r16f( uint8* dest, int col, const void* src, bool srgbWrite );
    };

    class SurfaceObj : public AllocatedObject
    {
    public:
        SurfaceObj() : m_owner(0), m_usage(TEXUSA_DEPTHSTENCIL), m_format(PIXFMT_D24S8), m_width(0), m_height(0) {}
        virtual ~SurfaceObj() {}

    public:
       virtual bool create( int width, int height, TextureUsage usage, PixelFormat fmt ) = 0;
       virtual bool createOffscreenPlain( int width, int height, PixelFormat fmt, int usage ) = 0;
       virtual bool getFromTextureObj( TextureObj* texObj, int level ) = 0;
       virtual bool getFromTextureObj( TextureObj* texObj, CubeMapFace face, int level ) = 0;
       virtual void destroy() = 0;

       virtual bool lock( TextureAccess access, LockedRect* lockedRect, const IntRect* rect ) = 0;
       virtual void unlock() = 0;

       void fillData( const IntRect* rect, const void* data, int dataPitch );
       void fillConvertData( const IntRect* rect, const float* rgba, int dataPitch, bool srgbWrite );

    public:
        TextureObj* getOwner() const { return m_owner; }

        TextureUsage  getUsage()  const { return m_usage; }
        PixelFormat   getFormat() const { return m_format; }

        int getWidth()  const { return m_width; }
        int getHeight() const { return m_height; }

    public:
        virtual void save( pcstr file ) = 0;

    protected:
        TextureObj*   m_owner;
        TextureUsage  m_usage;
        PixelFormat   m_format;
        int           m_width;
        int           m_height;
    };

    typedef ScopedPtr<SurfaceObj, SPFM_DELETE> SurfaceObjScpPtr;
}

