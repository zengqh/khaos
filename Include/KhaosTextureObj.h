#pragma once
#include "KhaosStdTypes.h"
#include "KhaosSurfaceObj.h"
#include "KhaosColor.h"
#include "KhaosRenderDeviceDef.h"
#include "KhaosVector2.h"

namespace Khaos
{
    class SamplerState : public AllocatedObject
    {
    public:
        SamplerState() { _init(); }

    public:
        void setFilter( TextureFilterSet flt ) { m_filter = flt; }
        void setFilterMag( TextureFilter flt ) { m_filter.tfMag = flt; }
        void setFilterMin( TextureFilter flt ) { m_filter.tfMin = flt; }
        void setFilterMip( TextureFilter flt ) { m_filter.tfMip = flt; }
        TextureFilterSet getFilter() const { return m_filter; }

        void setAddress( TextureAddressSet addr ) { m_address = addr; }
        void setAddressU( TextureAddress addr ) { m_address.addrU = addr; }
        void setAddressV( TextureAddress addr ) { m_address.addrV = addr; }
        void setAddressW( TextureAddress addr ) { m_address.addrW = addr; }
        TextureAddressSet getAddress() const { return m_address; }

        void setBorderColor( const Color& clr ) { m_borderClr = clr; }
        const Color& getBorderColor() const { return m_borderClr; }

        void  setMipLodBias( float bias ) { m_mipLodBias = bias; }
        float getMipLodBias() const { return m_mipLodBias; }

        void setMipMaxLevel( int level ) { m_mipMaxLevel = level; }
        int  getMipMaxLevel() const { return m_mipMaxLevel; }

        void setMaxAnisotropy( int ani ) { m_maxAnisotropy = ani; }
        int  getMaxAnisotropy() const { return m_maxAnisotropy; }

        void  setSRGB( bool en ) { m_isSRGB = en ? 1 : 0; }
        bool  isSRGB() const { return m_isSRGB != 0; }

        void  _invalidSRGB() { m_isSRGB = -1; }
        uint8 _getSRGB() const { return m_isSRGB; }

    public:
        void resetDefault() { _init(); }

    private:
        void _init();

    private:
        TextureFilterSet    m_filter;
        TextureAddressSet   m_address;
        Color               m_borderClr;
        float               m_mipLodBias;      // default: 0
        int                 m_mipMaxLevel;     // [0, n-1] default: 0
        int                 m_maxAnisotropy;   // [1, Max] default: 1
        uint8               m_isSRGB;
    };

    struct TexObjLoadParas
    {
        enum
        {
            MIP_FROMFILE = -1,
            MIP_AUTO     = 0
        };

        TexObjLoadParas() : type(TEXTYPE_2D), data(0), dataLen(0), 
            mipSize(MIP_AUTO), needMipmap(false) {}

        TextureType type;
        const void* data;
        int         dataLen;
        int         mipSize;
        bool        needMipmap;
    };

    struct TexObjCreateParas
    {
        TexObjCreateParas() : 
            type(TEXTYPE_2D), usage(TEXUSA_STATIC), format(PIXFMT_A8R8G8B8),
            levels(1), width(0), height(0), depth(0) {}

        TextureType   type;
        TextureUsage  usage;
        PixelFormat   format;
        int           levels;
        int           width;
        int           height;
        int           depth;
    };

    class TextureObj : public AllocatedObject
    {
    protected:
        typedef vector<SurfaceObj*>::type SurfaceObjArray;

    public:
        TextureObj::TextureObj() : 
            m_type(TEXTYPE_2D), m_usage(TEXUSA_STATIC), m_format(PIXFMT_A8R8G8B8),
            m_levels(0), m_width(0), m_height(0), m_depth(0),
            m_surface(0)
        {}

        virtual ~TextureObj() {}

    public:
        virtual bool load( const TexObjLoadParas& paras ) = 0;
        virtual bool create( const TexObjCreateParas& paras ) = 0;
        virtual void destroy() = 0;

