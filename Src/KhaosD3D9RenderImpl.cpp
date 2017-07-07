#include "KhaosPreHeaders.h"
#include "KhaosD3D9RenderImpl.h"
#include "KhaosFileSystem.h"


// 是否调试shader，调试能打印shader代码到磁盘
#if KHAOS_DEBUG
    #define TEST_DEBUG_SHADER_ 1
#else
    #define TEST_DEBUG_SHADER_ 0
#endif

// shader创建顺序
#define SHADER_SOURCE_ONLY  1
#define SHADER_BIN_ONLY     2
#define SHADER_BIN_FIRST    3

#if KHAOS_DEBUG
    #define SHADER_CREATE_TYPE  SHADER_SOURCE_ONLY //  因为debug所以总源码加载
#else
    #define SHADER_CREATE_TYPE  SHADER_BIN_FIRST // 发行版本bin优先随后源码加载
#endif


#if KHAOS_RENDER_SYSTEM == KHAOS_RENDER_SYSTEM_D3D9

namespace Khaos
{

#define g_d3dRenderDevice   static_cast<D3D9RenderDevice*>(g_renderDevice)
#define g_d3dDevice         static_cast<D3D9RenderDevice*>(g_renderDevice)->_getD3DDevice()

#define D3DFMT_INTZ_  (D3DFORMAT)(MAKEFOURCC('I','N','T','Z'))

