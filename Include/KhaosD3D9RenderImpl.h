#pragma once
#if KHAOS_RENDER_SYSTEM == KHAOS_RENDER_SYSTEM_D3D9

#include "KhaosRenderDevice.h"
#include <d3d9.h>
#include <d3d9types.h>
#include <D3DX9Shader.h>

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    struct ID3D9DeviceObject
    {
        virtual void onLostDevice() = 0;
        virtual void onResetDevice() = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    // D3D9VertexDeclaration
    class D3D9VertexDeclaration : public VertexDeclaration
    {
    public:
        D3D9VertexDeclaration();
        virtual ~D3D9VertexDeclaration();

    public:
        LPDIRECT3DVERTEXDECLARATION9 getVD();

    private:
        LPDIRECT3DVERTEXDECLARATION9 m_decl;
    };

    //////////////////////////////////////////////////////////////////////////
    // D3D9VertexBuffer
    class D3D9VertexBuffer : public VertexBuffer, public ID3D9DeviceObject
    {
    public:
        D3D9VertexBuffer();
        virtual ~D3D9VertexBuffer();

    public:
        virtual bool  create( int size, HardwareBufferUsage usage );

        virtual void* lock( HardwareBufferAccess access, int offset = 0, int size = 0 );
        virtual void  unlock();

        virtual void onLostDevice();
        virtual void onResetDevice();

    public:
        LPDIRECT3DVERTEXBUFFER9 getVB() const { return m_vb; }

    protected:
        virtual void _destroyImpl();

    protected:
        LPDIRECT3DVERTEXBUFFER9 m_vb;
    };

    //////////////////////////////////////////////////////////////////////////
    // D3D9IndexBuffer
    class D3D9IndexBuffer : public IndexBuffer, public ID3D9DeviceObject
    {
    public:
        D3D9IndexBuffer();
        virtual ~D3D9IndexBuffer();

    public:
        virtual bool  create( int size, HardwareBufferUsage usage, IndexElementType type );

        virtual void* lock( HardwareBufferAccess access, int offset = 0, int size = 0 );
        virtual void  unlock();

        virtual void onLostDevice();
        virtual void onResetDevice();

    public:
        LPDIRECT3DINDEXBUFFER9 getIB() const { return m_ib; }

    protected:
        virtual void _destroyImpl();

    protected:
        LPDIRECT3DINDEXBUFFER9 m_ib;
    };

    //////////////////////////////////////////////////////////////////////////
    // D3D9SurfaceObj
    class D3D9SurfaceObj : public SurfaceObj, public ID3D9DeviceObject
    {
    public:
        D3D9SurfaceObj() : m_sur(0) {}
        virtual ~D3D9SurfaceObj();

    public:
        virtual bool create( int width, int height, TextureUsage usage, PixelFormat fmt );
        virtual bool createOffscreenPlain( int width, int height, PixelFormat fmt, int usage );
        virtual bool getFromTextureObj( TextureObj* texObj, int level );
        virtual bool getFromTextureObj( TextureObj* texObj, CubeMapFace face, int level );
        virtual void destroy();

        virtual bool lock( TextureAccess access, LockedRect* lockedRect, const IntRect* rect );
        virtual void unlock();

        virtual void save( pcstr file );

        virtual void onLostDevice();
        virtual void onResetDevice();

    public:
        void _bindHandle( LPDIRECT3DSURFACE9 sur, TextureUsage usage );
        void _unbindHandle();

        LPDIRECT3DSURFACE9 getSur() const { return m_sur; }

    private:
        LPDIRECT3DSURFACE9 m_sur;
    };

    //////////////////////////////////////////////////////////////////////////
    // D3D9TextureObj
    class D3D9TextureObj : public TextureObj, public ID3D9DeviceObject
    {
    public:
        D3D9TextureObj();
        virtual ~D3D9TextureObj();

    public:
        virtual bool load( const TexObjLoadParas& paras );
        virtual bool create( const TexObjCreateParas& paras );
        virtual void destroy();

        virtual bool lock( int level, TextureAccess access, LockedRect* lockedRect, const IntRect* rect );
        virtual void unlock( int level );

        virtual bool lockCube( CubeMapFace face, int level, TextureAccess access, LockedRect* lockedRect, const IntRect* rect );
        virtual void unlockCube( CubeMapFace face, int level );

        virtual bool lockVolume( int level, TextureAccess access, LockedBox* lockedVolume, const IntBox* box );
        virtual void unlockVolume( int level );

        virtual void fetchSurface();

        virtual void save( pcstr file );

        virtual int getLevelWidth( int level ) const;
        virtual int getLevelHeight( int level ) const;
        virtual int getLevelDepth( int level ) const;

        virtual void onLostDevice();
        virtual void onResetDevice();

    public:
        LPDIRECT3DTEXTURE9 getTex2D() const
        {
            khaosAssert( m_type == TEXTYPE_2D );
            return m_tex;
        }

        LPDIRECT3DCUBETEXTURE9 getTexCube() const 
        {
            khaosAssert( m_type == TEXTYPE_CUBE );
            return m_texCube; 
        }

        LPDIRECT3DVOLUMETEXTURE9 getTexVolume() const
        {
            khaosAssert( m_type == TEXTYPE_VOLUME );
            return m_texVolume;
        }

        IDirect3DBaseTexture9*  getBaseTex() const 
        {
            if ( m_type == TEXTYPE_2D )
                return m_tex;
            if ( m_type == TEXTYPE_CUBE )
                return m_texCube;
            if ( m_type == TEXTYPE_VOLUME )
                return m_texVolume;
            return 0;
        }

    private:
        bool _loadTex2D( const TexObjLoadParas& paras );
        bool _loadTexCube( const TexObjLoadParas& paras );
        bool _loadTexVolume( const TexObjLoadParas& paras );

        bool _createTex2D( const TexObjCreateParas& paras );
        bool _createTexCube( const TexObjCreateParas& paras );
        bool _createTexVolume( const TexObjCreateParas& paras );

    private:
        union
        {
            LPDIRECT3DTEXTURE9       m_tex;
            LPDIRECT3DCUBETEXTURE9   m_texCube;
            LPDIRECT3DVOLUMETEXTURE9 m_texVolume;
        };
    };

    //////////////////////////////////////////////////////////////////////////
    // Effect
    class D3DShaderHolder : public AllocatedObject
    {
    public:
        virtual ~D3DShaderHolder() {}

        virtual bool create( uint64 id, const char* code, const CodeContext& binCode ) = 0;
        virtual void setFloat4N( int registerIdx, const float* v, int size ) = 0;
        virtual void setInt4N( int registerIdx, const int* v, int size ) = 0;

    protected:
        void _filterCreateParams( uint64 id, const char*& code, 
            const CodeContext& binCode, const char* profile, char* file );

        void _getShaderObjFunc( const char* file, const char* code,
            const CodeContext& binCode, const char* target,
            LPD3DXBUFFER& pShader, DWORD*& func );
    };

    class D3DVertexShaderHolder : public D3DShaderHolder
    {
    public:
        D3DVertexShaderHolder() : m_vs(0) {}
        virtual ~D3DVertexShaderHolder();

        virtual bool create( uint64 id, const char* code, const CodeContext& binCode );
        virtual void setFloat4N( int registerIdx, const float* v, int size );
        virtual void setInt4N( int registerIdx, const int* v, int size );

    public:
        LPDIRECT3DVERTEXSHADER9 _getVS() const { return m_vs; }

    private:
        LPDIRECT3DVERTEXSHADER9 m_vs;
    };

    class D3DPixelShaderHolder : public D3DShaderHolder
    {
    public:
        D3DPixelShaderHolder() : m_ps(0) {}
        virtual ~D3DPixelShaderHolder();

        virtual bool create( uint64 id, const char* code, const CodeContext& binCode );
        virtual void setFloat4N( int registerIdx, const float* v, int size );
        virtual void setInt4N( int registerIdx, const int* v, int size );

    public:
        LPDIRECT3DPIXELSHADER9 _getPS() const { return m_ps; }

    private:
        LPDIRECT3DPIXELSHADER9 m_ps;
    };

    class D3DShader : public Shader
    {
    private:
        static const int MAX_UNIFORM_REGISTER_COUNT = 256;

    public:
        D3DShader();
        virtual ~D3DShader();

    public:
#define KHAOS_MAKE_UNIFORM_DATA_EX_D3D9_(name, type, regIdx, v, size, count) \
        type tmp[MAX_UNIFORM_REGISTER_COUNT]; \
        const type* data = _makeUniformData( tmp, v, size, count ); \
        _set##name##4N(regIdx, data, count)

#define KHAOS_MAKE_UNIFORM_DATA_D3D9_(name, type, v, size, count) \
        KHAOS_MAKE_UNIFORM_DATA_EX_D3D9_(name, type, registerIdx, v, size, count)

#define KHAOS_MAKE_SHADER_SET_METHOD_D3D9_(name, type) \
    public: \
        virtual void set##name( int registerIdx, type v ) { KHAOS_MAKE_UNIFORM_DATA_D3D9_(name, type, &v, 1, 1); } \
        virtual void set##name##2( int registerIdx, const type* v ) { KHAOS_MAKE_UNIFORM_DATA_D3D9_(name, type, v, 2, 1); } \
        virtual void set##name##3( int registerIdx, const type* v ) { KHAOS_MAKE_UNIFORM_DATA_D3D9_(name, type, v, 3, 1); } \
        virtual void set##name##4( int registerIdx, const type* v ) { KHAOS_MAKE_UNIFORM_DATA_D3D9_(name, type, v, 4, 1); } \
        virtual void set##name##Array( int registerIdx, const type* v, int count ) { KHAOS_MAKE_UNIFORM_DATA_D3D9_(name, type, v, 1, count); } \
        virtual void set##name##2Array( int registerIdx, const type* v, int count ) { KHAOS_MAKE_UNIFORM_DATA_D3D9_(name, type, v, 2, count); } \
        virtual void set##name##3Array( int registerIdx, const type* v, int count ) { KHAOS_MAKE_UNIFORM_DATA_D3D9_(name, type, v, 3, count); } \
        virtual void set##name##4Array( int registerIdx, const type* v, int count ) { KHAOS_MAKE_UNIFORM_DATA_D3D9_(name, type, v, 4, count); } \
        virtual void set##name##Array( int registerIdx, int startIndex, const type* v, int count ) { KHAOS_MAKE_UNIFORM_DATA_EX_D3D9_(name, type, registerIdx+startIndex, v, 1, count); } \
        virtual void set##name##2Array( int registerIdx, int startIndex, const type* v, int count ) { KHAOS_MAKE_UNIFORM_DATA_EX_D3D9_(name, type, registerIdx+startIndex, v, 2, count); } \
        virtual void set##name##3Array( int registerIdx, int startIndex, const type* v, int count ) { KHAOS_MAKE_UNIFORM_DATA_EX_D3D9_(name, type, registerIdx+startIndex, v, 3, count); } \
        virtual void set##name##4Array( int registerIdx, int startIndex, const type* v, int count ) { KHAOS_MAKE_UNIFORM_DATA_EX_D3D9_(name, type, registerIdx+startIndex, v, 4, count); } \
        virtual void set##name##Pack( int registerIdx, const void* v, int bytes ) { khaosAssert( bytes % (sizeof(type) * 4) == 0 ); _set##name##4N( registerIdx, (const type*)v, (bytes / (sizeof(type) * 4)) ); } \
    private: \
        void _set##name##4N( int registerIdx, const type* v, int count ) { m_holder->set##name##4N( registerIdx, v, count ); }

        KHAOS_MAKE_SHADER_SET_METHOD_D3D9_(Float, float)
        KHAOS_MAKE_SHADER_SET_METHOD_D3D9_(Int, int)

        virtual void setMatrix3( int registerIdx, const Matrix3& mat3 );
        virtual void setMatrix4( int registerIdx, const Matrix4& mat4 );
        virtual void setMatrix4Array( int registerIdx, const Matrix4* mat4, int count );

    public:
        void _setHolder( D3DShaderHolder* holder ) { m_holder = holder; }
        D3DShaderHolder* _getHolder() const { return m_holder; }

    private:
        virtual bool _createImpl( uint64 id, const ShaderParser& parser, const CodeContext& binCode );

        void _addSection( String& strCode, const StringVector& strVec );
        void _addSection( String& strCode, const ShaderParser::UniformVarItemList& uvarList, bool showRegister );
        void _addSection( String& strCode, const ShaderParser::InOutVarItemList& iovarList, const String& structName );
        void _addSection( String& strCode, const ShaderParser::UniformPackMap& upm );

    private:
        static pcstr _getRegisterName( ShaderParser::RegisterType regType );

        template<typename T>
        static const T* _makeUniformData( T* dest, const T* src, int elementSize, int elementCount );

    private:
        D3DShaderHolder* m_holder;
    };

    class D3D9Effect : public Effect
    {
    public:
        virtual bool create( uint64 id, const EffectCreateContext& context );
    };

    //////////////////////////////////////////////////////////////////////////
    // D3D9RenderDevice
    class D3D9RenderDevice : public RenderDevice
    {
    private:
        typedef set<ID3D9DeviceObject*>::type DeviceObjectList;

    public:
        D3D9RenderDevice();
        virtual ~D3D9RenderDevice();

    public:
        virtual void init( const RenderDeviceCreateContext& context );
        virtual void shutdown();

        virtual VertexBuffer*       createVertexBuffer();
        virtual VertexDeclaration*  createVertexDeclaration();
        virtual TextureObj*         createTextureObj();
        virtual SurfaceObj*         createSurfaceObj();
        virtual IndexBuffer*        createIndexBuffer();
        virtual Effect*             createEffect();

        virtual bool isTextureFormatSupported( TextureUsage usage, TextureType type, PixelFormat fmt );

        virtual void beginRender();
        virtual void endRender();
        
        virtual void _setRenderTarget( int idx, SurfaceObj* surTarget );
        virtual void _setDepthStencil( SurfaceObj* surDepthStencil );
        virtual void setViewport( const IntRect& rect );
        virtual void clear( uint32 flags, const Color* clr, float z, uint stencil );

        virtual void _setEffect( Effect* effect );
        virtual void setVertexBuffer( VertexBuffer* vb );
        virtual void setIndexBuffer( IndexBuffer* ib );
        virtual void drawIndexedPrimitive( PrimitiveType type, int startIndex, int primitiveCount );
        virtual void drawPrimitive( PrimitiveType type, int startVertex, int primitiveCount );

        virtual void enableColorWriteenable( int rtIdx, bool enR, bool enG, bool enB, bool enA );
        virtual void enableSRGBWrite( bool en );

        virtual void _enableZTest( bool en );
        virtual void _enableZWrite( bool en );
        virtual void _setZFunc( CmpFunc func );
        virtual void setDepthBias( float sca, float off );

        virtual void _enableStencil( bool en );

        virtual void _setStencil_refVal( uint8 refVal );
        virtual void _setStencil_cmpFunc( CmpFunc cmpFunc );
        virtual void _setStencil_cmpMask( uint8 cmpMask );
        virtual void _setStencil_writeMask( uint8 writeMask );

        virtual void _setStencil_stencilFailOp( StencilOp stencilFailOp );
        virtual void _setStencil_zFailOp( StencilOp zFailOp );
        virtual void _setStencil_bothPassOp( StencilOp bothPassOp );

        virtual void _enableBlend( bool en );
        virtual void _setBlendOp( BlendOp op );
        virtual void _setSrcBlend( BlendVal val );
        virtual void _setDestBlend( BlendVal val );

        virtual void _setCullMode( CullMode cm );
        virtual void _setWireframe( bool en );

        virtual void _setTexture( int sampler, TextureObj* texObj );

        virtual void _setMagFilter( int sampler, TextureFilter tf );
        virtual void _setMinFilter( int sampler, TextureFilter tf );
        virtual void _setMipFilter( int sampler, TextureFilter tf );

        virtual void _setTexAddrU( int sampler, TextureAddress addr );
        virtual void _setTexAddrV( int sampler, TextureAddress addr );
        virtual void _setTexAddrW( int sampler, TextureAddress addr );

        virtual void _setBorderColor( int sampler, const Color& clr );

        virtual void _setMipLodBias( int sampler, float mipLodBias );
        virtual void _setMipMaxLevel( int sampler, int mipMaxLevel );
        virtual void _setMaxAnisotropy( int sampler, int maxAnisotropy );

        virtual void _setSRGB( int sampler, bool en );

        virtual void toDeviceViewMatrix( Matrix4& matView );
        virtual void toDeviceProjMatrix( Matrix4& matProj );
        virtual bool isDepthAcceptSize( int depthWidth, int depthHeight, int rttWidth, int rttHeight );

        virtual void readSurfaceToCPU( SurfaceObj* surSrc, SurfaceObj* surDestOffscreen );

    public:
        LPDIRECT3DDEVICE9 _getD3DDevice() const { return m_d3DDevice; }

        const D3DPRESENT_PARAMETERS& _getPresentParas() const { return m_d3dpp; }

        void _registerDeviceObject( ID3D9DeviceObject* deviceObj );
        void _unregisterDeviceObject( ID3D9DeviceObject* deviceObj );
        
    private:
        void _onLostDevice();
        void _onResetDevice();
        bool _testCooperativeLevel();

        void _createRtAndDs();
        void _freeRtAndDs();
        void _getViewport();

    private:
        // device
        LPDIRECT3D9           m_d3D;
        LPDIRECT3DDEVICE9     m_d3DDevice;

        D3DDISPLAYMODE        m_d3ddm;
        D3DPRESENT_PARAMETERS m_d3dpp;
        D3DCAPS9              m_d3dCaps;

        // device objects that used default pool
        DeviceObjectList      m_deviceObjectList;

        // render
        D3D9VertexBuffer*   m_currDrawVB;
        D3D9IndexBuffer*    m_currDrawIB;
        D3DVIEWPORT9        m_currViewport;

        // flag
        bool m_lostDevice;
        bool m_callBeginScene;
    };
}

#endif

