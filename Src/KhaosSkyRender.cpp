#include "KhaosPreHeaders.h"
#include "KhaosSkyRender.h"
#include "KhaosRenderSystem.h"
#include "KhaosSceneGraph.h"
#include "KhaosRenderDevice.h"
#include "KhaosEffectID.h"
#include "KhaosImageProcess.h"
#include "KhaosEffectContext.h"
#include "KhaosEffectBuildStrategy.h"
#include "KhaosEffectSetters.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class SkySimpleRenderBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id ) {}
        virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    KHAOS_DECL_SINGLE_SETTER(EffectSimpleSkyColorSetter, "simpleSkyColor");

    KHAOS_BEGIN_DYNAMIC_DEFINE(SkySimpleRenderBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(SkySimpleRenderBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectSimpleSkyColorSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    void EffectSimpleSkyColorSetter::doSet( EffSetterParams* params )
    {
        const Color& clr = g_skyRender->_getCurrentSimpleColor();
        m_param->setFloat4( clr.ptr() );
    }

    //////////////////////////////////////////////////////////////////////////
    class ImgProcSkySimpleColor : public ImageProcess
    {
    public:
        ImgProcSkySimpleColor()
        {
            m_pin = _createImagePin( ET_SKYSIMPLECLR );
            m_pin->setOwnOutputEx( 0 );
            m_pin->useMainDepth(true);
            _setRoot( m_pin );
        }

        virtual void process( EffSetterParams& params )
        {
            TextureObj* sceneBuff = g_renderSystem->_getCurrSceneBuf();
            m_pin->getOutput()->linkRTT( sceneBuff );
            ImageProcess::process( params );
        }

    private:
        ImagePin*   m_pin;
    };

    //////////////////////////////////////////////////////////////////////////
    SkyRender* g_skyRender = 0;

    SkyRender::SkyRender() : m_simpleRender(0)
    {
        khaosAssert(!g_skyRender);
        g_skyRender = this;
    }

    SkyRender::~SkyRender()
    {
        KHAOS_DELETE m_simpleRender;
        g_skyRender = 0;
    }

    void SkyRender::init()
    {
    }

    void SkyRender::renderBackground( EffSetterParams& params )
    {
        g_renderSystem->_setDrawNot( RenderSystem::RK_OPACITY ); // ����ֻ�����

        SkyEnv& skyEnv = g_renderSystem->_getCurrSceneGraph()->getSkyEnv();

        // ����ղ�
        if ( skyEnv.getSkyLayersCount() > 0 )
        {
            for ( int i = 0; i < skyEnv.getSkyLayersCount(); ++i )
            {
                // ������Ⱦÿ����
                SkyLayer* layer = skyEnv.getSkyLayer(i);

                // �趨���ģʽ
                bool isBlend = layer->isBlendEnabled();
                if ( i == 0 ) // ��0����Ϊ�ײ㱳����ǿ�Ʋ����ģʽ
                    isBlend = false;

                if ( isBlend )
                    g_renderDevice->setBlendStateSet( BlendStateSet::ADD );
                else
                    g_renderDevice->setBlendStateSet( BlendStateSet::REPLACE );

                // ������������Ⱦ
                switch ( layer->getType() )
                {
                case SkyLayer::SIMPLE_COLOR:
                    _renderSimple( params, layer );
                    break;
                }
            }
        }
        else // û�У�����ʹ��Ĭ�ϲ�
        {
            g_renderDevice->setBlendStateSet( BlendStateSet::REPLACE );

            SkyLayer layerDefaut;
            layerDefaut.setType( SkyLayer::SIMPLE_COLOR );
            layerDefaut.setColor( g_backColor );
            layerDefaut.enableBlend( false );

            _renderSimple( params, &layerDefaut );
        }

        g_renderSystem->_unsetDrawOnly();
    }

    void SkyRender::_renderSimple( EffSetterParams& params, SkyLayer* layer )
    {
        if ( !m_simpleRender )
        {
            // ��ʼ������
            _registerEffectVSPS2( ET_SKYSIMPLECLR, "commDrawScreen", "skySimpleRender", 
                SkySimpleRenderBuildStrategy );

            m_simpleRender = KHAOS_NEW ImgProcSkySimpleColor;
        }

        m_simpleClr = layer->getColor();
        ImgProcSkySimpleColor* proc = static_cast<ImgProcSkySimpleColor*>(m_simpleRender);
        proc->process( params );
    }
}