    //////////////////////////////////////////////////////////////////////////
    uint16 _toFloat16_Impl( float v )
    {
        D3DXFLOAT16 v16f;
        D3DXFloat32To16Array( &v16f, &v, 1 );
        return *(uint16*)(&v16f);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    inline void _safeComRelease( T*& p )
    {
        if ( p )
        {
            p->Release();
            p = 0;
        }
    }

    inline BYTE _toD3DDeclType( VertexElementType type )
    {
        static const BYTE dt[] =
        {
            D3DDECLTYPE_FLOAT1,     // VET_FLOAT1
            D3DDECLTYPE_FLOAT2,     // VET_FLOAT2
            D3DDECLTYPE_FLOAT3,     // VET_FLOAT3
            D3DDECLTYPE_FLOAT4,     // VET_FLOAT4
            D3DDECLTYPE_D3DCOLOR,   // VET_COLOR
            D3DDECLTYPE_UBYTE4      // VET_UBYTE4
        };
       
        khaosAssert( 0 <= type && type < KHAOS_ARRAY_SIZE(dt) );
        return dt[type];
    }

    inline BYTE _toD3DDeclUseage( VertexElementSemantic semantic )
    {
        static const BYTE du[] =
        {
            D3DDECLUSAGE_POSITION,      // VES_POSITION
            D3DDECLUSAGE_NORMAL,        // VES_NORMAL
            D3DDECLUSAGE_TEXCOORD,      // VES_TEXTURE
            D3DDECLUSAGE_COLOR,         // VES_COLOR
            D3DDECLUSAGE_TANGENT,       // VES_TANGENT
            D3DDECLUSAGE_BLENDINDICES,  // VES_BONEINDICES
            D3DDECLUSAGE_BLENDWEIGHT    // VES_BONEWEIGHT
        };

        khaosAssert( 0 <= semantic && semantic < KHAOS_ARRAY_SIZE(du) );
        return du[semantic];
    }

    inline DWORD _toD3DHardwareBufferUsage( HardwareBufferUsage usage )
    {
        static const DWORD hbu[] =
        {
            0,                                      // HBU_STATIC
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY   // HBU_DYNAMIC
        };

        khaosAssert( 0 <= usage && usage < KHAOS_ARRAY_SIZE(hbu) );
        return hbu[usage];
    }

    inline D3DPOOL _toD3DPoolByHardwareBufferUsage( HardwareBufferUsage usage )
    {
        static const D3DPOOL pool[] =
        {
            D3DPOOL_MANAGED,    // HBU_STATIC
            D3DPOOL_DEFAULT     // HBU_DYNAMIC
        };

        khaosAssert( 0 <= usage && usage < KHAOS_ARRAY_SIZE(pool) );
        return pool[usage];
    }

    inline DWORD _toD3DLockFlag( HardwareBufferAccess access )
    {
        static const DWORD flag[] =
        {
            D3DLOCK_READONLY,   // HBA_READ (static)
            0,                  // HBA_WRITE (static/dynamic)
            0,                  // HBA_READ_WRITE (static)
            D3DLOCK_DISCARD,    // HBA_WRITE_DISCARD (dynamic)
            D3DLOCK_NOOVERWRITE // HBA_WRITE_NO_OVERWRITE (dynamic)
        };

        khaosAssert( 0 <= access && access < KHAOS_ARRAY_SIZE(flag) );
        return flag[access];
    }

    inline DWORD _toD3DLockFlag( TextureAccess access )
    {
        static const DWORD flag[] =
        {
            D3DLOCK_READONLY,   // TEXACC_READ (static)
            0,                  // TEXACC_WRITE (static/dynamic)
            0,                  // TEXACC_READ_WRITE (static)
            D3DLOCK_DISCARD,    // TEXACC_WRITE_DISCARD (dynamic)
        };

        khaosAssert( 0 <= access && access < KHAOS_ARRAY_SIZE(flag) );
        return flag[access];
    }

    inline DWORD _toD3DTexUsage( TextureUsage usage )
    {
        static const DWORD du[] =
        {
            0,                      // TEXUSA_STATIC
            D3DUSAGE_DYNAMIC,       // TEXUSA_DYNAMIC
            D3DUSAGE_RENDERTARGET,  // TEXUSA_RENDERTARGET
            D3DUSAGE_DEPTHSTENCIL   // TEXUSA_DEPTHSTENCIL
        };

        khaosAssert( 0 <= usage && usage < KHAOS_ARRAY_SIZE(du) );
        return du[usage];
    }

    inline D3DFORMAT _toD3DTexFormat( PixelFormat fmt )
    {
        static const D3DFORMAT df[] =
        {
            D3DFMT_A8R8G8B8,        // PIXFMT_A8R8G8B8
            D3DFMT_X8R8G8B8,        // PIXFMT_X8R8G8B8
            D3DFMT_A8,              // PIXFMT_A8
            D3DFMT_L8,              // PIXFMT_L8
            D3DFMT_DXT1,            // PIXFMT_DXT1
            D3DFMT_DXT5,            // PIXFMT_DXT5
            D3DFMT_R16F,            // PIXFMT_16F
            D3DFMT_R32F,            // PIXFMT_32F
            D3DFMT_A16B16G16R16F,   // PIXFMT_A16B16G16R16F
            D3DFMT_A32B32G32R32F,   // PIXFMT_A32B32G32R32F
			D3DFMT_G16R16F,			// PIXFMT_G16R16F
            D3DFMT_A16B16G16R16,    // PIXFMT_A16B16G16R16
            D3DFMT_A2R10G10B10,     // PIXFMT_A2R10G10B10
            D3DFMT_V16U16,          // PIXFMT_V16U16
            D3DFMT_A8L8,            // PIXFMT_A8L8
            D3DFMT_D24S8,           // PIXFMT_D24S8
            D3DFMT_INTZ_,           // PIXFMT_INTZ
        };
        
        khaosAssert( 0 <= fmt && fmt < KHAOS_ARRAY_SIZE(df) );
        return df[fmt];
    }

    inline PixelFormat _fromD3DTexFormat( D3DFORMAT df )
    {
        switch ( df )
        {
        case D3DFMT_A8R8G8B8:
            return PIXFMT_A8R8G8B8;
        
        case D3DFMT_X8R8G8B8:
            return PIXFMT_X8R8G8B8;

        case D3DFMT_A8:
            return PIXFMT_A8;

        case D3DFMT_L8:
            return PIXFMT_L8;

        case D3DFMT_DXT1:
            return PIXFMT_DXT1;

        case D3DFMT_DXT5:
            return PIXFMT_DXT5;

        case D3DFMT_R16F:
            return PIXFMT_16F;

        case D3DFMT_R32F:
            return PIXFMT_32F;

        case D3DFMT_A16B16G16R16F:
            return PIXFMT_A16B16G16R16F;

        case D3DFMT_A32B32G32R32F:
            return PIXFMT_A32B32G32R32F;

		case D3DFMT_G16R16F:
			return PIXFMT_G16R16F;

        case D3DFMT_A16B16G16R16:
            return PIXFMT_A16B16G16R16;

        case D3DFMT_A2R10G10B10:
            return PIXFMT_A2R10G10B10;

        case D3DFMT_V16U16:
            return PIXFMT_V16U16;

        case D3DFMT_A8L8:
            return PIXFMT_A8L8;

        case D3DFMT_D24S8:
            return PIXFMT_D24S8;

        case D3DFMT_INTZ_:
            return PIXFMT_INTZ;

        default:
            khaosAssert(0);
            return PIXFMT_A8R8G8B8;
        }
    }

    inline D3DCUBEMAP_FACES _toD3DCubeMapFace( CubeMapFace face )
    {
        // dx z反转
        static const D3DCUBEMAP_FACES dfc[] =
        {
            D3DCUBEMAP_FACE_POSITIVE_X, // CUBEMAP_FACE_POSITIVE_X
            D3DCUBEMAP_FACE_NEGATIVE_X, // CUBEMAP_FACE_NEGATIVE_X
            D3DCUBEMAP_FACE_POSITIVE_Y, // CUBEMAP_FACE_POSITIVE_Y
            D3DCUBEMAP_FACE_NEGATIVE_Y, // CUBEMAP_FACE_NEGATIVE_Y
            D3DCUBEMAP_FACE_POSITIVE_Z, // CUBEMAP_FACE_POSITIVE_Z
            D3DCUBEMAP_FACE_NEGATIVE_Z  // CUBEMAP_FACE_NEGATIVE_Z
        };

        khaosAssert( 0 <= face && face < KHAOS_ARRAY_SIZE(dfc) );
        return dfc[face];
    }

    inline D3DTEXTUREFILTERTYPE _toD3DTexFilter( TextureFilter flt )
    {
        static const D3DTEXTUREFILTERTYPE dfl[] =
        {
            D3DTEXF_NONE,           // TEXF_NONE
            D3DTEXF_POINT,          // TEXF_POINT
            D3DTEXF_LINEAR,         // TEXF_LINEAR
            D3DTEXF_ANISOTROPIC,    // TEXF_ANISOTROPIC
            D3DTEXF_PYRAMIDALQUAD,  // TEXF_PYRAMIDALQUAD
            D3DTEXF_GAUSSIANQUAD    // TEXF_GAUSSIANQUAD
        };

        khaosAssert( 0 <= flt && flt < KHAOS_ARRAY_SIZE(dfl) );
        return dfl[flt];
    }

    inline D3DTEXTUREADDRESS _toD3DTexAddr( TextureAddress addr )
    {
        static const D3DTEXTUREADDRESS dta[] =
        {
            D3DTADDRESS_WRAP,       // TEXADDR_WRAP
            D3DTADDRESS_MIRROR,     // TEXADDR_MIRROR
            D3DTADDRESS_CLAMP,      // TEXADDR_CLAMP
            D3DTADDRESS_BORDER,     // TEXADDR_BORDER
            D3DTADDRESS_MIRRORONCE  // TEXADDR_MIRRORONCE
        };

        khaosAssert( 0 <= addr && addr < KHAOS_ARRAY_SIZE(dta) );
        return dta[addr];
    }

    inline D3DPOOL _toD3DPoolByTexUsage( TextureUsage usage )
    {
        static const D3DPOOL pool[] =
        {
            D3DPOOL_MANAGED,    // TEXUSA_STATIC
            D3DPOOL_DEFAULT,    // TEXUSA_DYNAMIC
            D3DPOOL_DEFAULT,    // TEXUSA_RENDERTARGET
            D3DPOOL_DEFAULT     // TEXUSA_DEPTHSTENCIL
        };

        khaosAssert( 0 <= usage && usage < KHAOS_ARRAY_SIZE(pool) );
        return pool[usage];
    }

    inline D3DFORMAT _toD3DIndexFormat( IndexElementType indexElementType )
    {
        static const D3DFORMAT fmt[] =
        {
            D3DFMT_INDEX16, // IET_INDEX16
            D3DFMT_INDEX32  // IET_INDEX32
        };

        khaosAssert( 0 <= indexElementType && indexElementType < KHAOS_ARRAY_SIZE(fmt) );
        return fmt[indexElementType];
    }

    inline D3DPRIMITIVETYPE _toD3DPrimType( PrimitiveType primType )
    {
        static const D3DPRIMITIVETYPE pt[] =
        {
            D3DPT_TRIANGLELIST, // PT_TRIANGLELIST
            D3DPT_LINELIST,     // PT_LINELIST
        };

        khaosAssert( 0 <= primType && primType < KHAOS_ARRAY_SIZE(pt) );
        return pt[primType];
    }

    inline D3DSTENCILOP _toD3DStencilOp( StencilOp op )
    {
        static const D3DSTENCILOP v[] =
        {
            D3DSTENCILOP_KEEP,
            D3DSTENCILOP_ZERO,
            D3DSTENCILOP_REPLACE,
            D3DSTENCILOP_INCRSAT,
            D3DSTENCILOP_DECRSAT,
            D3DSTENCILOP_INVERT,
            D3DSTENCILOP_INCR,
            D3DSTENCILOP_DECR
        };

        khaosAssert( 0 <= op && op < KHAOS_ARRAY_SIZE(v) );
        return v[op];
    }

    inline D3DCMPFUNC _toD3DCmpFunc( CmpFunc func )
    {
        static const D3DCMPFUNC v[] =
        {
            D3DCMP_NEVER,
            D3DCMP_LESS,
            D3DCMP_EQUAL,
            D3DCMP_LESSEQUAL,
            D3DCMP_GREATER,
            D3DCMP_NOTEQUAL,
            D3DCMP_GREATEREQUAL,
            D3DCMP_ALWAYS
        };

        khaosAssert( 0 <= func && func < KHAOS_ARRAY_SIZE(v) );
        return v[func];
    }

    inline D3DBLENDOP _toD3DBlendOp( BlendOp op )
    {
        static const D3DBLENDOP v[] =
        {
            D3DBLENDOP_ADD,
            D3DBLENDOP_SUBTRACT,
            D3DBLENDOP_REVSUBTRACT,
            D3DBLENDOP_MIN,
            D3DBLENDOP_MAX
        };

        khaosAssert( 0 <= op && op < KHAOS_ARRAY_SIZE(v) );
        return v[op];
    }

    inline D3DBLEND _toD3DBlendVal( BlendVal val )
    {
        static const D3DBLEND v[] =
        {
            D3DBLEND_ZERO,
            D3DBLEND_ONE,
            D3DBLEND_SRCCOLOR,
            D3DBLEND_INVSRCCOLOR,
            D3DBLEND_SRCALPHA,
            D3DBLEND_INVSRCALPHA,
            D3DBLEND_DESTALPHA,
            D3DBLEND_INVDESTALPHA,
            D3DBLEND_DESTCOLOR,
            D3DBLEND_INVDESTCOLOR,
            D3DBLEND_SRCALPHASAT,
            D3DBLEND_BOTHSRCALPHA,
            D3DBLEND_BOTHINVSRCALPHA,
            D3DBLEND_BLENDFACTOR,
            D3DBLEND_INVBLENDFACTOR
        };

        khaosAssert( 0 <= val && val < KHAOS_ARRAY_SIZE(v) );
        return v[val];
    }

    inline D3DCULL _toD3DCull( CullMode cm )
    {
        static const D3DCULL v[] =
        {
            D3DCULL_NONE,
            D3DCULL_CW,
            D3DCULL_CCW
        };

        khaosAssert( 0 <= cm && cm < KHAOS_ARRAY_SIZE(v) );
        return v[cm];
    }

    //////////////////////////////////////////////////////////////////////////
    D3D9VertexDeclaration::D3D9VertexDeclaration() : m_decl(0)
    {
    }

    D3D9VertexDeclaration::~D3D9VertexDeclaration()
    {
        _safeComRelease( m_decl );
    }

    LPDIRECT3DVERTEXDECLARATION9 D3D9VertexDeclaration::getVD()
    {
        if ( m_decl )
            return m_decl;

        static D3DVERTEXELEMENT9 ves[MAXD3DDECLLENGTH + 1] = {0};
        static const D3DVERTEXELEMENT9 vesEnd = D3DDECL_END();

        // 翻译一把
        D3DVERTEXELEMENT9* dest = ves;

        for ( ElementList::iterator it = m_elements.begin(), ite = m_elements.end(); it != ite; ++it )
        {
            VertexElement& src = *it;

            dest->Stream        = 0;
            dest->Offset        = src.offset;
            dest->Type          = _toD3DDeclType(src.type);
            dest->Method        = D3DDECLMETHOD_DEFAULT;
            dest->Usage         = _toD3DDeclUseage(src.semantic);
            dest->UsageIndex    = src.index;

            ++dest;
        }

        *dest = vesEnd;

        // 创建
        if ( FAILED( g_d3dDevice->CreateVertexDeclaration( ves, &m_decl ) ) )
        {
            khaosLogLn( KHL_L1, "CreateVertexDeclaration failed" );
        }

        return m_decl;
    }

    //////////////////////////////////////////////////////////////////////////
    D3D9VertexBuffer::D3D9VertexBuffer() : m_vb(0)
    {
    }

    D3D9VertexBuffer::~D3D9VertexBuffer()
    {
        destroy();

        if ( m_usage == HBU_DYNAMIC )
            g_d3dRenderDevice->_unregisterDeviceObject( this );
    }

    void D3D9VertexBuffer::_destroyImpl()
    {
        _safeComRelease( m_vb );
    }

    bool D3D9VertexBuffer::create( int size, HardwareBufferUsage usage )
    {
        khaosAssert(!m_vb);

        if ( FAILED( g_d3dDevice->CreateVertexBuffer( size, _toD3DHardwareBufferUsage(usage), 0,
            _toD3DPoolByHardwareBufferUsage(usage), &m_vb, 0 ) ) )
        {
            khaosLogLn( KHL_L1, "CreateVertexBuffer failed" );
            return false;
        }

        m_size = size;
        m_usage = usage;

        if ( usage == HBU_DYNAMIC )
            g_d3dRenderDevice->_registerDeviceObject( this );

        return true;
    }

    void* D3D9VertexBuffer::lock( HardwareBufferAccess access, int offset, int size )
    {
        void* ptr = 0;
        m_vb->Lock( offset, size, &ptr, _toD3DLockFlag(access) );
        return ptr;
    }

    void  D3D9VertexBuffer::unlock()
    {
        m_vb->Unlock();
    }

    void D3D9VertexBuffer::onLostDevice()
    {
        khaosAssert( m_usage == HBU_DYNAMIC );
        m_vb->Release();
    }

    void D3D9VertexBuffer::onResetDevice()
    {
        khaosAssert( m_usage == HBU_DYNAMIC );
        create( m_size, m_usage );
    }

    //////////////////////////////////////////////////////////////////////////
    D3D9IndexBuffer::D3D9IndexBuffer() : m_ib(0)
    {
    }

    D3D9IndexBuffer::~D3D9IndexBuffer()
    {        
        destroy();

        if ( m_usage == HBU_DYNAMIC )
            g_d3dRenderDevice->_unregisterDeviceObject( this );
    }

    void D3D9IndexBuffer::_destroyImpl()
    {
        _safeComRelease( m_ib );
    }

    bool D3D9IndexBuffer::create( int size, HardwareBufferUsage usage, IndexElementType type )
    {
        khaosAssert(!m_ib);

        if ( FAILED( g_d3dDevice->CreateIndexBuffer( size, _toD3DHardwareBufferUsage(usage), 
            _toD3DIndexFormat(type), _toD3DPoolByHardwareBufferUsage(usage), &m_ib, 0 ) ) )
        {
            khaosLogLn( KHL_L1, "CreateIndexBuffer failed" );
            return false;
        }

        m_size = size;
        m_usage = usage;
        m_indexType = type;

        if ( usage == HBU_DYNAMIC )
            g_d3dRenderDevice->_registerDeviceObject( this );

        return true;
    }

    void* D3D9IndexBuffer::lock( HardwareBufferAccess access, int offset, int size )
    {
        void* ptr = 0;
        m_ib->Lock( offset, size, &ptr, _toD3DLockFlag(access) );
        return ptr;
    }
    
    void D3D9IndexBuffer::unlock()
    {
        m_ib->Unlock();
    }

    void D3D9IndexBuffer::onLostDevice()
    {
        khaosAssert( m_usage == HBU_DYNAMIC );
        m_ib->Release();
    }

    void D3D9IndexBuffer::onResetDevice()
    {
        khaosAssert( m_usage == HBU_DYNAMIC );
        create( m_size, m_usage, m_indexType );
    }

    //////////////////////////////////////////////////////////////////////////
    D3D9SurfaceObj::~D3D9SurfaceObj()
    {
        destroy();
    }

    bool D3D9SurfaceObj::create( int width, int height, TextureUsage usage, PixelFormat fmt )
    {
        khaosAssert( !m_sur );

        if ( usage == TEXUSA_DEPTHSTENCIL )
        {
            D3DFORMAT curDSFmt = D3DFMT_D24S8;
            fmt = PIXFMT_D24S8; // 深度总用这个格式

            HRESULT hr = g_d3dDevice->CreateDepthStencilSurface( width, height, curDSFmt,
                D3DMULTISAMPLE_NONE, 0, TRUE, &m_sur, 0 ); 

            if ( FAILED(hr) )
            {
                khaosLogLn( KHL_L1, "D3D9SurfaceObj::create TEXUSA_DEPTHSTENCIL" );
                return false;
            }
        }
        else
        {
            khaosAssert(0);
            return false;
        }

        m_owner  = 0;
        m_usage  = usage;
        m_format = fmt;
        m_width  = width;
        m_height = height;

        g_d3dRenderDevice->_registerDeviceObject( this ); // 自主创建需要注册被托管
        return true;
    }

    bool D3D9SurfaceObj::createOffscreenPlain( int width, int height, PixelFormat fmt, int usage ) 
    {
        khaosAssert( !m_sur );

        D3DFORMAT    d3dfmt   = _toD3DTexFormat( fmt );
        D3DPOOL      d3dpool  = D3DPOOL_DEFAULT;
        TextureUsage texusage = TEXUSA_DYNAMIC;
        
        switch ( usage )
        {
        case 0: // only cpu used
            d3dpool = D3DPOOL_SCRATCH;
            texusage = TEXUSA_STATIC; // 暂且static
            break;
        case 1: // need submit to gpu
            d3dpool = D3DPOOL_SYSTEMMEM;
            texusage = TEXUSA_STATIC;
            break;
        }

        HRESULT hr = g_d3dDevice->CreateOffscreenPlainSurface( width, height, d3dfmt, d3dpool, &m_sur, 0 );

        if ( FAILED(hr) )
        {
            khaosLogLn( KHL_L1, "D3D9SurfaceObj::createOffscreenPlain" );
            return false;
        }

        m_owner  = 0;
        m_usage  = texusage;
        m_format = fmt;
        m_width  = width;
        m_height = height;

        if ( m_usage == TEXUSA_DYNAMIC )
            g_d3dRenderDevice->_registerDeviceObject( this ); // 自主创建需要注册被托管
        return true;
    }

    bool D3D9SurfaceObj::getFromTextureObj( TextureObj* texObj, int level )
    {
        D3D9TextureObj* d3dTex = static_cast<D3D9TextureObj*>(texObj);
        LPDIRECT3DSURFACE9 sur = 0;
        d3dTex->getTex2D()->GetSurfaceLevel( level, &sur );
        _bindHandle( sur, d3dTex->getUsage() );
        m_owner = texObj;
        // 不托管，交给TextureObj
        return true;
    }

    bool D3D9SurfaceObj::getFromTextureObj( TextureObj* texObj, CubeMapFace face, int level )
    {
        D3D9TextureObj* d3dTex = static_cast<D3D9TextureObj*>(texObj);
        LPDIRECT3DSURFACE9 sur = 0;
        d3dTex->getTexCube()->GetCubeMapSurface( _toD3DCubeMapFace(face), level, &sur );
        _bindHandle( sur, d3dTex->getUsage() );
        m_owner = texObj;
        // 不托管，交给TextureObj
        return true;
    }

    void D3D9SurfaceObj::_bindHandle( LPDIRECT3DSURFACE9 sur, TextureUsage usage )
    {
        // 内部用，绑定rtt和ds
        khaosAssert( !m_sur );
        m_sur = sur;

        m_owner  = 0;
        m_usage  = usage;
        
        D3DSURFACE_DESC desc = {};
        sur->GetDesc( &desc );
        
        m_format = _fromD3DTexFormat( desc.Format );
        m_width  = desc.Width;
        m_height = desc.Height;
    }

    void D3D9SurfaceObj::_unbindHandle()
    {
        if ( m_sur )
        {
            m_sur->Release();
            m_sur = 0;
        }
    }

    bool D3D9SurfaceObj::lock( TextureAccess access, LockedRect* lockedRect, const IntRect* rect )
    {
        D3DLOCKED_RECT d3dLockRect = {};
        DWORD          flag = _toD3DLockFlag(access);

        if ( SUCCEEDED( m_sur->LockRect( &d3dLockRect, (const RECT*)rect, flag ) ) )
        {
            lockedRect->bits  = d3dLockRect.pBits;
            lockedRect->pitch = d3dLockRect.Pitch;
            return true;
        }

        return false;
    }

    void D3D9SurfaceObj::unlock()
    {
        m_sur->UnlockRect();
    }

    void D3D9SurfaceObj::destroy()
    {
        if ( m_sur )
        {
            m_sur->Release();
            m_sur = 0;

            if ( !m_owner ) // 自主创建需要注销
                g_d3dRenderDevice->_unregisterDeviceObject( this );
        }
    }

    void D3D9SurfaceObj::save( pcstr file )
    {
        D3DXSaveSurfaceToFileA( file, D3DXIFF_DDS, m_sur, 0, 0 );
    }

    void D3D9SurfaceObj::onLostDevice()
    {
        _unbindHandle();
    }

    void D3D9SurfaceObj::onResetDevice()
    {
        khaosAssert( !m_owner );
        create( m_width, m_height, m_usage, m_format );
    }

    //////////////////////////////////////////////////////////////////////////
    UINT _toD3DMipLevels( const TexObjLoadParas& paras )
    {
        UINT mipLevels;

        if ( paras.needMipmap )
        {
            if ( paras.mipSize == TexObjLoadParas::MIP_FROMFILE )
                mipLevels = D3DX_FROM_FILE;
            else if ( paras.mipSize == TexObjLoadParas::MIP_AUTO )
                mipLevels = D3DX_DEFAULT;
            else
                mipLevels = paras.mipSize;
        }
        else
        {
            mipLevels = 1;
        }

        return mipLevels;
    }

    D3D9TextureObj::D3D9TextureObj() : m_tex(0)
    {
    }

    D3D9TextureObj::~D3D9TextureObj()
    {
        destroy();
    }

    bool D3D9TextureObj::_loadTex2D( const TexObjLoadParas& paras )
    {
        //if ( FAILED( D3DXCreateTextureFromFileInMemory( g_d3dDevice, paras.data, paras.dataLen, &m_tex ) ) )

        UINT mipLevels = _toD3DMipLevels(paras);

        HRESULT hr = D3DXCreateTextureFromFileInMemoryEx( g_d3dDevice, 
                paras.data, paras.dataLen, // image data
                D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, // width, height
                mipLevels, //(paras.needMipmap ? D3DX_DEFAULT : 1), // mipmap  
                0, // usage
                D3DFMT_UNKNOWN, // format
                D3DPOOL_MANAGED, // pool
                D3DX_FILTER_POINT, // filter
                D3DX_DEFAULT, // mip filter
                0, // color key 
                0, // image info
                0, // palette
                &m_tex
            );

        if ( FAILED(hr) )
        {
            khaosLogLn( KHL_L1, "D3D9TextureObj::load TEXTYPE_2D failed" );
            return false;
        }

        LPDIRECT3DSURFACE9 pSur = 0;
        D3DSURFACE_DESC desc;

        m_tex->GetSurfaceLevel(0, &pSur );
        pSur->GetDesc( &desc );
        pSur->Release();

        m_format = _fromD3DTexFormat( desc.Format );
        m_levels = m_tex->GetLevelCount();
        m_width  = desc.Width;
        m_height = desc.Height;
        m_depth  = 0;

        return true;
    }

    bool D3D9TextureObj::_loadTexCube( const TexObjLoadParas& paras )
    {
        //if ( FAILED( D3DXCreateCubeTextureFromFileInMemory( g_d3dDevice, paras.data, paras.dataLen, &m_texCube ) ) )

        UINT mipLevels = _toD3DMipLevels(paras);

        HRESULT hr = D3DXCreateCubeTextureFromFileInMemoryEx(
            g_d3dDevice,
            paras.data, paras.dataLen, // image data
            D3DX_DEFAULT, // Size
            mipLevels,
            0, // Usage,
            D3DFMT_UNKNOWN, // Format
            D3DPOOL_MANAGED, // Pool
            D3DX_FILTER_POINT, // Filter
            D3DX_DEFAULT, // MipFilter
            0, // ColorKey
            0, // IMAGE_INFO
            0, // Palette
            &m_texCube
        );

        if ( FAILED(hr) )
        {
            khaosLogLn( KHL_L1, "D3D9TextureObj::load TEXTYPE_CUBE" );
            return false;
        }

        LPDIRECT3DSURFACE9 pSur = 0;
        D3DSURFACE_DESC desc;

        m_texCube->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_X, 0, &pSur );
        pSur->GetDesc( &desc );
        pSur->Release();

        m_format = _fromD3DTexFormat( desc.Format );
        m_levels = m_texCube->GetLevelCount();
        m_width  = desc.Width;
        m_height = desc.Height;
        m_depth  = 0;

        return true;
    }

