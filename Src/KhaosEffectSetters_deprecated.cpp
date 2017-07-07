#include "KhaosPreHeaders.h"
#include "KhaosEffectSetters.h"

#include "KhaosRenderDevice.h"

#include "KhaosCamera.h"
#include "KhaosLightNode.h"
#include "KhaosSysResManager.h"

#include "KhaosRenderable.h"
#include "KhaosUniformPacker.h"
#include "KhaosRenderSystem.h"

#include "KhaosImageProcess.h"
#include "KhaosImageProcessShadow.h"
#include "KhaosImageProcessUtil.h"
#include "KhaosImageProcessHDR.h"
#include "KhaosImageProcessAA.h"

#include "KhaosSampleUtil.h"

namespace Khaos
{
#if 0
    //////////////////////////////////////////////////////////////////////////
    KHAOS_DECL_SINGLE_SETTER(EffectDiffuseSetter, NAME_MTR_DIFFUSEA);
    KHAOS_DECL_SINGLE_SETTER(EffectSpecularSetter, NAME_MTR_SPECULAR);
    KHAOS_DECL_SINGLE_SETTER(EffectSpecularPreSetter, NAME_MTR_SPECULAR);
    KHAOS_DECL_SINGLE_SETTER(EffectEmissiveSetter, NAME_MTR_EMISSIVE);

    KHAOS_DECL_TEXSETTER(EffectDiffMapSetter, NAME_MAP_DIFFUSE);

    KHAOS_DECL_SINGLE_SETTER(EffectMatWorldViewSetter, NAME_MATWORLDVIEW);
    KHAOS_DECL_SINGLE_SETTER(EffectMatViewToTexSetter, NAME_MATVIEWTOTEX);
    KHAOS_DECL_SINGLE_SETTER(EffectMatViewProjectInvSetter, NAME_MATVIEWPROJINV);
    KHAOS_DECL_SINGLE_SETTER(EffectMatProjectInvSetter, NAME_MATPROJINV);

    KHAOS_DECL_SINGLE_SETTER(EffectRandomBRepeatSetter, NAME_RANDOM_REPEAT);
    KHAOS_DECL_TEXSETTER(EffectTexRandomBSetter, NAME_MAP_RANDOM);

    //////////////////////////////////////////////////////////////////////////
    void EffectDiffuseSetter::doSet( EffSetterParams* params )
    {
        // diffuse & alpha
        Material* mtr = params->_mtr;
        const Color& diff = mtr->getAttrib<DiffuseAttrib>()->getDiffuse();
        m_param->setFloat4( diff.ptr() );
    }

    void EffectSpecularSetter::doSet( EffSetterParams* params )
    {
        // specular & shininess
        Material* mtr = params->_mtr;
        Color spec = mtr->getAttrib<SpecularAttrib>()->getSpecularShininess();
        khaosAssert( 0 <= spec.a && spec.a <= 1 );
        m_param->setFloat4( spec.ptr() );
    }

    void EffectSpecularPreSetter::doSet( EffSetterParams* params )
    {
        // only shininess
        Material* mtr = params->_mtr;

        if ( SpecularAttrib* attr = static_cast<SpecularAttrib*>( mtr->isAttribEnabled(MA_SPECULAR) ) )
        {
            Color spec = mtr->getAttrib<SpecularAttrib>()->getSpecularShininess();
            khaosAssert( 0 <= spec.a && spec.a <= 1 );
            m_param->setFloat4( spec.ptr() );
        }
        else
        {
            static const Vector4 nullspec(0,0,0,-1);
            m_param->setFloat4( nullspec.ptr() );
        }
    }

    void EffectEmissiveSetter::doSet( EffSetterParams* params )
    {
        // emissive
        m_param->setFloat3( params->_mtr->getAttrib<EmissiveAttrib>()->getEmissive().ptr() );
    }

    Texture* EffectDiffMapSetter::_getTex( EffSetterParams* params )
    {
        return params->_mtr->getAttrib<DiffuseMapAttrib>()->getTexture();
    }

    //////////////////////////////////////////////////////////////////////////
    void EffectAmbientLightSetter::doSet( EffSetterParams* params )
    {
        RenderSettings* curSettings = g_renderSystem->_getMainCamera()->getRenderSettings();

        const Color* clr = &Color::BLACK;

        if ( curSettings->isSettingEnabled<AmbientSetting>() )
            clr = &curSettings->getSetting<AmbientSetting>()->getAmbient();

        m_param->setFloat3( clr->ptr() );
    }

    //////////////////////////////////////////////////////////////////////////
    void EffectMatWorldViewSetter::doSet( EffSetterParams* params )
    {
        // mat world view
        const Matrix4& matWorld = params->ra->getImmMatWorld();
        Matrix4 matWV = g_renderSystem->_getCurrCamera()->getViewMatrixRD() * matWorld;
        m_param->setMatrix4( matWV );
    }

    void EffectMatViewToTexSetter::doSet( EffSetterParams* params )
    {
        // mat view to texture
        Matrix4 projToTex;
        Camera* cam = g_renderSystem->_getCurrCamera();
        g_renderDevice->matProjToTex( projToTex, 0, cam->getViewportWidth(), cam->getViewportHeight() );
        Matrix4 matViewToTex = projToTex * cam->getProjMatrixRD();
        m_param->setMatrix4( matViewToTex );
    }

    void EffectMatViewProjectInvSetter::doSet( EffSetterParams* params )
    {
        // mat view project inv
        //Matrix4 mat = g_renderSystem->_getCurrCamera()->getViewProjMatrixRD().inverse();

        // no jitter here
        khaosAssert( g_renderSystem->_getCurrCamera() == g_renderSystem->_getMainCamera() );

        Matrix4 matProj = g_renderSystem->_getCurrCamera()->getProjMatrix();
        g_renderDevice->toDeviceProjMatrix( matProj );

        matProj = matProj * g_renderSystem->_getCurrCamera()->getViewMatrixRD();
        m_param->setMatrix4( matProj.inverse() );
    }

    void EffectMatProjectInvSetter::doSet( EffSetterParams* params )
    {
        // mat project inv
        Matrix4 mat = g_renderSystem->_getCurrCamera()->getProjMatrixRD().inverse();
        m_param->setMatrix4( mat );
    }

    //////////////////////////////////////////////////////////////////////////
    void EffectRandomBRepeatSetter::doSet( EffSetterParams* params )
    {
        Camera* cam = g_renderSystem->_getCurrCamera();
        Texture* texRand = g_sysResManager->getTexRandomB();

        Vector2 info( cam->getViewportWidth() / (float)texRand->getWidth(),
            cam->getViewportHeight() / (float)texRand->getHeight() );
        m_param->setFloat2( info.ptr() );
    }
    
    Texture* EffectTexRandomBSetter::_getTex( EffSetterParams* params )
    {
        return g_sysResManager->getTexRandomB();
    }

#endif
}

