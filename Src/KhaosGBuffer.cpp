#include "KhaosPreHeaders.h"
#include "KhaosGBuffer.h"
#include "KhaosRenderBufferPool.h"
#include "KhaosRenderDevice.h"
#include "KhaosCamera.h"
#include "KhaosRenderSystem.h"
#include "KhaosEffectID.h"
#include "KhaosImageProcess.h"
#include "KhaosEffectContext.h"
#include "KhaosEffectBuildStrategy.h"
#include "KhaosEffectSetters.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class DepthConverterBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id ) {}
        virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    KHAOS_DECL_SINGLE_SETTER(EffectDepthConvertSetter, "convInfo");

    KHAOS_BEGIN_DYNAMIC_DEFINE(DepthConverterBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(DepthConverterBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexInputSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectDepthConvertSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    void EffectDepthConvertSetter::doSet( EffSetterParams* params )
    {
        // D = zn / (zf + D * (zn-zf))
        Camera* cam = g_renderSystem->_getCurrCamera();
        
        Vector4 info;
        info.x = cam->getZNear();
        info.y = cam->getZFar();
        info.z = cam->getZNear() - cam->getZFar();

        m_param->setFloat4( info.ptr() );
    }
    //////////////////////////////////////////////////////////////////////////
    class ImageDepthConverter : public ImageProcess
    {
    public:
        ImageDepthConverter()
        {
            m_pin = _createImagePin( ET_DEPTHCONVERT );
            m_pin->setOwnOutputEx( 0 );
            m_pin->useMainDepth(true);
            _setRoot( m_pin );
        }

        void setOutput( TextureObj* texOut )
        {
            m_pin->getOutput()->linkRTT( texOut );
        }

        virtual void process( EffSetterParams& params )
        {
            TextureObj* texOut = g_renderDevice->getMainDepthStencil();
            m_pin->setInput( texOut );

            ImageProcess::process( params );
        }

    private:
        ImagePin*   m_pin;
    };

    //////////////////////////////////////////////////////////////////////////
    GBuffer::GBuffer() : m_depthLinearBuf(0), m_procConverter(0)
    {
    }

    GBuffer::~GBuffer()
    {
        clean();
    }

    void GBuffer::clean()
    {
        KHAOS_SAFE_DELETE( m_procConverter );
    }

    void GBuffer::init()
    {
        Viewport* vp = m_gBuf.createViewport(0);
        vp->setClearFlag( RCF_DEPTH|RCF_STENCIL ); // 不需要清理颜色，由天空来填充
        //vp->setClearColor( Color::BLACK ); // scene color black init, depth to 1, stencil to 0
    }

    void GBuffer::prepareResource( RenderBufferPool& renderBufferPool, TextureObj* sceneBuffer )
    {
        // base albedo | baked ao
        TextureObj* rttDiffuse = renderBufferPool.getRTTBuffer( PIXFMT_A8R8G8B8, 0, 0 );

        // metallic | specular | roughness | x
        TextureObj* rttSpecular = renderBufferPool.getRTTBuffer( PIXFMT_A8R8G8B8, 0, 0 );

        // normal.x | normal.y | mormal.z | shadow tmp
        TextureObj* rttNormal = renderBufferPool.getRTTBuffer( PIXFMT_A8R8G8B8, 0, 0 );

        // depth [0, 1] in view space
        m_depthLinearBuf = renderBufferPool.getRTTBuffer( PIXFMT_32F, 0, 0 );

        // depth surface
        SurfaceObj* surDepth = g_renderDevice->getMainDepthStencil()->getSurface(0);

        // link all
        m_gBuf.linkRTT( 0, rttDiffuse );
        m_gBuf.linkRTT( 1, rttSpecular );
        m_gBuf.linkRTT( 2, rttNormal );
        m_gBuf.linkRTT( 3, sceneBuffer );
        m_gBuf.linkDepth( surDepth );
    }

    TextureObj* GBuffer::getDiffuseBuffer()
    {
        return m_gBuf.getRTT( 0 );
    }

    TextureObj* GBuffer::getSpecularBuffer()
    {
        return m_gBuf.getRTT( 1 );
    }

    TextureObj* GBuffer::getNormalBuffer()
    {
        return m_gBuf.getRTT( 2 );
    }

    TextureObj* GBuffer::getDepthBuffer()
    {
        return m_depthLinearBuf;
    }

    void GBuffer::beginRender( Camera* currCamera )
    {
        m_gBuf.beginRender(0);

        Viewport* vpMain = m_gBuf.getViewport(0);
        vpMain->setRectByRtt(0);
        vpMain->linkCamera( currCamera );
        vpMain->apply();
    }

    void GBuffer::endRender( EffSetterParams& params )
    {
        m_gBuf.endRender();
        _convertDepth( params );
    }

    void GBuffer::_convertDepth( EffSetterParams& params )
    {
        if ( !m_procConverter )
        {
            _registerEffectVSPS2( ET_DEPTHCONVERT, "commDrawScreen", "depthConvert", 
                DepthConverterBuildStrategy );

            m_procConverter = KHAOS_NEW ImageDepthConverter;
        }
        
        ImageDepthConverter* proc = static_cast<ImageDepthConverter*>(m_procConverter);
        proc->setOutput( m_depthLinearBuf );
        proc->process( params );
    }
}