    bool D3D9TextureObj::_loadTexVolume( const TexObjLoadParas& paras )
    {
        UINT mipLevels = _toD3DMipLevels(paras);

        HRESULT hr = D3DXCreateVolumeTextureFromFileInMemoryEx( g_d3dDevice, 
            paras.data, paras.dataLen, // image data
            D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, // width, height, depth
            mipLevels, // mipmap  
            0, // usage
            D3DFMT_UNKNOWN, // format
            D3DPOOL_MANAGED, // pool
            D3DX_FILTER_POINT, // filter
            D3DX_DEFAULT, // mip filter
            0, // color key 
            0, // image info
            0, // palette
            &m_texVolume
            );

        if ( FAILED(hr) )
        {
            khaosLogLn( KHL_L1, "D3D9TextureObj::load TEXTYPE_VOLUME failed" );
            return false;
        }

        D3DVOLUME_DESC desc;
        m_texVolume->GetLevelDesc( 0, &desc );

        m_format = _fromD3DTexFormat( desc.Format );
        m_levels = m_texVolume->GetLevelCount();
        m_width  = desc.Width;
        m_height = desc.Height;
        m_depth  = desc.Depth;

        return true;
    }

    bool D3D9TextureObj::load( const TexObjLoadParas& paras )
    {
        khaosAssert( !m_tex );

        switch ( paras.type )
        {
        case TEXTYPE_2D:
            if ( !_loadTex2D( paras ) )
                return false;
            break;

        case TEXTYPE_CUBE:
            if ( !_loadTexCube( paras ) )
                return false;
            break;

        case TEXTYPE_VOLUME:
            if ( !_loadTexVolume( paras ) )
                return false;
            break;

        default:
            khaosAssert(0);
            return false;
        }
        
        m_type  = paras.type;
        m_usage = TEXUSA_STATIC;
        return true;
    }

