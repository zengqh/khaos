#pragma once
#include "KhaosEffectContext.h"
#include "KhaosNameDef.h"

namespace Khaos
{
    class UniformPacker;

    //////////////////////////////////////////////////////////////////////////
#define KHAOS_DECL_SINGLE_SETTER(cls, var) \
    class cls : public EffectSetterBase \
    { \
        KHAOS_DECLARE_RTTI(cls) \
    public: \
        cls() : EffectSetterBase(), m_param(0) {} \
        virtual void init( EffectContext* context ) \
        { \
            EffectSetterBase::init( context ); \
            m_param = context->getEffect()->getParam(var); \
        } \
        virtual void doSet( EffSetterParams* params ); \
    private: \
        ShaderParamEx* m_param; \
    };

    //////////////////////////////////////////////////////////////////////////
#define KHAOS_DECL_TEXSETTER(cls, var) \
    class cls : public EffectSetterBase \
    { \
        KHAOS_DECLARE_RTTI(cls) \
    public: \
        cls() : EffectSetterBase(), m_tex(0) {} \
        virtual void init( EffectContext* context ) \
        { \
            EffectSetterBase::init( context ); \
            m_tex = context->getEffect()->getParam(var); \
        } \
        virtual void doSet( EffSetterParams* params ) \
        { \
            m_tex->setTextureSamplerState( _getTex(params) ); \
        } \
    private: \
        Texture* _getTex( EffSetterParams* params ); \
        ShaderParamEx* m_tex; \
    };

    //////////////////////////////////////////////////////////////////////////
#define KHAOS_BEGIN_CLASS_EFFECT_SETTER_EX(x, y) \
    class x : public y \
    { \
        KHAOS_DECLARE_RTTI(x) \
        virtual void doSet( EffSetterParams* params ); \
        virtual void init( EffectContext* context ) \
        { \
            EffectSetterBase::init( context );

#define KHAOS_BEGIN_CLASS_EFFECT_SETTER(x) \
    KHAOS_BEGIN_CLASS_EFFECT_SETTER_EX(x, EffectSetterBase)

#define KHAOS_MAP_SHADERPARAM(v, n) \
            v = context->getEffect()->getParam(n);

#define KHAOS_DECL_SHADERPARAM(x) \
        } \
        ShaderParamEx* x; void __dumy_func_##x() {

#define KHAOS_DECL_MISC_BEGIN() }

#define KHAOS_DECL_MISC_END() void __dumy_misc() {

#define KHAOS_END_CLASS_EFFECT_SETTER() } \
    };


    //////////////////////////////////////////////////////////////////////////
    KHAOS_DECL_SINGLE_SETTER(EffectSurfaceParamsSetter, NAME_SURFACE_PARAMS);
    KHAOS_DECL_SINGLE_SETTER(EffectSpecularMaskParamsSetter, NAME_SPECULARMASK_PARAMS);
    KHAOS_DECL_SINGLE_SETTER(EffectAlphaTestSetter, NAME_MTR_ALPHAREF);

    KHAOS_DECL_TEXSETTER(EffectBaseMapSetter, NAME_MAP_BASE);
    KHAOS_DECL_TEXSETTER(EffectSpecularMapSetter, NAME_MAP_SPECULAR);
    KHAOS_DECL_TEXSETTER(EffectEmissiveMapSetter, NAME_MAP_EMISSIVE);
    KHAOS_DECL_TEXSETTER(EffectNormMapSetter, NAME_MAP_BUMP);
    KHAOS_DECL_TEXSETTER(EffectOpacityMapSetter, NAME_MAP_OPACITY);
    KHAOS_DECL_TEXSETTER(EffectBakedAOMapSetter, NAME_MAP_BAKEDAO);

