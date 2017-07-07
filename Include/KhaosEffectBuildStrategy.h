#pragma once
#include "KhaosEffectContext.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    // DefaultBuildStrategy
    class DefaultBuildStrategy : public IEffectBuildStrategy, public AllocatedObject
    {
    public:
        virtual ~DefaultBuildStrategy() {}

        virtual void   release();
        virtual void   calcEffectID( Renderable* ra, EffectID& id );
        virtual void   calcInferID( const EffectID& id, EffectID& inferId  );
        virtual String makeSystemDefine();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void   installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );

    protected:
        static Material::ShaderID _makeMtrShaderID( Material* mtr, 
            bool onlyAlphaTest, bool disableNormalMap );

        void _addMacro( String& strMacro, pcstr macName );
        void _addMacro( String& strMacro, pcstr macName, int macVal );
    };

#define KHAOS_BEGIN_DYNAMIC_DEFINE(cls) \
    String cls::makeDynamicDefine( const EffectID& id, const EffectID& inferId ) \
    { \
        String strMacro;

#define KHAOS_CALL_DEFAULT_DYNAMIC_DEFINE() \
    strMacro += DefaultBuildStrategy::makeDynamicDefine(id, inferId);

#define KHAOS_TEST_EFFECT_FLAG(x) \
        if ( id.testFlag(EffectID::x) ) \
            _addMacro( strMacro, #x );

#define KHAOS_TEST_EFFECT_SELECT(x, mac, v1, v2) \
        if ( id.testFlag(EffectID::x) ) \
            _addMacro( strMacro, #mac, v1 ); \
        else \
            _addMacro( strMacro, #mac, v2 );

#define KHAOS_TEST_EFFECT_VALUE_OFFSET(x, mac, mask, offset) \
        if ( id.testFlag(EffectID::x) ) \
            _addMacro( strMacro, #mac, (int)id.fetchValue(EffectID::mask, EffectID::offset) );

#define KHAOS_ALWAYS_EFFECT_VALUE_OFFSET(mac, mask, offset) \
        _addMacro( strMacro, #mac, (int)id.fetchValue(EffectID::mask, EffectID::offset) );

#define KHAOS_ALWAYS_EFFECT_VALUE(mac, val) \
    _addMacro( strMacro, #mac, (int)(val) );

#define KHAOS_TEST_INFER_FLAG(x) \
        if ( inferId.testFlag(EffectID::x) ) \
            _addMacro( strMacro, #x );

#define KHAOS_END_DYNAMIC_DEFINE() \
        return strMacro; \
    }

#define KHAOS_BEGIN_INSTALL_SETTER(cls) \
    void cls::installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId ) \
    {

#define KHAOS_DERIVED_INSTALL_SETTER(base) \
        base::installEffectSetter( ec, id, inferId );

#define KHAOS_ALWAYS_BIND_SETTER(type, setter) \
        ec->addSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_ALWAYS_BIND_SETTER_REPLACE(type, setter) \
        ec->replaceSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_EFFECT_SETTER(val, type, setter) \
        if ( id.testFlag( EffectID::val ) ) \
            ec->addSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_EFFECT_SETTER_2(val1, val2, type, setter) \
        if ( id.testFlag( EffectID::val1 | EffectID::val2 ) ) \
            ec->addSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_EFFECT_SETTER_REPLACE(val, type, setter) \
        if ( id.testFlag( EffectID::val ) ) \
            ec->replaceSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_EFFECT_SETTER_DEFER(val2, type, setter) \
        if ( id.testFlag(EffectID::HAS_DEFERRED) && id.testFlag(EffectID::val2) ) \
            ec->addSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_EFFECT_SETTER_DEFER_REPLACE(val2, type, setter) \
        if ( id.testFlag(EffectID::HAS_DEFERRED) && id.testFlag(EffectID::val2) ) \
            ec->replaceSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_EFFECT_SETTER_FORWARD(val2, type, setter) \
        if ( !id.testFlag(EffectID::HAS_DEFERRED) && id.testFlag(EffectID::val2) ) \
            ec->addSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_EFFECT_SETTER_FORWARD_REPLACE(val2, type, setter) \
        if ( !id.testFlag(EffectID::HAS_DEFERRED) && id.testFlag(EffectID::val2) ) \
            ec->replaceSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_INFER_SETTER(val, type, setter) \
        if ( inferId.testFlag( EffectID::val ) ) \
            ec->addSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_INFER_SETTER_REPLACE(val, type, setter) \
        if ( inferId.testFlag( EffectID::val ) ) \
            ec->replaceSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_INFER_SETTER_DEFER(val2, type, setter) \
        if ( id.testFlag(EffectID::HAS_DEFERRED) && inferId.testFlag(EffectID::val2) ) \
            ec->addSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_INFER_SETTER_DEFER_REPLACE(val2, type, setter) \
        if ( id.testFlag(EffectID::HAS_DEFERRED) && inferId.testFlag(EffectID::val2) ) \
            ec->replaceSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_INFER_SETTER_FORWARD(val2, type, setter) \
        if ( !id.testFlag(EffectID::HAS_DEFERRED) && inferId.testFlag(EffectID::val2) ) \
            ec->addSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_INFER_SETTER_FORWARD_REPLACE(val2, type, setter) \
        if ( !id.testFlag(EffectID::HAS_DEFERRED) && inferId.testFlag(EffectID::val2) ) \
            ec->replaceSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define _OIDHAS(x) id.testFlag(EffectID::x)
#define _IIDHAS(x) inferId.testFlag(EffectID::x)

#define KHAOS_TEST_SETTER_CUSTOM(x, type, setter) \
    if ( x ) \
        ec->addSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_TEST_SETTER_CUSTOM_REPLACE(x, type, setter) \
    if ( x ) \
        ec->replaceSetter( EffectSetterBase::type, KHAOS_NEW setter );

#define KHAOS_END_INSTALL_SETTER() \
    }

#define KHAOS_NO_CALC_EFFECT_AND_INFER_ID() \
    virtual void calcEffectID( Renderable* ra, EffectID& id ) { id.clear(); } \
    virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}

    //////////////////////////////////////////////////////////////////////////
    // ReflectBuildStrategy
    class ReflectBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
    };

    //////////////////////////////////////////////////////////////////////////
    // ShadowPreBuildStrategy
    class ShadowPreBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // DeferPreBuildStrategy
    class DeferPreBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void calcInferID( const EffectID& id, EffectID& inferId  );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // LitAccBuildStrategy
    class LitAccBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void calcInferID( const EffectID& id, EffectID& inferId  );
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // VolProbeAccBuildStrategy
    class VolProbeAccBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // DeferCompositeBuildStrategy
    class DeferCompositeBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void calcInferID( const EffectID& id, EffectID& inferId  );
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // ShadowMaskBuildStrategy
    class ShadowMaskBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void calcInferID( const EffectID& id, EffectID& inferId  );
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // DownscaleZBuildStrategy
    class DownscaleZBuildStrategy : public DefaultBuildStrategy
    {
    public:
        KHAOS_NO_CALC_EFFECT_AND_INFER_ID();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // SSAOBuildStrategy
    class SSAOBuildStrategy : public DefaultBuildStrategy
    {
    public:
        KHAOS_NO_CALC_EFFECT_AND_INFER_ID();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // SSAOFilterBuildStrategy
    class SSAOFilterBuildStrategy : public DefaultBuildStrategy
    {
    public:
        KHAOS_NO_CALC_EFFECT_AND_INFER_ID();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // HDRInitLuminBuildStrategy
    class HDRInitLuminBuildStrategy : public DefaultBuildStrategy
    {
    public:
        KHAOS_NO_CALC_EFFECT_AND_INFER_ID();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // HDRLuminIterativeBuildStrategy
    class HDRLuminIterativeBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // HDRAdaptedLumBuildStrategy
    class HDRAdaptedLumBuildStrategy : public DefaultBuildStrategy
    {
    public:
        KHAOS_NO_CALC_EFFECT_AND_INFER_ID();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // HDRBrightPassBuildStrategy
    class HDRBrightPassBuildStrategy : public DefaultBuildStrategy
    {
    public:
        KHAOS_NO_CALC_EFFECT_AND_INFER_ID();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // HDRFlaresPassBuildStrategy
    class HDRFlaresPassBuildStrategy : public DefaultBuildStrategy
    {
    public:
        KHAOS_NO_CALC_EFFECT_AND_INFER_ID();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // HDRFinalPassBuildStrategy
    class HDRFinalPassBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // PostAA
    class AAEdgeDetectionBuildStrategy : public DefaultBuildStrategy
    {
    public:
        KHAOS_NO_CALC_EFFECT_AND_INFER_ID();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    class AABlendingWeightsCalcBuildStrategy : public DefaultBuildStrategy
    {
    public:
        KHAOS_NO_CALC_EFFECT_AND_INFER_ID();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    class AAFinalBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    class AAFinal2BuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id ) {}
        virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    class AATempBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // CommScaleBuildStrategy
    class CommScaleBuildStrategy : public DefaultBuildStrategy
    {
    public:
        KHAOS_NO_CALC_EFFECT_AND_INFER_ID();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // CommCopyBuildStrategy
    class CommCopyBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // CommBlurBuildStrategy
    class CommBlurBuildStrategy : public DefaultBuildStrategy
    {
    public:
        KHAOS_NO_CALC_EFFECT_AND_INFER_ID();
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    //////////////////////////////////////////////////////////////////////////
    // UIBuildStrategy
    class UIBuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id );
        virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };
}