    bool D3D9TextureObj::_createTex2D( const TexObjCreateParas& paras )
    {
        HRESULT hr = g_d3dDevice->CreateTexture( paras.width, paras.height, paras.levels, 
            _toD3DTexUsage(paras.usage), _toD3DTexFormat(paras.format), _toD3DPoolByTexUsage(paras.usage), 
            &m_tex, 0 );

        if ( FAILED(hr) )
        {
            khaosLogLn( KHL_L1, "D3D9TextureObj::_createTex2D failed" );
            return false;
        }

        // rtt的话，保存他的表面以备后用
        if ( paras.usage == TEXUSA_RENDERTARGET || paras.usage == TEXUSA_DEPTHSTENCIL )
        {
            fetchSurface();
        }

        return true;
    }

    void D3D9TextureObj::fetchSurface()
    {
        if ( m_type == TEXTYPE_2D )
        {
            _createSurfaceObjArray();

            for ( int lv = 0; lv < getLevels(); ++lv )
            {
                _getSurface(0, lv) = KHAOS_NEW D3D9SurfaceObj;
                _getSurface(0, lv)->getFromTextureObj( this, lv );
            }
        }
        else if ( m_type == TEXTYPE_CUBE )
        {
            _createSurfaceObjArray();

            for ( int lv = 0; lv < getLevels(); ++lv )
            {
                for ( int f = 0; f < 6; ++f )
                {
                    _getSurface(f, lv) = KHAOS_NEW D3D9SurfaceObj;
                    _getSurface(f, lv)->getFromTextureObj( this, (CubeMapFace)f, lv );
                }
            }
        }
        else if ( m_type == TEXTYPE_VOLUME )
        {
            khaosAssert(0); // not support
        }
    }

    bool D3D9TextureObj::_createTexCube( const TexObjCreateParas& paras )
    {
        HRESULT hr = g_d3dDevice->CreateCubeTexture( paras.width, paras.levels, 
            _toD3DTexUsage(paras.usage), _toD3DTexFormat(paras.format), _toD3DPoolByTexUsage(paras.usage), 
            &m_texCube, 0 );

        if ( FAILED(hr) )
        {
            khaosLogLn( KHL_L1, "D3D9TextureObj::_createTexCube failed" );
            return false;
        }

        // rtt的话，保存他的表面以备后用
        if ( paras.usage == TEXUSA_RENDERTARGET )
        {
            fetchSurface();
        }

        return true;
    }

    bool D3D9TextureObj::_createTexVolume( const TexObjCreateParas& paras )
    {
        HRESULT hr = g_d3dDevice->CreateVolumeTexture( paras.width, paras.height, paras.depth, paras.levels, 
            _toD3DTexUsage(paras.usage), _toD3DTexFormat(paras.format), _toD3DPoolByTexUsage(paras.usage), 
            &m_texVolume, 0 );

        if ( FAILED(hr) )
        {
            khaosLogLn( KHL_L1, "D3D9TextureObj::_createTexVolume failed" );
            return false;
        }

        return true;
    }

    bool D3D9TextureObj::create( const TexObjCreateParas& paras )
    {
        khaosAssert( !m_tex );
        
        m_type = paras.type;
        m_usage  = paras.usage;
        m_format = paras.format;
        m_levels = paras.levels;
        m_width  = paras.width;
        m_height = paras.height;
        m_depth  = paras.depth;

        switch ( paras.type )
        {
        case TEXTYPE_2D:
            if ( !_createTex2D( paras ) )
                return false;
            break;

        case TEXTYPE_CUBE:
            if ( !_createTexCube( paras ) )
                return false;
            break;

        case TEXTYPE_VOLUME:
            if ( !_createTexVolume( paras ) )
                return false;
            break;

        default:
            khaosAssert(0);
            return false;
        } 

        if ( m_usage != TEXUSA_STATIC ) // 除了静态被托管，其他都要处理丢失
            g_d3dRenderDevice->_registerDeviceObject( this );

        return true;
    }

    void D3D9TextureObj::destroy()
    {
        _destroySurfaceObjArray();

        if ( IDirect3DBaseTexture9* tex = getBaseTex() )
        {
            tex->Release();
            m_tex = 0;

            if ( m_usage != TEXUSA_STATIC )
                g_d3dRenderDevice->_unregisterDeviceObject( this );
        }
    }

    bool D3D9TextureObj::lock( int level, TextureAccess access, LockedRect* lockedRect, const IntRect* rect )
    {
        D3DLOCKED_RECT d3dLockRect = {};
        DWORD          flag = _toD3DLockFlag(access);

        if ( SUCCEEDED( m_tex->LockRect( level, &d3dLockRect, (const RECT*)rect, flag ) ) )
        {
            lockedRect->bits  = d3dLockRect.pBits;
            lockedRect->pitch = d3dLockRect.Pitch;
            return true;
        }

        return false;
    }

    void D3D9TextureObj::unlock( int level )
    {
        m_tex->UnlockRect( level );
    }

    bool D3D9TextureObj::lockCube( CubeMapFace face, int level, TextureAccess access, LockedRect* lockedRect, const IntRect* rect )
    {
        D3DLOCKED_RECT d3dLockRect = {};
        DWORD          flag = _toD3DLockFlag(access);

        KhaosStaticAssert( sizeof(IntRect) == sizeof(RECT) );

        if ( SUCCEEDED( m_texCube->LockRect( (D3DCUBEMAP_FACES)face, level, &d3dLockRect, (const RECT*)rect, flag ) ) )
        {
            lockedRect->bits  = d3dLockRect.pBits;
            lockedRect->pitch = d3dLockRect.Pitch;
            return true;
        }

        return false;
    }

    void D3D9TextureObj::unlockCube( CubeMapFace face, int level )
    {
        m_texCube->UnlockRect( (D3DCUBEMAP_FACES)face, level );
    }

    bool D3D9TextureObj::lockVolume( int level, TextureAccess access, LockedBox* lockedVolume, const IntBox* box )
    {
        D3DLOCKED_BOX d3dLockBox = {};
        DWORD         flag = _toD3DLockFlag(access);

        KhaosStaticAssert( sizeof(D3DBOX) == sizeof(IntBox) );

        if ( SUCCEEDED( m_texVolume->LockBox( level, &d3dLockBox, (const D3DBOX*)box, flag ) ) )
        {
            lockedVolume->bits       = d3dLockBox.pBits;
            lockedVolume->rowPitch   = d3dLockBox.RowPitch;
            lockedVolume->slicePitch = d3dLockBox.SlicePitch;
            return true;
        }

        return false;
    }