        virtual bool lock( int level, TextureAccess access, LockedRect* lockedRect, const IntRect* rect ) = 0;
        virtual void unlock( int level ) = 0;

        virtual bool lockCube( CubeMapFace face, int level, TextureAccess access, LockedRect* lockedRect, const IntRect* rect ) = 0;
        virtual void unlockCube( CubeMapFace face, int level ) = 0;

        virtual bool lockVolume( int level, TextureAccess access, LockedBox* lockedVolume, const IntBox* box ) = 0;
        virtual void unlockVolume( int level ) = 0;

        void fillData( int level, const IntRect* rect, const void* data, int dataPitch );
        void fillConvertData( int level, const IntRect* rect, const float* rgba, int dataPitch, bool sRGBWrite );
        void fillCubeData( CubeMapFace face, int level, const IntRect* rect, const void* data, int dataPitch );
        void fillVolumeData( int level, int depth, const IntRect* rect, const void* data, int dataPitch );

        virtual void fetchSurface() = 0;

        virtual void save( pcstr file ) = 0;

        virtual int getLevelWidth( int level ) const = 0;
        virtual int getLevelHeight( int level ) const = 0;
        virtual int getLevelDepth( int level ) const = 0;

    public:
        TextureType   getType()   const { return m_type; }
        TextureUsage  getUsage()  const { return m_usage; }
        PixelFormat   getFormat() const { return m_format; }

        int getLevels() const { return m_levels; }
        int getWidth()  const { return m_width; }
        int getHeight() const { return m_height; }
        int getDepth()  const { return m_depth; }

        SurfaceObj* getSurface( int level ) const;
        SurfaceObj* getSurface( CubeMapFace face, int level ) const;

    protected:
        void _createSurfaceObjArray();
        void _destroySurfaceObjArray();

        SurfaceObj*& _getSurface( int i, int level );
        int _getSurfaceCountPerLevel() const;

    protected:
        TextureType         m_type;
        TextureUsage        m_usage;
        PixelFormat         m_format;
        int                 m_levels;
        int                 m_width;
        int                 m_height;
        int                 m_depth;

    private:
        SurfaceObjArray*    m_surface;
    };

    class TextureObjUnit : public AllocatedObject
    {
    public:
        TextureObjUnit() : m_texObj(0), m_cpuData(0), m_managed(false) {}
        virtual ~TextureObjUnit();

    public:
        // for convenience, must ensure create texture object first
        bool loadTex( const TexObjLoadParas& paras )
        {
            return createTextureObj()->load( paras );
        }

        bool createTex( const TexObjCreateParas& paras )
        {
            return createTextureObj()->create( paras );
        }

        void destroyTex()
        {
            m_texObj->destroy();
        }

        bool lock( int level, TextureAccess access, LockedRect* lockedRect, const IntRect* rect )
        {
            return m_texObj->lock( level, access, lockedRect, rect );
        }

        void unlock( int level )
        {
            return m_texObj->unlock( level );
        }

        bool lockCube( CubeMapFace face, int level, TextureAccess access, LockedRect* lockedRect, const IntRect* rect )
        {
            return m_texObj->lockCube( face, level, access, lockedRect, rect );
        }

        void unlockCube( CubeMapFace face, int level ) 
        {
            return m_texObj->unlockCube( face, level );
        }

        bool lockVolume( int level, TextureAccess access, LockedBox* lockedVolume, const IntBox* box )
        {
            return m_texObj->lockVolume( level, access, lockedVolume, box );
        }

        void unlockVolume( int level )
        {
            return m_texObj->unlockVolume( level );
        }

        void fillData( int level, const IntRect* rect, const void* data, int dataPitch )
        {
            m_texObj->fillData( level, rect, data, dataPitch );
        }

