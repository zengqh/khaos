#pragma once

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    enum RenderClearFlag
    {
        RCF_NULL    = 0x0,
        RCF_TARGET  = 0x1,
        RCF_DEPTH   = 0x2,
        RCF_STENCIL = 0x4,
        RCF_ALL     = 0xff
    };

    enum CmpFunc
    {
        CMP_NEVER,
        CMP_LESS,
        CMP_EQUAL,
        CMP_LESSEQUAL,
        CMP_GREATER,
        CMP_NOTEQUAL,
        CMP_GREATEREQUAL,
        CMP_ALWAYS
    };

    enum StencilOp
    {        
        STENCILOP_KEEP,
        STENCILOP_ZERO,
        STENCILOP_REPLACE,
        STENCILOP_INCRSAT,
        STENCILOP_DECRSAT,
        STENCILOP_INVERT,
        STENCILOP_INCR,
        STENCILOP_DECR    
    };

    enum BlendOp
    {
        BLENDOP_ADD,
        BLENDOP_SUBTRACT,
        BLENDOP_REVSUBTRACT,
        BLENDOP_MIN,
        BLENDOP_MAX
    };

    enum BlendVal
    {
        BLEND_ZERO,
        BLEND_ONE,
        BLEND_SRCCOLOR,
        BLEND_INVSRCCOLOR,
        BLEND_SRCALPHA,
        BLEND_INVSRCALPHA,
        BLEND_DESTALPHA,
        BLEND_INVDESTALPHA,
        BLEND_DESTCOLOR,
        BLEND_INVDESTCOLOR,
        BLEND_SRCALPHASAT,
        BLEND_BOTHSRCALPHA,
        BLEND_BOTHINVSRCALPHA,
        BLEND_BLENDFACTOR,
        BLEND_INVBLENDFACTOR
    };

    enum CullMode
    {
        CULL_NONE,
        CULL_CW,
        CULL_CCW
    };

    enum HardwareBufferUsage
    {
        HBU_STATIC,
        HBU_DYNAMIC
    };

    enum HardwareBufferAccess
    {
        HBA_READ,
        HBA_WRITE,
        HBA_READ_WRITE,
        HBA_WRITE_DISCARD,
        HBA_WRITE_NO_OVERWRITE
    };

    enum VertexElementSemantic
    {
        VES_POSITION,
        VES_NORMAL,
        VES_TEXTURE,
        VES_COLOR,
        VES_TANGENT,
        VES_BONEINDICES,
        VES_BONEWEIGHT,
        VES_MAXCOUNT
    };

    enum VertexElementType
    {
        VET_FLOAT1,
        VET_FLOAT2,
        VET_FLOAT3,
        VET_FLOAT4,
        VET_COLOR,
        VET_UBYTE4,
    };

    enum IndexElementType
    {
        IET_INDEX16,
        IET_INDEX32
    };

    enum PrimitiveType
    {
        PT_TRIANGLELIST,
        PT_LINELIST
    };

    enum TextureUsage
    {
        TEXUSA_STATIC,
        TEXUSA_DYNAMIC,
        TEXUSA_RENDERTARGET,
        TEXUSA_DEPTHSTENCIL
    };

    enum PixelFormat
    {
        PIXFMT_A8R8G8B8,
        PIXFMT_X8R8G8B8,
        PIXFMT_A8,
        PIXFMT_L8,
        PIXFMT_DXT1,
        PIXFMT_DXT5,
        PIXFMT_16F,
        PIXFMT_32F,
        PIXFMT_A16B16G16R16F,
        PIXFMT_A32B32G32R32F,
		PIXFMT_G16R16F,
        PIXFMT_A16B16G16R16,
        PIXFMT_A2R10G10B10,
        PIXFMT_V16U16,
        PIXFMT_A8L8,
        PIXFMT_D24S8,
        PIXFMT_INTZ,
        PIXFMT_INVALID
    };

    enum CubeMapFace
    {
        CUBEMAP_FACE_POSITIVE_X,
        CUBEMAP_FACE_NEGATIVE_X,
        CUBEMAP_FACE_POSITIVE_Y,
        CUBEMAP_FACE_NEGATIVE_Y,
        CUBEMAP_FACE_POSITIVE_Z,
        CUBEMAP_FACE_NEGATIVE_Z
    };

    enum TextureType
    {
        TEXTYPE_2D,
        TEXTYPE_3D,
        TEXTYPE_CUBE,
        TEXTYPE_VOLUME
    };

    enum TextureAccess
    {
        TEXACC_READ,
        TEXACC_WRITE,
        TEXACC_READ_WRITE,
        TEXACC_WRITE_DISCARD
    };

    enum TextureFilter
    {
        TEXF_NONE,          // mip = disable
        TEXF_POINT,         // mag/min = point, mip = nearest mip level
        TEXF_LINEAR,        // mag/min = linear, mip = trilinear filtering, interpolates the two nearest mip levels
        TEXF_ANISOTROPIC,   // mag/min = anisotropic
        TEXF_PYRAMIDALQUAD, // mag/min = 4-sample tent filter
        TEXF_GAUSSIANQUAD   // mag/min = 4-sample Gaussian filter
    };

    enum TextureAddress
    {
        TEXADDR_WRAP,
        TEXADDR_MIRROR,
        TEXADDR_CLAMP,
        TEXADDR_BORDER,
        TEXADDR_MIRRORONCE
    };

    enum 
    {
        MAX_SAMPLER_NUMS   = 16,
        MAX_MULTI_RTT_NUMS = 4
    };

    //////////////////////////////////////////////////////////////////////////
    struct DepthStateSet
    {
        DepthStateSet() : data(0) {}
        DepthStateSet( uint32 v ) : data(v) {}
        DepthStateSet( bool zt, bool zw, CmpFunc func ) : ztest(zt), zwrite(zw), cmpfunc(func) {}

        union
        {
            struct  
            {
                uint32 ztest   : 2;
                uint32 zwrite  : 2;
                uint32 cmpfunc : 4;
            };

            uint32 data;
        };

        bool operator==( const DepthStateSet& rhs ) const
        {
            return this->data == rhs.data;
        }

        bool operator!=( const DepthStateSet& rhs ) const
        {
            return this->data != rhs.data;
        }

    public:
        static DepthStateSet SOLID_DRAW;
        static DepthStateSet TRANS_DRAW;
        static DepthStateSet TEST_GE;
        static DepthStateSet ALL_DISABLED;
    };

    struct MaterialStateSet
    {
        MaterialStateSet() : data(0) {}
        MaterialStateSet( uint32 v ) : data(v) {}
        MaterialStateSet( CullMode cm, bool wf ) : cullMode(cm), wireframe(wf) {}

        union
        {
            struct  
            {
                uint32 cullMode  : 2;
                uint32 wireframe : 2;
            };

            uint32 data;
        };

        bool operator==( const MaterialStateSet& rhs ) const
        {
            return this->data == rhs.data;
        }

        bool operator!=( const MaterialStateSet& rhs ) const
        {
            return this->data != rhs.data;
        }

    public:
        static MaterialStateSet FRONT_DRAW;
        static MaterialStateSet BACK_DRAW;
        static MaterialStateSet TWOSIDE_DRAW;
    };

    struct BlendStateSet
    {
        BlendStateSet() : data(0) {}
        BlendStateSet( uint32 v ) : data(v) {}
        BlendStateSet( BlendOp p, BlendVal sv, BlendVal dv ) : op(p), srcVal(sv), destVal(dv) {}

        union
        {
            struct  
            {
                uint32 op      : 3;
                uint32 srcVal  : 4;
                uint32 destVal : 4;
            };

            uint32 data;
        };

        bool operator==( const BlendStateSet& rhs ) const
        {
            return this->data == rhs.data;
        }

        bool operator!=( const BlendStateSet& rhs ) const
        {
            return this->data != rhs.data;
        }

    public:
        static BlendStateSet REPLACE;
        static BlendStateSet ALPHA;
        static BlendStateSet ADD;
    };

    struct TextureFilterSet
    {
        TextureFilterSet() : data(0) {}
        TextureFilterSet( uint32 v ) : data(v) {}
        TextureFilterSet( TextureFilter tmag, TextureFilter tmin, TextureFilter tmip ) :
            tfMag(tmag), tfMin(tmin), tfMip(tmip) {}

        union
        {
            struct  
            {
                uint32 tfMag : 4;
                uint32 tfMin : 4;
                uint32 tfMip : 4;
            };

            uint32 data;
        };

        bool operator==( const TextureFilterSet& rhs ) const
        {
            return this->data == rhs.data;
        }

        bool operator!=( const TextureFilterSet& rhs ) const
        {
            return this->data != rhs.data;
        }

    public:
        static TextureFilterSet NEAREST;
        static TextureFilterSet BILINEAR;
        static TextureFilterSet TRILINEAR;
        static TextureFilterSet ANISOTROPIC;
    };

    struct TextureAddressSet
    {
        TextureAddressSet() : data(0) {}
        TextureAddressSet( uint32 v ) : data(v) {}
        TextureAddressSet( TextureAddress u, TextureAddress v, TextureAddress w ) :
            addrU(u), addrV(v), addrW(w) {}

        union
        {
            struct
            {
                uint32 addrU : 4;
                uint32 addrV : 4;
                uint32 addrW : 4;
            };

            uint32 data;
        };

        bool operator==( const TextureAddressSet& rhs ) const
        {
            return this->data == rhs.data;
        }

        bool operator!=( const TextureAddressSet& rhs ) const
        {
            return this->data != rhs.data;
        }

    public:
        static TextureAddressSet WRAP;
        static TextureAddressSet CLAMP;
        static TextureAddressSet BORDER;
    };

    struct StencilStateSet
    {
        StencilStateSet() : data(0) {}

        StencilStateSet( uint64 d ) : data(d) {}

        StencilStateSet( 
            uint8 rv, CmpFunc cf, uint8 cm, uint8 wm, StencilOp sfo, StencilOp zfo, StencilOp bpo 
        ) :
            refVal(rv), cmpFunc(cf), cmpMask(cm), writeMask(wm),
            stencilFailOp(sfo), zFailOp(zfo), bothPassOp(bpo),
            _allInvalid(0)
        {
        }

        bool operator==( const StencilStateSet& rhs ) const
        {
            return this->data == rhs.data;
        }

        bool operator!=( const StencilStateSet& rhs ) const
        {
            return this->data != rhs.data;
        }

        union
        {
            struct
            {
                uint8 refVal; // (ref & mask) cmpFunc (value & mask)
                uint8 cmpFunc; // 刚好1byte, see CmpFunc
                uint8 cmpMask;
                uint8 writeMask;

                uint8 stencilFailOp; // 刚好1byte, see StencilOp
                uint8 zFailOp;
                uint8 bothPassOp;

                uint8 _allInvalid; // 非0，则所有数据无效
            };

            uint64 data;
        };
    };
}