    void D3D9TextureObj::unlockVolume( int level )
    {
        m_texVolume->UnlockBox( level );
    }

    void D3D9TextureObj::onLostDevice()
    {
        _destroySurfaceObjArray();

        if ( IDirect3DBaseTexture9* tex = getBaseTex() )
        {
            tex->Release();
            m_tex = 0;
        }
    }

    void D3D9TextureObj::onResetDevice()
    {
        TexObjCreateParas paras;

        paras.type   = m_type;
        paras.usage  = m_usage;
        paras.format = m_format;
        paras.levels = m_levels;
        paras.width  = m_width;
        paras.height = m_height;
        paras.depth  = m_depth;

        khaosAssert( m_usage != TEXUSA_STATIC );
        create( paras );
    }

    void D3D9TextureObj::save( pcstr file ) 
    {
        D3DXSaveTextureToFileA( file, D3DXIFF_DDS, getBaseTex(), 0 );
    }

    int D3D9TextureObj::getLevelWidth( int level ) const
    {
        if ( m_type == TEXTYPE_2D )
        {
            D3DSURFACE_DESC desc = {};
            m_tex->GetLevelDesc( level, &desc );
            return (int) desc.Width;
        }
        else if ( m_type == TEXTYPE_CUBE )
        {
            D3DSURFACE_DESC desc = {};
            m_texCube->GetLevelDesc( level, &desc );
            return (int) desc.Width;
        }
        else if ( m_type == TEXTYPE_VOLUME )
        {
            D3DVOLUME_DESC desc = {};
            m_texVolume->GetLevelDesc( level, &desc );
            return (int) desc.Width;
        }
        
        khaosAssert(0);
        return 0;
    }

    int D3D9TextureObj::getLevelHeight( int level ) const
    {
        if ( m_type == TEXTYPE_2D )
        {
            D3DSURFACE_DESC desc = {};
            m_tex->GetLevelDesc( level, &desc );
            return (int) desc.Height;
        }
        else if ( m_type == TEXTYPE_CUBE )
        {
            D3DSURFACE_DESC desc = {};
            m_texCube->GetLevelDesc( level, &desc );
            return (int) desc.Height;
        }
        else if ( m_type == TEXTYPE_VOLUME )
        {
            D3DVOLUME_DESC desc = {};
            m_texVolume->GetLevelDesc( level, &desc );
            return (int) desc.Height;
        }

        khaosAssert(0);
        return 0;
    }