        void fillCubeData( CubeMapFace face, int level, const IntRect* rect, const void* data, int dataPitch )
        {
            m_texObj->fillCubeData( face, level, rect, data, dataPitch );
        }

        void fillVolumeData( int level, int depth, const IntRect* rect, const void* data, int dataPitch )
        {
            m_texObj->fillVolumeData( level, depth, rect, data, dataPitch );
        }

        TextureType   getType()   const { return m_texObj->getType(); }
        TextureUsage  getUsage()  const { return m_texObj->getUsage(); }
        PixelFormat   getFormat() const { return m_texObj->getFormat(); }

        int getLevels() const { return m_texObj->getLevels(); }
        int getWidth()  const { return m_texObj->getWidth(); }
        int getHeight() const { return m_texObj->getHeight(); }
        int getDepth()  const { return m_texObj->getDepth(); }

        int getLevelWidth( int level ) const { return m_texObj->getLevelWidth(level); }
        int getLevelHeight( int level ) const { return m_texObj->getLevelHeight(level); }

        SurfaceObj* getSurface( int level ) const { return m_texObj->getSurface(level); }
        SurfaceObj* getSurface( CubeMapFace face, int level ) const { return m_texObj->getSurface(face, level); }

    public:
        // for convenience using sampler state
        void setFilter( TextureFilterSet flt ) { m_samplerState.setFilter( flt ); }
        void setFilterMag( TextureFilter flt ) { m_samplerState.setFilterMag( flt ); }
        void setFilterMin( TextureFilter flt ) { m_samplerState.setFilterMin( flt ); }
        void setFilterMip( TextureFilter flt ) { m_samplerState.setFilterMip( flt ); }
        TextureFilterSet getFilter() const { return m_samplerState.getFilter(); }

        void setAddress( TextureAddressSet addr ) { m_samplerState.setAddress(addr); }
        void setAddressU( TextureAddress addr )   { m_samplerState.setAddressU(addr); }
        void setAddressV( TextureAddress addr )   { m_samplerState.setAddressV(addr); }
        void setAddressW( TextureAddress addr )   { m_samplerState.setAddressW(addr); }
        TextureAddressSet getAddress() const { return m_samplerState.getAddress(); }

        void setBorderColor( const Color& clr ) { m_samplerState.setBorderColor(clr); }
        const Color& getBorderColor() const { return m_samplerState.getBorderColor(); }

        void  setMipLodBias( float bias ) { m_samplerState.setMipLodBias(bias); }
        float getMipLodBias() const { return m_samplerState.getMipLodBias(); }

        void setMipMaxLevel( int level ) { m_samplerState.setMipMaxLevel(level); }
        int  getMipMaxLevel() const { return m_samplerState.getMipMaxLevel(); }

        void setMaxAnisotropy( int ani ) { m_samplerState.setMaxAnisotropy(ani); }
        int  getMaxAnisotropy() const { return m_samplerState.getMaxAnisotropy(); }

        void setSRGB( bool en ) { m_samplerState.setSRGB(en); }
        bool isSRGB() const { return m_samplerState.isSRGB(); }

        void resetDefaultSamplerState() { m_samplerState.resetDefault(); }

    public:
        void        bindTextureObj( TextureObj* texObj );
        TextureObj* createTextureObj();
        void        freeTextureObj();
        TextureObj* getTextureObj() const { return m_texObj; }

        const SamplerState& getSamplerState() const { return m_samplerState; }
        SamplerState& getSamplerState() { return m_samplerState; }

    public:
        void  _fetchReadData();
        const Color& _readTex2DPix( int x, int y ) const;
        Color _readTex2D( float u, float v );
        Color _readTex2D( const Vector2& uv ) { return _readTex2D(uv.x, uv.y); }

    protected:
        float _adjustUV( float uv, int addr ) const;

    protected:
        TextureObj*     m_texObj;
        SamplerState    m_samplerState;
        float*          m_cpuData;
        bool            m_managed;
    };
}