    //////////////////////////////////////////////////////////////////////////
    KHAOS_DECL_TEXSETTER(EffectLitMapSetter, NAME_MAP_LIGHT);

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectFullLitMapSetter)
        KHAOS_MAP_SHADERPARAM(m_litMapA, NAME_MAP_LIGHT)
        KHAOS_MAP_SHADERPARAM(m_litMapB, NAME_MAP_LIGHTB)
        KHAOS_DECL_SHADERPARAM(m_litMapA)
        KHAOS_DECL_SHADERPARAM(m_litMapB)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectEnvDiffStaticSetter)
        KHAOS_MAP_SHADERPARAM(m_envMapDiff, NAME_MAP_ENVDIFF)
        KHAOS_DECL_SHADERPARAM(m_envMapDiff)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectEnvSpecStaticSetter)
        KHAOS_MAP_SHADERPARAM(m_envMapSpec, NAME_MAP_ENVSPEC)
        KHAOS_MAP_SHADERPARAM(m_mapInfoA, NAME_ENVMAPINFOA)
        KHAOS_DECL_SHADERPARAM(m_envMapSpec)
        KHAOS_DECL_SHADERPARAM(m_mapInfoA)
    KHAOS_END_CLASS_EFFECT_SETTER()

    typedef EffectEnvSpecStaticSetter EffectEnvSpecDynamicSetter; // 当前dynamic和static spec env map 处理一致

    //////////////////////////////////////////////////////////////////////////
    KHAOS_DECL_SINGLE_SETTER(EffectAmbParamsSetter, NAME_AMBPARAS);

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectDirLitSetter)
        KHAOS_MAP_SHADERPARAM(m_dirLits, NAME_DIRLITS)
        KHAOS_DECL_SHADERPARAM(m_dirLits)
    KHAOS_END_CLASS_EFFECT_SETTER()   

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectDirLitShadowDeferUseSetter)
        KHAOS_MAP_SHADERPARAM(m_shadowMap0, NAME_MAP_NORMAL)
        KHAOS_DECL_SHADERPARAM(m_shadowMap0)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectDirLitShadowMaskSetter)
        KHAOS_MAP_SHADERPARAM(m_matShadowProj, NAME_MATSHADOWPROJ)
        KHAOS_MAP_SHADERPARAM(m_shadowParams, NAME_SHADOWPARAMS)
        KHAOS_MAP_SHADERPARAM(m_shadowMap0, NAME_MAPSHADOW0)
        KHAOS_MAP_SHADERPARAM(m_shadowMap1, NAME_MAPSHADOW1)
        KHAOS_MAP_SHADERPARAM(m_shadowMap2, NAME_MAPSHADOW2)
        KHAOS_MAP_SHADERPARAM(m_shadowMap3, NAME_MAPSHADOW3)
        KHAOS_DECL_SHADERPARAM(m_matShadowProj)
        KHAOS_DECL_SHADERPARAM(m_shadowParams)
        KHAOS_DECL_SHADERPARAM(m_shadowMap0)
        KHAOS_DECL_SHADERPARAM(m_shadowMap1)
        KHAOS_DECL_SHADERPARAM(m_shadowMap2)
        KHAOS_DECL_SHADERPARAM(m_shadowMap3)
        KHAOS_DECL_MISC_BEGIN()
            EffectDirLitShadowMaskSetter(int flag) : m_flag(flag) {}
            int m_flag;
        KHAOS_DECL_MISC_END()
    KHAOS_END_CLASS_EFFECT_SETTER()   

    class EffectLitSetterBase : public EffectSetterBase
    {
    protected:
        static const int POINT_PACK_FLOATS;
        static const int SPOT_PACK_FLOATS;

        void _packPoint( UniformPacker& pack, Light* lit );
        void _packSpot( UniformPacker& pack, Light* lit );
    };

    KHAOS_BEGIN_CLASS_EFFECT_SETTER_EX(EffectLitSetter, EffectLitSetterBase)
        KHAOS_MAP_SHADERPARAM(m_pointLits, NAME_POINTLITS)
        KHAOS_MAP_SHADERPARAM(m_spotLits, NAME_SPOTLITS)
        KHAOS_MAP_SHADERPARAM(m_pointLitsCount, NAME_POINTLITS_COUNT)
        KHAOS_MAP_SHADERPARAM(m_spotLitsCount, NAME_SPOTLITS_COUNT)
        KHAOS_DECL_SHADERPARAM(m_pointLits)
        KHAOS_DECL_SHADERPARAM(m_spotLits)
        KHAOS_DECL_SHADERPARAM(m_pointLitsCount)
        KHAOS_DECL_SHADERPARAM(m_spotLitsCount)
        KHAOS_DECL_MISC_BEGIN()
            void _checkPointLit( EffSetterParams* params );
            void _checkSpotLit( EffSetterParams* params );
        KHAOS_DECL_MISC_END()
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER_EX(EffectLitAccSetter, EffectLitSetterBase)
        KHAOS_MAP_SHADERPARAM(m_pointLits, NAME_POINTLITS)
        KHAOS_MAP_SHADERPARAM(m_spotLits, NAME_SPOTLITS)
        KHAOS_DECL_SHADERPARAM(m_pointLits)
        KHAOS_DECL_SHADERPARAM(m_spotLits)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectVolProbAccSetter)
        KHAOS_MAP_SHADERPARAM(m_matWorldToVol, NAME_MATWORLDTOVOL)
        KHAOS_MAP_SHADERPARAM(m_mapVolR, NAME_MAP_VOLR)
        KHAOS_MAP_SHADERPARAM(m_mapVolG, NAME_MAP_VOLG)
        KHAOS_MAP_SHADERPARAM(m_mapVolB, NAME_MAP_VOLB)
        KHAOS_DECL_SHADERPARAM(m_matWorldToVol)
        KHAOS_DECL_SHADERPARAM(m_mapVolR)
        KHAOS_DECL_SHADERPARAM(m_mapVolG)
        KHAOS_DECL_SHADERPARAM(m_mapVolB)
    KHAOS_END_CLASS_EFFECT_SETTER()   

    //////////////////////////////////////////////////////////////////////////
    KHAOS_DECL_SINGLE_SETTER(EffectMatWorldSetter, NAME_MATWORLD);
    KHAOS_DECL_SINGLE_SETTER(EffectMatWVPSetter, NAME_MATWVP);
    KHAOS_DECL_SINGLE_SETTER(EffectMatViewProjPreSetter, NAME_MATVIEWPROJPRE);
    
    //////////////////////////////////////////////////////////////////////////
    KHAOS_DECL_SINGLE_SETTER(EffectCameraPosWorldSetter, NAME_CAMERAPOSWORLD);
    KHAOS_DECL_SINGLE_SETTER(EffectZFarSetter, NAME_CAMERAZFAR);
    KHAOS_DECL_SINGLE_SETTER(EffectZFarFovSetter, NAME_CAMERAZFARFOV);
    KHAOS_DECL_SINGLE_SETTER(EffectCamInfoSetter, NAME_CAMERAINFO);
    KHAOS_DECL_SINGLE_SETTER(EffectTargetInfoSetter, NAME_TARGETINFO);
    KHAOS_DECL_SINGLE_SETTER(EffectTargetInfoExSetter, NAME_TARGETINFOEX);
    KHAOS_DECL_SINGLE_SETTER(EffectScreenScaleSetter, NAME_SCREENSCALE);

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectCamVolBiasSetter)
        KHAOS_MAP_SHADERPARAM(m_camVolBias, NAME_CAMVOLBIAS)
        KHAOS_DECL_SHADERPARAM(m_camVolBias)
        KHAOS_DECL_MISC_BEGIN()
            EffectCamVolBiasSetter(int flag) : m_flag(flag) {}
            int m_flag;
        KHAOS_DECL_MISC_END()
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectCamJitterParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_jitterWorld, NAME_JITTER_WORLD)
        KHAOS_MAP_SHADERPARAM(m_jitterUV, NAME_JITTER_UV)
        KHAOS_DECL_SHADERPARAM(m_jitterWorld)
        KHAOS_DECL_SHADERPARAM(m_jitterUV)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_DECL_SINGLE_SETTER(EffectGammaSetter, NAME_GAMMA_VALUE);
    KHAOS_DECL_SINGLE_SETTER(EffectGammaAdjSetter, NAME_GAMMA_VALUE);

    //////////////////////////////////////////////////////////////////////////
    KHAOS_DECL_SINGLE_SETTER(EffectPoissonDiskSetter, NAME_POISSON_DISK);
    KHAOS_DECL_TEXSETTER(EffectTexEnvLUTSetter, NAME_MAP_ENVLUT);
    KHAOS_DECL_TEXSETTER(EffectTexRandomSetter, NAME_MAP_RANDOM);
    KHAOS_DECL_TEXSETTER(EffectTexNormalFittingSetter, NAME_MAP_NORMALFITTING);

    KHAOS_DECL_TEXSETTER(EffectGBufDiffuseSetter, NAME_GBUF_DIFFUSE);
    KHAOS_DECL_TEXSETTER(EffectGBufSpecularSetter, NAME_GBUF_SPECULAR);
    KHAOS_DECL_TEXSETTER(EffectTexDepthSetter, NAME_MAP_DEPTH);
    KHAOS_DECL_TEXSETTER(EffectTexDepthHalfSetter, NAME_MAP_DEPTHHALF);
    KHAOS_DECL_TEXSETTER(EffectTexDepthQuarterSetter, NAME_MAP_DEPTHQUARTER);
    KHAOS_DECL_TEXSETTER(EffectTexNormalSetter, NAME_MAP_NORMAL);
    KHAOS_DECL_TEXSETTER(EffectTexAOSetter, NAME_MAP_AO);
    KHAOS_DECL_TEXSETTER(EffectTexInputSetter, NAME_MAP_INPUT);

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_CLASS_EFFECT_SETTER(CommScaleParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_scaleParams, NAME_COMMSCALE_PARAMS)
        KHAOS_DECL_SHADERPARAM(m_scaleParams)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_DECL_SINGLE_SETTER(CommBlurTexOffsetsSetter, NAME_COMMBLUR_TEXOFFSETS);
    KHAOS_DECL_SINGLE_SETTER(CommBlurPSWeightsSetter, NAME_COMMBLUR_PSWEIGHTS);

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(DownscaleZParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_paramA, NAME_DOWNSCALEZ_PARAMA)
        KHAOS_MAP_SHADERPARAM(m_paramB, NAME_DOWNSCALEZ_PARAMB)
        KHAOS_DECL_SHADERPARAM(m_paramA)
        KHAOS_DECL_SHADERPARAM(m_paramB)
    KHAOS_END_CLASS_EFFECT_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_CLASS_EFFECT_SETTER(SSAOParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_param1, NAME_COMM_PARAMA)
        KHAOS_MAP_SHADERPARAM(m_param2, NAME_COMM_PARAMB)
        KHAOS_MAP_SHADERPARAM(m_param3, NAME_COMM_PARAMC)
        KHAOS_DECL_SHADERPARAM(m_param1)
        KHAOS_DECL_SHADERPARAM(m_param2)
        KHAOS_DECL_SHADERPARAM(m_param3)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(SSAOFilterSampleOffsetsSetter)
        KHAOS_MAP_SHADERPARAM(m_param1, NAME_COMM_PARAMA)
        KHAOS_MAP_SHADERPARAM(m_param2, NAME_COMM_PARAMB)
        KHAOS_DECL_SHADERPARAM(m_param1)
        KHAOS_DECL_SHADERPARAM(m_param2)
    KHAOS_END_CLASS_EFFECT_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_CLASS_EFFECT_SETTER(InitLumParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_offsetParams, NAME_CALCLUM_PARAMS)
        KHAOS_DECL_SHADERPARAM(m_offsetParams)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(LumIterParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_offsetParams, NAME_CALCLUM_PARAMS)
        KHAOS_DECL_SHADERPARAM(m_offsetParams)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(AdaptedLumParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_offsetParams, NAME_CALCLUM_PARAMS)
        KHAOS_MAP_SHADERPARAM(m_lum0, NAME_MAP_LUM0)
        KHAOS_MAP_SHADERPARAM(m_lum1, NAME_MAP_LUM1)
        KHAOS_DECL_SHADERPARAM(m_lum0)
        KHAOS_DECL_SHADERPARAM(m_lum1)
        KHAOS_DECL_SHADERPARAM(m_offsetParams)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(BrightPassParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_target, NAME_MAP_TARGETQUARTER)
        KHAOS_MAP_SHADERPARAM(m_lum1, NAME_MAP_LUM1)
        KHAOS_DECL_SHADERPARAM(m_target)
        KHAOS_DECL_SHADERPARAM(m_lum1)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(FlaresPassParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_target_4, NAME_MAP_TARGETQUARTER)
        KHAOS_MAP_SHADERPARAM(m_target_8, NAME_MAP_TARGETEIGHTH)
        KHAOS_MAP_SHADERPARAM(m_target_16, NAME_MAP_TARGETSIXTEENTH)
        KHAOS_DECL_SHADERPARAM(m_target_4)
        KHAOS_DECL_SHADERPARAM(m_target_8)
        KHAOS_DECL_SHADERPARAM(m_target_16)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(HDRParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_param0, NAME_HDRParams0)
        KHAOS_MAP_SHADERPARAM(m_param1, NAME_HDRParams1)
        KHAOS_MAP_SHADERPARAM(m_param5, NAME_HDRParams5)
        KHAOS_MAP_SHADERPARAM(m_param8, NAME_HDRParams8)
        KHAOS_DECL_SHADERPARAM(m_param0)
        KHAOS_DECL_SHADERPARAM(m_param1)
        KHAOS_DECL_SHADERPARAM(m_param5)
        KHAOS_DECL_SHADERPARAM(m_param8)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(HDRFinalPassParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_targetFull, NAME_MAP_TARGETFULL)
        KHAOS_MAP_SHADERPARAM(m_lum1, NAME_MAP_LUM1)
        KHAOS_MAP_SHADERPARAM(m_bloom, NAME_MAP_TARGETQUARTER)
        //KHAOS_MAP_SHADERPARAM(m_viewProjPrev, NAME_MATVIEWPROJPRE)
        KHAOS_DECL_SHADERPARAM(m_targetFull)
        KHAOS_DECL_SHADERPARAM(m_lum1)
        KHAOS_DECL_SHADERPARAM(m_bloom)
        //KHAOS_DECL_SHADERPARAM(m_viewProjPrev)
    KHAOS_END_CLASS_EFFECT_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectAABlendParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_area, NAME_MAP_AREA)
        KHAOS_MAP_SHADERPARAM(m_search, NAME_MAP_SEARCH)
        KHAOS_MAP_SHADERPARAM(m_subIdx, NAME_COMM_PARAMA);
        KHAOS_DECL_SHADERPARAM(m_area)
        KHAOS_DECL_SHADERPARAM(m_search)
        KHAOS_DECL_SHADERPARAM(m_subIdx)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectAAFinalParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_color, NAME_MAP_COLOR)
        KHAOS_MAP_SHADERPARAM(m_blend, NAME_MAP_BLEND)
        KHAOS_DECL_SHADERPARAM(m_color)
        KHAOS_DECL_SHADERPARAM(m_blend)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectAAFinal2ParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_color, NAME_MAP_COLOR)
        KHAOS_DECL_SHADERPARAM(m_color)
    KHAOS_END_CLASS_EFFECT_SETTER()

    KHAOS_BEGIN_CLASS_EFFECT_SETTER(EffectAATempParamsSetter)
        KHAOS_MAP_SHADERPARAM(m_frameCurr, NAME_MAP_FRAMECURR)
        KHAOS_MAP_SHADERPARAM(m_framePre, NAME_MAP_FRAMEPRE)
        KHAOS_MAP_SHADERPARAM(m_viewProjPrev, NAME_MATVIEWPROJPRE)
        KHAOS_DECL_SHADERPARAM(m_frameCurr)
        KHAOS_DECL_SHADERPARAM(m_framePre)
        KHAOS_DECL_SHADERPARAM(m_viewProjPrev)
    KHAOS_END_CLASS_EFFECT_SETTER()

    //////////////////////////////////////////////////////////////////////////
#define KHAOS_EFFSTATUS_SETTER(name) \
    class name : public EffectSetterBase \
    { \
        KHAOS_DECLARE_RTTI(name) \
    public: \
        virtual void doSet( EffSetterParams* params ); \
    };

    KHAOS_EFFSTATUS_SETTER(EffectEmptyStateSetter)
    KHAOS_EFFSTATUS_SETTER(EffectCommStateSetter)
    KHAOS_EFFSTATUS_SETTER(EffectDeferPreStateSetter)
    KHAOS_EFFSTATUS_SETTER(EffectShadowPreStateSetter)
    KHAOS_EFFSTATUS_SETTER(EffectPostStateSetter)
    KHAOS_EFFSTATUS_SETTER(EffectPostBlendAddStateSetter)
}