    int D3D9TextureObj::getLevelDepth( int level ) const
    {
        if ( m_type == TEXTYPE_VOLUME )
        {
            D3DVOLUME_DESC desc = {};
            m_texVolume->GetLevelDesc( level, &desc );
            return (int) desc.Depth;
        }

        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    void D3DShaderHolder::_filterCreateParams( uint64 id, const char*& code, 
        const CodeContext& binCode, const char* profile, char* file )
    {
        // 调试记录文件
#if TEST_DEBUG_SHADER_
        String strID = uint64ToHexStr( id );
        sprintf( file, _MAKE_PROJ_ROOT_DIR("Samples/Media/ShaderSrc/%s_%s.%s"), profile, strID.c_str(), profile );
        writeStringToFile( code, file );
#endif

        // 加载策略
#if SHADER_CREATE_TYPE == SHADER_BIN_ONLY
        code = 0;
    #if TEST_DEBUG_SHADER_
        file[0] = 0;
    #endif
#elif SHADER_CREATE_TYPE == SHADER_SOURCE_ONLY
        const_cast<CodeContext&>(binCode).src = 0;
#endif
    }

    void D3DShaderHolder::_getShaderObjFunc( const char* file, const char* code,
        const CodeContext& binCode, const char* target,
        LPD3DXBUFFER& pShader, DWORD*& func )
    {
        pShader = 0;
        func = 0;

        // 二进制优先
        if ( binCode.src ) 
        {
            func = (DWORD*)binCode.src;
            return;
        }

        // 无二进制通过源码编译
        LPD3DXBUFFER pErrorMsgs = 0;
        HRESULT      hr = S_OK;

#if TEST_DEBUG_SHADER_
        hr = D3DXCompileShaderFromFileA( file, 0, 0, "main", target, D3DXSHADER_DEBUG, &pShader, &pErrorMsgs, 0 );
#else
            int codeLen = (int)strlen(code);
        hr = D3DXCompileShader( code, codeLen, 0, 0, "main", target, 0, &pShader, &pErrorMsgs, 0 );
#endif

        if ( pErrorMsgs )
        {
            khaosLogLn( KHL_L1, "CompileShader[%s] msg: %s", target, pErrorMsgs->GetBufferPointer() );
            pErrorMsgs->Release();
        }

        if ( FAILED(hr) )
        {
            if ( pShader )
                pShader->Release();
            return;
        }

        func = (DWORD*)pShader->GetBufferPointer();
    }

    D3DVertexShaderHolder::~D3DVertexShaderHolder()
    {
        _safeComRelease( m_vs );
    }

    bool D3DVertexShaderHolder::create( uint64 id, const char* code, const CodeContext& binCode )
    {
        // filter params
        char file[1024] = {};
        _filterCreateParams( id, code, binCode, "vs", file );

        // get shader object function
        LPD3DXBUFFER pShader = 0;
        DWORD* func = 0;
        _getShaderObjFunc( file, code, binCode, "vs_3_0", pShader, func );
        
        // create
        bool ret = true;

        if ( FAILED( g_d3dDevice->CreateVertexShader( func, &m_vs ) ) )
        {
            khaosLogLn( KHL_L1, "CreateVertexShader failed" );
            ret = false;
        }

        if ( pShader )
            pShader->Release();
        return true;
    }

    void D3DVertexShaderHolder::setFloat4N( int registerIdx, const float* v, int size )
    {
        g_d3dDevice->SetVertexShaderConstantF( registerIdx, v, size );
    }

    void D3DVertexShaderHolder::setInt4N( int registerIdx, const int* v, int size )
    {
        g_d3dDevice->SetVertexShaderConstantI( registerIdx, v, size );
    }

    D3DPixelShaderHolder::~D3DPixelShaderHolder()
    {
        _safeComRelease(m_ps);
    }

    bool D3DPixelShaderHolder::create( uint64 id, const char* code, const CodeContext& binCode )
    {
        // filter params
        char file[1024] = {};
        _filterCreateParams( id, code, binCode, "ps", file );

        // get shader object function
        LPD3DXBUFFER pShader = 0;
        DWORD* func = 0;
        _getShaderObjFunc( file, code, binCode, "ps_3_0", pShader, func );

        // create
        bool ret = true;

        if ( FAILED( g_d3dDevice->CreatePixelShader( func, &m_ps ) ) )
        {
            khaosLogLn( KHL_L1, "CreatePixelShader failed" );
            ret = false;
        }

        if ( pShader )
            pShader->Release();
        return true;
    }

    void D3DPixelShaderHolder::setFloat4N( int registerIdx, const float* v, int size )
    {
        g_d3dDevice->SetPixelShaderConstantF( registerIdx, v, size );
    }

    void D3DPixelShaderHolder::setInt4N( int registerIdx, const int* v, int size )
    {
        g_d3dDevice->SetPixelShaderConstantI( registerIdx, v, size );
    }

    D3DShader::D3DShader() : m_holder(0)
    {
    }
    
    D3DShader::~D3DShader()
    {
        KHAOS_DELETE m_holder;
    }

    bool D3DShader::_createImpl( uint64 id, const ShaderParser& parser, const CodeContext& binCode )
    {
        String strCode;

        // section head
        _addSection( strCode, parser.getSectionHead() );
        strCode += "\n\n";

        // uniform pack
        _addSection( strCode, parser.getUniformPackMap() );
        strCode += "\n\n";

        // section uniform
        _addSection( strCode, parser.getSectionUniform(), true );
        strCode += "\n\n";

        // section in
        _addSection( strCode, parser.getSectionInVar(), "PARAMS_IN" );

        // section out
        _addSection( strCode, parser.getSectionOutVar(), "PARAMS_OUT" );

        // section common
        _addSection( strCode, parser.getSectionComm() );
        strCode += "\n\n";

        // void main(
        strCode += "void main( PARAMS_IN pIn, out PARAMS_OUT pOut )\n{\n";

        // section main code
        _addSection( strCode, parser.getSectionMainFunc() );

        // }
        strCode += "\n}\n\n";

        return m_holder->create( id, strCode.c_str(), binCode );
    }

    void D3DShader::_addSection( String& strCode, const ShaderParser::UniformPackMap& upm )
    {
        // 遍历包结构
        for ( ShaderParser::UniformPackMap::const_iterator it = upm.begin(), ite = upm.end(); it != ite; ++it )
        {
            const ShaderParser::UniformPack* pack = it->second;

            // 包名字
            strCode += "struct " + pack->packName + "\n{\n";

            // 生成每个包内元素
            _addSection( strCode, pack->itemList, false );

            // 包尾巴
            strCode += "};\n";
        }
    }

    void D3DShader::_addSection( String& strCode, const StringVector& strVec )
    {
        for ( StringVector::const_iterator it = strVec.begin(), ite = strVec.end(); it != ite; ++it )
        {
            const String& line = *it;
            strCode += line;
            strCode += "\n";
        }
    }

    void D3DShader::_addSection( String& strCode, const ShaderParser::UniformVarItemList& uvarList, bool showRegister )
    {
        for ( ShaderParser::UniformVarItemList::const_iterator it = uvarList.begin(), ite = uvarList.end(); it != ite; ++it )
        {
            const ShaderParser::UniformVarItem& item = *it;

            if ( const ShaderParser::UniformVar* uvar = item.var ) // 变量
            {
                // type name[n] : register(__auto__);
                strCode += uvar->typeName + " " + uvar->varName;

                if ( uvar->isArray() ) // 数组
                    strCode += "[" + intToString(uvar->arraySize) + "]";

                if ( showRegister ) // 是否显示寄存器
                    strCode += String(" : register(") + _getRegisterName(uvar->regType) + intToString(uvar->regBegin) + ")";
                
                strCode += ";\n";
            }
            else // 其他指令
            {
                khaosAssert( item.line );
                strCode += *item.line;
                strCode += "\n";
            }
        }
    }

    void D3DShader::_addSection( String& strCode, const ShaderParser::InOutVarItemList& iovarList, const String& structName )
    {
        strCode += "struct " + structName + "\n{\n";

        for ( ShaderParser::InOutVarItemList::const_iterator it = iovarList.begin(), ite = iovarList.end(); it != ite; ++it )
        {
            const ShaderParser::InOutVarItem& item = *it;

            if ( ShaderParser::InOutVar* iovar = item.var ) // 变量
            {
                // type name : sema[,]
                strCode += iovar->typeName + " " + iovar->varName + " : " + iovar->semantics;
                
                //if ( (it + 1 != ite) ) // 不是最后个加上;
                    strCode += ";\n";
                //else
                //    strCode += "\n";
            }
            else // 其他命令
            {
                khaosAssert( item.line );
                strCode += *item.line;
                strCode += "\n";
            }
        }

        strCode += "};\n";
    }

    pcstr D3DShader::_getRegisterName( ShaderParser::RegisterType regType )
    {
        static pcstr n[] = { "c", "i", "s" };
        khaosAssert ( 0 <= regType && regType < KHAOS_ARRAY_SIZE(n) );
        return n[regType];
    }

    template<typename T>
    const T* D3DShader::_makeUniformData( T* dest, const T* src, int elementSize, int elementCount )
    {
        // 正好4个元素对齐
        if ( elementSize == 4 )
            return src;

        // 需要补充字节对齐
        khaosAssert( elementSize < 4 );
        int perCopyedSize = sizeof(T) * elementSize;
        int perFilledSize = sizeof(T) * (4 - elementSize);

        T* pos = dest;
        for ( int it = 0; it < elementCount; ++it )
        {
            memcpy( pos, src, perCopyedSize );
            memset( pos+elementSize, 0, perFilledSize );

            src += elementSize;
            pos += 4;
        }

        return dest;
    }

    void D3DShader::setMatrix3( int registerIdx, const Matrix3& mat3 )
    {
        Matrix4 mat4(mat3);
        m_holder->setFloat4N( registerIdx, mat4[0], 3 );
    }

    void D3DShader::setMatrix4( int registerIdx, const Matrix4& mat4 )
    {
        m_holder->setFloat4N( registerIdx, mat4[0], 4 );
    }

    void D3DShader::setMatrix4Array( int registerIdx, const Matrix4* mat4, int count )
    {
        m_holder->setFloat4N( registerIdx, (*mat4)[0], 4 * count );
    }

    //////////////////////////////////////////////////////////////////////////
    D3DShader* _createD3DShader( ShaderProfile profile, uint64 id, const EffectCreateContext& context )
    {
        D3DShader* s = KHAOS_NEW D3DShader;

        switch ( profile )
        {
        case SP_VERTEXSHADER:
            s->_setHolder( KHAOS_NEW D3DVertexShaderHolder );
            break;

        case SP_PIXELSHADER:
            s->_setHolder( KHAOS_NEW D3DPixelShaderHolder );
            break;

        default:
            khaosAssert(0);
            break;
        }
        
        if ( s->create( id, context.codes[profile], context.heads, context.binCodes[profile] ) )
            return s;

        KHAOS_DELETE s;
        return 0;
    }

    bool D3D9Effect::create( uint64 id, const EffectCreateContext& context )
    {
        for ( int i = 0; i < SP_MAXCOUNT; ++i )
        {
            if ( context.codes[i].size > 0 )
            {
                m_shaders[i] = _createD3DShader( (ShaderProfile)i, id, context );

                if ( !m_shaders[i] )
                    return false;
            }
        }
      
        _buildParamEx(); // 创建const table
        return true;
    }


    //////////////////////////////////////////////////////////////////////////
    RenderDevice* createRenderDevice()
    {
        return KHAOS_NEW D3D9RenderDevice;
    }

    D3D9RenderDevice::D3D9RenderDevice() : 
        m_d3D(0), m_d3DDevice(0),
        m_currDrawVB(0), m_currDrawIB(0),
        m_lostDevice(false), m_callBeginScene(false)
    {
        m_mainRenderTarget = KHAOS_NEW D3D9SurfaceObj;
        m_mainDepthStencil = KHAOS_NEW D3D9TextureObj;
    }

    D3D9RenderDevice::~D3D9RenderDevice()
    {
        KHAOS_DELETE m_mainDepthStencil;
        KHAOS_DELETE m_mainRenderTarget;
    }

    void D3D9RenderDevice::init( const RenderDeviceCreateContext& context )
    {
        do
        {
            // 创建d3d9
            m_d3D = Direct3DCreate9(D3D_SDK_VERSION);
            if ( !m_d3D )
                break;

            // 获取显示适配模式
            if ( FAILED( m_d3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &m_d3ddm ) ) )
                break;

            // 创建d3d9设备
            ZeroMemory( &m_d3dpp, sizeof(m_d3dpp) );

            m_d3dpp.BackBufferWidth              = context.windowWidth;
            m_d3dpp.BackBufferHeight             = context.windowHeight;
            m_d3dpp.BackBufferFormat             = m_d3ddm.Format;
            m_d3dpp.BackBufferCount              = 1;
            m_d3dpp.MultiSampleType              = D3DMULTISAMPLE_NONE;
            m_d3dpp.MultiSampleQuality           = 0;
            m_d3dpp.SwapEffect                   = D3DSWAPEFFECT_DISCARD;
            m_d3dpp.hDeviceWindow                = (HWND)context.handleWindow;
            m_d3dpp.Windowed                     = TRUE;
            m_d3dpp.EnableAutoDepthStencil       = FALSE; //TRUE
            m_d3dpp.AutoDepthStencilFormat       = D3DFMT_UNKNOWN; //D3DFMT_D24S8;
            m_d3dpp.Flags                        = 0; //D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
            m_d3dpp.FullScreen_RefreshRateInHz   = 0;
            m_d3dpp.PresentationInterval         = D3DPRESENT_INTERVAL_IMMEDIATE; // 关闭垂直同步

            // 创建
            if ( FAILED( m_d3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (HWND)context.handleWindow, 
                            D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_d3dpp, &m_d3DDevice ) ) )
                break;

            // 获取设备特性
            m_d3DDevice->GetDeviceCaps( &m_d3dCaps );

            m_windowWidth  = m_d3dpp.BackBufferWidth;
            m_windowHeight = m_d3dpp.BackBufferHeight;

            _createRtAndDs();
            _getViewport();
            return;
        }
        while (0);

        // 创建失败
        shutdown();
    }

    void D3D9RenderDevice::shutdown()
    {
        _freeRtAndDs();
        _safeComRelease(m_d3DDevice);
        _safeComRelease(m_d3D);
    }

    VertexBuffer* D3D9RenderDevice::createVertexBuffer()
    {
        return KHAOS_NEW D3D9VertexBuffer;
    }

    VertexDeclaration* D3D9RenderDevice::createVertexDeclaration()
    {
        return KHAOS_NEW D3D9VertexDeclaration;
    }

    TextureObj* D3D9RenderDevice::createTextureObj()
    {
        return KHAOS_NEW D3D9TextureObj;
    }

    SurfaceObj* D3D9RenderDevice::createSurfaceObj()
    {
        return KHAOS_NEW D3D9SurfaceObj;
    }

    IndexBuffer* D3D9RenderDevice::createIndexBuffer()
    {
        return KHAOS_NEW D3D9IndexBuffer;
    }

    Effect* D3D9RenderDevice::createEffect()
    {
        return KHAOS_NEW D3D9Effect;
    }

    bool D3D9RenderDevice::isTextureFormatSupported( TextureUsage usage, TextureType type, PixelFormat fmt )
    {
        D3DRESOURCETYPE d3drestypes[] =
        {
            D3DRTYPE_TEXTURE, // TEXTYPE_2D
            D3DRTYPE_VOLUMETEXTURE, // TEXTYPE_3D
            D3DRTYPE_CUBETEXTURE // TEXTYPE_CUBE
        };

        khaosAssert( type < KHAOS_ARRAY_SIZE(d3drestypes) );

        return SUCCEEDED(
            m_d3D->CheckDeviceFormat( m_d3dCaps.AdapterOrdinal, m_d3dCaps.DeviceType, m_d3ddm.Format,
            _toD3DTexUsage(usage), d3drestypes[type], _toD3DTexFormat(fmt) ) );
    }

    void D3D9RenderDevice::_registerDeviceObject( ID3D9DeviceObject* deviceObj )
    {
        m_deviceObjectList.insert( deviceObj );
    }

    void D3D9RenderDevice::_unregisterDeviceObject( ID3D9DeviceObject* deviceObj )
    {
        m_deviceObjectList.erase( deviceObj );
    }

    void D3D9RenderDevice::_onLostDevice()
    {
        _freeRtAndDs();

        for ( DeviceObjectList::iterator it = m_deviceObjectList.begin(), ite = m_deviceObjectList.end(); it != ite; ++it )
        {
            ID3D9DeviceObject* obj = *it;
            obj->onLostDevice();
        }
    }

    void D3D9RenderDevice::_onResetDevice()
    {
        _createRtAndDs();
        _getViewport();

        for ( DeviceObjectList::iterator it = m_deviceObjectList.begin(), ite = m_deviceObjectList.end(); it != ite; ++it )
        {
            ID3D9DeviceObject* obj = *it;
            obj->onResetDevice();
        }

        _resetState();
    }

    bool D3D9RenderDevice::_testCooperativeLevel()
    {
        if ( !m_lostDevice )
            return true;

        HRESULT hr = S_OK;

        // Test the cooperative level to see if it's okay to render
        if ( FAILED( hr = m_d3DDevice->TestCooperativeLevel() ) )
        {
            // If the device was lost, do not render until we get it back
            if ( D3DERR_DEVICELOST == hr )
                return false;

            // Check if the device needs to be reset.
            if ( D3DERR_DEVICENOTRESET == hr )
            {
                // Release all vidmem objects
                _onLostDevice();

                // Get current display mode
                if ( FAILED( m_d3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &m_d3ddm ) ) )
                    return false;

                m_d3dpp.BackBufferFormat = m_d3ddm.Format;

                // Reset the device
                if ( FAILED( m_d3DDevice->Reset( &m_d3dpp ) ) )
                    return false;

                m_windowWidth  = m_d3dpp.BackBufferWidth;
                m_windowHeight = m_d3dpp.BackBufferHeight;

                // Restore device objects
                _onResetDevice();
            }
            else // Unknown error
            {
                return false;
            }
        }

        // Restore all ok
        m_lostDevice = false;
        return true;
    }

    void D3D9RenderDevice::_createRtAndDs()
    {
        // main render target
        D3D9SurfaceObj* mainRenderTarget = static_cast<D3D9SurfaceObj*>(m_mainRenderTarget);

        LPDIRECT3DSURFACE9 rt = 0;
        m_d3DDevice->GetRenderTarget( 0, &rt );
        mainRenderTarget->_bindHandle( rt, TEXUSA_RENDERTARGET );

        // depth stencil
        TexObjCreateParas paras;

        paras.type   = TEXTYPE_2D;
        paras.usage  = TEXUSA_DEPTHSTENCIL;
        paras.format = PIXFMT_INTZ;
        paras.levels = 1;
        paras.width  = m_windowWidth;
        paras.height = m_windowHeight;

        m_mainDepthStencil->create( paras );
        _unregisterDeviceObject( static_cast<D3D9TextureObj*>(m_mainDepthStencil) ); // 我们自己管理重建，不注册托管设备丢失
    }

    void D3D9RenderDevice::_freeRtAndDs()
    {
        static_cast<D3D9SurfaceObj*>(m_mainRenderTarget)->_unbindHandle();
        m_mainDepthStencil->destroy();
    }

    void D3D9RenderDevice::beginRender()
    {
        if ( _testCooperativeLevel() )
        {
            m_d3DDevice->BeginScene();
            m_callBeginScene = true;
        }
        else
        {
            m_callBeginScene = false;
        }
    }

    void D3D9RenderDevice::endRender()
    {
        if ( m_callBeginScene ) // must match with BeginScene
        {
            m_d3DDevice->EndScene();

            HRESULT hr = m_d3DDevice->Present( 0, 0, 0, 0 );

            if ( D3DERR_DEVICELOST == hr )
                m_lostDevice = true;
        }
    }

    void D3D9RenderDevice::_setRenderTarget( int idx, SurfaceObj* surTarget )
    {
        m_d3DDevice->SetRenderTarget( idx, surTarget ? static_cast<D3D9SurfaceObj*>(surTarget)->getSur() : 0 );
    }

    void D3D9RenderDevice::_setDepthStencil( SurfaceObj* surDepthStencil )
    {
        m_d3DDevice->SetDepthStencilSurface( static_cast<D3D9SurfaceObj*>(surDepthStencil)->getSur() );
    }

    void D3D9RenderDevice::_getViewport()
    {
        m_d3DDevice->GetViewport( &m_currViewport );
    }

    void D3D9RenderDevice::setViewport( const IntRect& rect )
    {
        m_currViewport.X      = rect.left;
        m_currViewport.Y      = rect.top;
        m_currViewport.Width  = rect.getWidth();
        m_currViewport.Height = rect.getHeight();

        m_d3DDevice->SetViewport( &m_currViewport );
    }

    void D3D9RenderDevice::clear( uint32 flags, const Color* clr, float z, uint stencil )
    {
        DWORD d3dFlag = 0;
        if ( flags & RCF_TARGET )
            d3dFlag |= D3DCLEAR_TARGET;
        if ( flags & RCF_DEPTH )
            d3dFlag |= D3DCLEAR_ZBUFFER;
        if ( flags & RCF_STENCIL )
            d3dFlag |= D3DCLEAR_STENCIL;
      
        D3DCOLOR d3dclr = clr ? clr->getAsARGB() : 0;

        m_d3DDevice->Clear( 0, 0, d3dFlag, d3dclr, z, stencil );
    }

    void D3D9RenderDevice::_setEffect( Effect* effect )
    {
        D3DShader* vs = static_cast<D3DShader*>(effect->getVertexShader());
        D3DShader* ps = static_cast<D3DShader*>(effect->getPixelShader());

        D3DVertexShaderHolder* vsh = static_cast<D3DVertexShaderHolder*>(vs->_getHolder());
        D3DPixelShaderHolder*  psh = static_cast<D3DPixelShaderHolder*>(ps->_getHolder());

        m_d3DDevice->SetVertexShader( vsh->_getVS() );
        m_d3DDevice->SetPixelShader( psh->_getPS() );
    }

    void D3D9RenderDevice::setVertexBuffer( VertexBuffer* vb )
    {
        m_currDrawVB = static_cast<D3D9VertexBuffer*>(vb);
        D3D9VertexDeclaration* d3dvd = static_cast<D3D9VertexDeclaration*>(vb->getDeclaration());
        m_d3DDevice->SetVertexDeclaration( d3dvd->getVD() );
        m_d3DDevice->SetStreamSource( 0, m_currDrawVB->getVB(), 0, d3dvd->getStride() );
    }

    void D3D9RenderDevice::setIndexBuffer( IndexBuffer* ib )
    {
        m_currDrawIB = static_cast<D3D9IndexBuffer*>(ib);
        m_d3DDevice->SetIndices( m_currDrawIB->getIB() );
    }

    void D3D9RenderDevice::drawIndexedPrimitive( PrimitiveType type, int startIndex, int primitiveCount )
    {
        m_d3DDevice->DrawIndexedPrimitive( _toD3DPrimType(type), 0, 0, m_currDrawVB->getVertexCount(), 
            startIndex, primitiveCount );
    }

    void D3D9RenderDevice::drawPrimitive( PrimitiveType type, int startVertex, int primitiveCount )
    {
        m_d3DDevice->DrawPrimitive( _toD3DPrimType(type), startVertex, primitiveCount );
    }

    void D3D9RenderDevice::toDeviceViewMatrix( Matrix4& matView )
    {
        // 引擎右手系(gl标准)，dx左手系，在视空间中只要z轴反一下即可
        matView[2][0] = -matView[2][0];
        matView[2][1] = -matView[2][1];
        matView[2][2] = -matView[2][2];
        matView[2][3] = -matView[2][3];
    }

    void D3D9RenderDevice::toDeviceProjMatrix( Matrix4& matProj )
    {
        // 引擎投影后z范围是[-1,1](gl标准)，dx是[0,1]，这里转换下
        
        // gl透视矩阵：(这里矩阵 v = M * v)
        // 其中 f = cot(fovy/2),  asp = w / h
        //
        // f/asp    0         0                 0 
        //   0      f         0                 0
        //   0      0   (zn+zf)/(zn-zf)     (2*zn*zf)/(zn-zf)
        //   0      0         -1                0
        //
        //  dx透视矩阵：(同上，所以和dx文档上行列转置了下)
        //
        //  f/asp   0       0               0
        //   0      f       0               0
        //   0      0    zf/(zf-zn)   (-zn*zf)/(zf-zn)
        //   0      0       1               0

        // 无论如何（请无视上面的透视矩阵，写出来只供回忆用，下面处理对于任何投影矩阵都一致）：
        // Vz' = Xz*Vx + Yz*Vy + Zz*Vz + Wz*Vw (其中X,Y,Z是各个基，V是顶点，x,y,z是分量)
        // Vw' = Xw*Vx + Yw*Vy + Zw*Vz + Ww*Vw
        // 将齐次坐标转换后Vzp' = Vz'/Vw' = (Xz*Vx + Yz*Vy + Zz*Vz + Wz*Vw) / (Xw*Vx + Yw*Vy + Zw*Vz + Ww*Vw)
        // 如果另：Xz' = (Xz + Xw)/2， Yz' = (Yz + Yw)/2, .....
        // 则： Vzp'' = Vz'/Vw' = (Xz'*Vx + Yz'*Vy + Zz'*Vz + Wz'*Vw) / (Xw*Vx + Yw*Vy + Zw*Vz + Ww*Vw)
        //            =  0.5 * (Xz*Vx + Yz*Vy + Zz*Vz + Wz*Vw) / (Xw*Vx + Yw*Vy + Zw*Vz + Ww*Vw) +
        //               0.5 * (Xw*Vx + Yw*Vy + Zw*Vz + Ww*Vw) / (Xw*Vx + Yw*Vy + Zw*Vz + Ww*Vw)
        //            = 0.5 * Vzp' + 0.5
        // 这样范围就由[-1,1]变换到[0, 1]了，因此只需对第2行做变换

        matProj[2][0] = (matProj[2][0] + matProj[3][0]) / 2;
        matProj[2][1] = (matProj[2][1] + matProj[3][1]) / 2;
        matProj[2][2] = (matProj[2][2] + matProj[3][2]) / 2;
        matProj[2][3] = (matProj[2][3] + matProj[3][3]) / 2;

        // 这个转换后
        // 当z = -zn => z' = 0
        // 当z = -zf => z' = 1
        // 而dx投影矩阵
        // 当z = zn => z' = 0
        // 当z = zf => z' = 1
        // 因为dx是左手系，引擎右手系，所以翻转z
        matProj[0][2] = -matProj[0][2];
        matProj[1][2] = -matProj[1][2];
        matProj[2][2] = -matProj[2][2];
        matProj[3][2] = -matProj[3][2];
    }

    void D3D9RenderDevice::enableColorWriteenable( int rtIdx, bool enR, bool enG, bool enB, bool enA )
    {
        static const D3DRENDERSTATETYPE channel[] = 
        {
            D3DRS_COLORWRITEENABLE,
            D3DRS_COLORWRITEENABLE1,
            D3DRS_COLORWRITEENABLE2,
            D3DRS_COLORWRITEENABLE3
        };
        
        uint val = 0;

        if ( enR )
            val |= D3DCOLORWRITEENABLE_RED;

        if ( enG )
            val |= D3DCOLORWRITEENABLE_GREEN;

        if ( enB )
            val |= D3DCOLORWRITEENABLE_BLUE;

        if ( enA )
            val |= D3DCOLORWRITEENABLE_ALPHA;

        khaosAssert( 0 <= rtIdx && rtIdx < KHAOS_ARRAY_SIZE(channel) );
        m_d3DDevice->SetRenderState( channel[rtIdx], val );
    }

    void D3D9RenderDevice::enableSRGBWrite( bool en )
    {
        m_d3DDevice->SetRenderState( D3DRS_SRGBWRITEENABLE, en );
    }

    void D3D9RenderDevice::_enableZTest( bool en )
    {
        m_d3DDevice->SetRenderState( D3DRS_ZENABLE, en ? D3DZB_TRUE : D3DZB_FALSE );
    }

    void D3D9RenderDevice::_enableZWrite( bool en )
    {
        m_d3DDevice->SetRenderState( D3DRS_ZWRITEENABLE, en );
    }

    void D3D9RenderDevice::_setZFunc( CmpFunc func )
    {
        m_d3DDevice->SetRenderState( D3DRS_ZFUNC, _toD3DCmpFunc(func) );
    }

    void D3D9RenderDevice::setDepthBias( float sca, float off )
    {
        m_d3DDevice->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&sca );
        m_d3DDevice->SetRenderState( D3DRS_DEPTHBIAS, *(DWORD*)&off );
    }

    void D3D9RenderDevice::_enableStencil( bool en ) 
    {
        m_d3DDevice->SetRenderState( D3DRS_STENCILENABLE, en ? TRUE : FALSE );
    }

    void D3D9RenderDevice::_setStencil_refVal( uint8 refVal )
    {
        m_d3DDevice->SetRenderState( D3DRS_STENCILREF, refVal );
    }

    void D3D9RenderDevice::_setStencil_cmpFunc( CmpFunc cmpFunc )
    {
        m_d3DDevice->SetRenderState( D3DRS_STENCILFUNC, _toD3DCmpFunc(cmpFunc) );
    }

    void D3D9RenderDevice::_setStencil_cmpMask( uint8 cmpMask )
    {
        m_d3DDevice->SetRenderState( D3DRS_STENCILMASK, cmpMask );
    }

    void D3D9RenderDevice::_setStencil_writeMask( uint8 writeMask )
    {
        m_d3DDevice->SetRenderState( D3DRS_STENCILWRITEMASK, writeMask );
    }

    void D3D9RenderDevice::_setStencil_stencilFailOp( StencilOp stencilFailOp )
    {
        m_d3DDevice->SetRenderState( D3DRS_STENCILFAIL, _toD3DStencilOp(stencilFailOp) );
    }

    void D3D9RenderDevice::_setStencil_zFailOp( StencilOp zFailOp )
    {
        m_d3DDevice->SetRenderState( D3DRS_STENCILZFAIL, _toD3DStencilOp(zFailOp) );
    }

    void D3D9RenderDevice::_setStencil_bothPassOp( StencilOp bothPassOp )
    {
        m_d3DDevice->SetRenderState( D3DRS_STENCILPASS, _toD3DStencilOp(bothPassOp) );
    }

    void D3D9RenderDevice::_enableBlend( bool en )
    {
        m_d3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, en );
    }

    void D3D9RenderDevice::_setBlendOp( BlendOp op )
    {
        m_d3DDevice->SetRenderState( D3DRS_BLENDOP, _toD3DBlendOp(op) );
    }

    void D3D9RenderDevice::_setSrcBlend( BlendVal val )
    {
        m_d3DDevice->SetRenderState( D3DRS_SRCBLEND, _toD3DBlendVal(val) );
    }

    void D3D9RenderDevice::_setDestBlend( BlendVal val )
    {
        m_d3DDevice->SetRenderState( D3DRS_DESTBLEND, _toD3DBlendVal(val) );
    }

    void D3D9RenderDevice::_setCullMode( CullMode cm )
    {
        m_d3DDevice->SetRenderState( D3DRS_CULLMODE, _toD3DCull(cm) );
    }

    void D3D9RenderDevice::_setWireframe( bool en )
    {
        m_d3DDevice->SetRenderState( D3DRS_FILLMODE, en ? D3DFILL_WIREFRAME : D3DFILL_SOLID );
    }

    void D3D9RenderDevice::_setTexture( int sampler, TextureObj* texObj )
    {
        D3D9TextureObj* d3dTexObj = static_cast<D3D9TextureObj*>(texObj);
        m_d3DDevice->SetTexture( sampler, d3dTexObj ? d3dTexObj->getBaseTex() : 0 );
    }

    void D3D9RenderDevice::_setMagFilter( int sampler, TextureFilter tf )
    {
        m_d3DDevice->SetSamplerState( sampler, D3DSAMP_MAGFILTER, _toD3DTexFilter(tf) );
    }

    void D3D9RenderDevice::_setMinFilter( int sampler, TextureFilter tf )
    {
        m_d3DDevice->SetSamplerState( sampler, D3DSAMP_MINFILTER, _toD3DTexFilter(tf) );
    }

    void D3D9RenderDevice::_setMipFilter( int sampler, TextureFilter tf )
    {
        m_d3DDevice->SetSamplerState( sampler, D3DSAMP_MIPFILTER, _toD3DTexFilter(tf) );
    }

    void D3D9RenderDevice::_setTexAddrU( int sampler, TextureAddress addr )
    {
        m_d3DDevice->SetSamplerState( sampler, D3DSAMP_ADDRESSU, _toD3DTexAddr(addr) );
    }

    void D3D9RenderDevice::_setTexAddrV( int sampler, TextureAddress addr )
    {
        m_d3DDevice->SetSamplerState( sampler, D3DSAMP_ADDRESSV, _toD3DTexAddr(addr) );
    }

    void D3D9RenderDevice::_setTexAddrW( int sampler, TextureAddress addr )
    {
        m_d3DDevice->SetSamplerState( sampler, D3DSAMP_ADDRESSW, _toD3DTexAddr(addr) );
    }

    void D3D9RenderDevice::_setBorderColor( int sampler, const Color& clr )
    {
        m_d3DDevice->SetSamplerState( sampler, D3DSAMP_BORDERCOLOR, clr.getAsARGB() );
    }

    void D3D9RenderDevice::_setMipLodBias( int sampler, float mipLodBias )
    {
        m_d3DDevice->SetSamplerState( sampler, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&mipLodBias );
    }

    void D3D9RenderDevice::_setMipMaxLevel( int sampler, int mipMaxLevel )
    {
        m_d3DDevice->SetSamplerState( sampler, D3DSAMP_MAXMIPLEVEL, mipMaxLevel );
    }

    void D3D9RenderDevice::_setMaxAnisotropy( int sampler, int maxAnisotropy )
    {
        m_d3DDevice->SetSamplerState( sampler, D3DSAMP_MAXANISOTROPY, maxAnisotropy );
    }

    void D3D9RenderDevice::_setSRGB( int sampler, bool en )
    {
        m_d3DDevice->SetSamplerState( sampler, D3DSAMP_SRGBTEXTURE, en ? TRUE : FALSE );
    }

    bool D3D9RenderDevice::isDepthAcceptSize( int depthWidth, int depthHeight, int rttWidth, int rttHeight )
    {
        // dx要求depth大于等于rtt尺寸即可
        return depthWidth >= rttWidth && depthHeight >= rttHeight;
    }

    void D3D9RenderDevice::readSurfaceToCPU( SurfaceObj* surSrc, SurfaceObj* surDestOffscreen )
    {
        D3D9SurfaceObj* surSrcImpl  = static_cast<D3D9SurfaceObj*>(surSrc); 
        D3D9SurfaceObj* surDestImpl = static_cast<D3D9SurfaceObj*>(surDestOffscreen);

        D3DXLoadSurfaceFromSurface( surDestImpl->getSur(), 0, 0, surSrcImpl->getSur(), 0, 0,
            D3DX_FILTER_POINT, 0 );
    }
}

#endif

