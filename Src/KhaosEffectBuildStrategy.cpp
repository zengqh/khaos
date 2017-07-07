#include "KhaosPreHeaders.h"
#include "KhaosEffectBuildStrategy.h"
#include "KhaosEffectSetters.h"
#include "KhaosRenderDevice.h"
#include "KhaosRenderable.h"
#include "KhaosRenderSystem.h"
#include "KhaosLightNode.h"
#include "KhaosImageProcess.h"
#include "KhaosImageProcessShadow.h"
#include "KhaosRenderSetting.h"
#include "KhaosCamera.h"
#include "KhaosSceneGraph.h"
#include "KhaosImageProcessUtil.h"
#include "KhaosImageProcessAA.h"
#include "KhaosImageProcessHDR.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void DefaultBuildStrategy::release()
    {
        KHAOS_DELETE this;
    }

    void DefaultBuildStrategy::_addMacro( String& strMacro, pcstr macName )
    {
        strMacro += String("DEFINE_LINE(") + macName + ")\n#define " + macName + "\n";
    }

    void DefaultBuildStrategy::_addMacro( String& strMacro, pcstr macName, int macVal )
    {
        String strMacVal = intToString( macVal );

        strMacro += "DEFINE_LINE(";
        strMacro += macName;
        strMacro += ", ";
        strMacro += strMacVal;
        strMacro += ")\n#define ";
        strMacro += macName;
        strMacro += " ";
        strMacro += strMacVal;
        strMacro += "\n";
    }

    String DefaultBuildStrategy::makeSystemDefine()
    {
        String strMacro;

        // ƽ̨����
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        _addMacro( strMacro, "KHAOS_WIN32" );
#elif KHAOS_PLATFORM == KHAOS_PLATFORM_IOS
        _addMacro( strMacro, "KHAOS_IOS" );
#elif KHAOS_PLATFORM == KHAOS_PLATFORM_ANDROID
        _addMacro( strMacro, "KHAOS_ANDROID" );
#endif
        // ��Ⱦ�豸����
#if KHAOS_RENDER_SYSTEM == KHAOS_RENDER_SYSTEM_D3D9
        _addMacro( strMacro, "KHAOS_HLSL" );
#else
        _addMacro( strMacro, "KHAOS_GLSL" );
#endif

        // ��Ⱦ���Գ���
        _addMacro( strMacro, "MAX_DIRECTIONAL_LIGHTS", MAX_DIRECTIONAL_LIGHTS );
        _addMacro( strMacro, "MAX_POINT_LIGHTS", MAX_POINT_LIGHTS );
        _addMacro( strMacro, "MAX_SPOT_LIGHTS", MAX_SPOT_LIGHTS );
        _addMacro( strMacro, "MAX_POISSON_DISK", MAX_POISSON_DISK );

        return strMacro;
    }

    Material::ShaderID DefaultBuildStrategy::_makeMtrShaderID( Material* mtr, 
        bool onlyAlphaTest, bool disableNormalMap )
    {
        // �����ǽ���Բ��ʱ���ļ򵥹���
        Material::ShaderID flag = mtr->getShaderFlag();

        if ( onlyAlphaTest ) // �Ƿ����Ҫalphatest���
        {
            if ( flag & EffectID::EN_ALPHATEST ) // ֻ����2��
                flag &= (EffectID::EN_BASEMAP | EffectID::EN_OPACITYMAP | EffectID::EN_ALPHATEST); // ��ʵ�ض���basemap��material update���ж�
            else
                flag = 0;

            return flag;
        }

        if ( !g_renderSystem->_isNormalMapEnabled() || disableNormalMap ) // �Ƿ�ȥ��������ͼ
            flag &= ~EffectID::EN_NORMMAP;

        return flag;
    }

    static void _calcGlobalFlag( Renderable* ra, EffectID& id )
    {
#if 0
        // ȫ�ֵ�һЩ����
        if ( g_renderSystem->_getRenderMode() != RM_FORWARD && // �ӳ���Ⱦ
            g_renderSystem->_getRenderPassStage() == MP_MATERIAL ) // ��ɫ�׶�
        {
            Material* mtr = ra->getImmMaterial();

            if ( !mtr->isBlendEnabled() ) // ��͸������
            {
                id.setFlag( EffectID::HAS_DEFERRED ); // �����ӳ���Ⱦ

                // �Ƿ���ao
                if ( g_renderSystem->_getCurrCamera()->getRenderSettings()->isSettingEnabled<SSAORenderSetting>() )
                {
                    id.setFlag( EffectID::HAS_AO );
                }
            }
        }
#endif

        // ������ջ����Ƿ�gamma
#if 0   // �����ˣ�����ʹ��srgbwrite
        if ( g_renderSystem->_getRenderPassStage() == MP_MATERIAL && // ��ɫ�׶�
            !g_renderSystem->_getCurrHDRSetting() ) // hdr������������gamma��������������
        {
            if ( g_renderSystem->_getCurrGammaSetting() )
                id.setFlag( EffectID::HAS_GAMMA );
        }
#endif
    }

    static void _calcSimpleAmb( EffectID& id )
    {
        // ����򵥻�����
        SceneGraph* sg = g_renderSystem->_getCurrSceneGraph();
        SkyEnv& sky = sg->getSkyEnv();

        if ( sky.isEnabled( SkyEnv::SKY_SIMPLEAMB ) )
            id.setFlag( EffectID::HAS_SIMPLE_AMB );
    }

    static void _calcLightingFlag( Renderable* ra, EffectID& id, 
        bool checkMainLit, bool checkSM, bool checkCostarLits )
    {
        // ������������صĿ���

        // ���ܹ��գ������ж���ϸ����
        const LightsInfo* litInfo = ra->getImmLightInfo();

        // �������Դ
        if ( checkMainLit )
        {
            // ƽ�й����
            const LightsInfo::LightItem* litItem = (const LightsInfo::LightItem*) ra->findCurrDirLitInfoItem();

            if ( litItem )
            {
                id.setFlag( EffectID::HAS_DIR_LITS ); // ��ƽ�й���

                if ( checkSM && litItem->inSm ) // �Ƿ�������Ӱ
                {
                    // ���ò���tap�飬�⵼��HAS_SHADOW����
                    id.mergeFlag( litItem->lit->getShadowTapGroups(), 
                        EffectID::MASK_SHADOW_TAPGROUPS, EffectID::BITOFF_SHADOW_TAPGROUPS );

                    // ������Ӱpssm����
                    if ( litItem->lit->_getShadowInfo()->cascadesCount == 4 )
                        id.setFlag( EffectID::HAS_PSSM4 );
                }
            }
        }
        
        // ��鸱��Դ
		if ( checkCostarLits ) 
		{
			// ���Դ
			LightsInfo::LightListInfo* pointLits = litInfo->getLightListInfo(LT_POINT);
			if ( pointLits && pointLits->maxN > 0 )
			{
				id.setFlag( EffectID::HAS_POINT_LITS );
			}

			// �۹��
			LightsInfo::LightListInfo* spotLits = litInfo->getLightListInfo(LT_SPOT);
			if ( spotLits && spotLits->maxN > 0 )
			{
				id.setFlag( EffectID::HAS_SPOT_LITS );
			}
		}
    }

    static void _calcMiscLightFlag( Renderable* ra, EffectID& id,
        bool checkLitMap, bool enableSpecularLighting, bool checkEnvMap )
    {
        RenderableSharedData* rsd = ra->getRDSharedData();
        InstanceSharedDataMgr& mgr = g_renderSystem->_getCurrSceneGraph()->getInstSharedDataMgr();

        // ���lightmap
        if ( checkLitMap )
        {
            if ( LightmapItem* lmi = mgr.getLightmap( rsd->getLightMapID() ) )
            {
                if ( lmi->isPrepared() )
                {
                    if ( lmi->getType() == LightmapItem::LMT_BASIC ) // 01
                        id.setFlag( EffectID::HAS_LIGHTMAP0 );
                    else if ( lmi->getType() == LightmapItem::LMT_AO ) // 10
                        id.setFlag( EffectID::HAS_LIGHTMAP1 );
                    else if ( lmi->getType() == LightmapItem::LMT_FULL ) // 11
                        id.setFlag( EffectID::HAS_LIGHTMAP0 | EffectID::HAS_LIGHTMAP1 );
                }
            }
        }

        // ��黷����̽
        EnvProbeData* envProbe = 0;

        if ( checkEnvMap )
        {
            RenderableSharedData* rsd = ra->getRDSharedData();
            InstanceSharedDataMgr& mgr = g_renderSystem->_getCurrSceneGraph()->getInstSharedDataMgr();
            envProbe = mgr.getEnvProbe( rsd->getEnvProbeID() );
        }

        // ���diffuse env map
        if ( envProbe )
        {
            // check diffuse env map
            uint type = envProbe->getPreparedDiffuseType();

            if ( type == EnvProbeData::STATIC_DIFFUSE ) // ��̬
                id.setFlag( EffectID::HAS_ENVDIFF0 );
            else if ( type == EnvProbeData::DYNAMIC_DIFFUSE ) // ��̬
                id.setFlag( EffectID::HAS_ENVDIFF1 );
        }

        // �Ƿ�����߹����(�Ⱦ������ǲ���Ӧ��Ҳ�и߹���Ϣ)
        if ( id.testFlag(EffectID::EN_SPECULAR) && enableSpecularLighting )
        {
            id.setFlag( EffectID::HAS_SPECULAR_LIGHTING );

            // ���specular��������
            if ( envProbe )
            {
                uint type = envProbe->getPreparedSpecularType();

                if ( type == EnvProbeData::STATIC_SPECULAR ) // ��̬
                    id.setFlag( EffectID::HAS_ENVSPEC0 );
                else if ( type == EnvProbeData::DYNAMIC_SPECULAR ) // ��̬
                    id.setFlag( EffectID::HAS_ENVSPEC1 );
            }
        }
    }

    static bool _isNeedNormal( const EffectID& id )
    {
        // ��Щ�����Ҫ������Ϣ

        // 1. ��ƽ�й�
        // 2. ǰ�����޵ƹ�
        // 3. �߹����
        // 4. �򵥻�����
        // 5. directional lightmap

        if ( id.testFlag( EffectID::HAS_DIR_LITS | EffectID::HAS_FINIT_LITS |
                          EffectID::HAS_SPECULAR_LIGHTING | 
                          EffectID::HAS_SIMPLE_AMB ) )
            return true;

        const EffectID::ValueType HAS_FULL_LIGHTMAP = EffectID::HAS_LIGHTMAP0 | EffectID::HAS_LIGHTMAP1;
        if ( id.getMaskedValue( HAS_FULL_LIGHTMAP ) == HAS_FULL_LIGHTMAP )
            return true;

        return false;
    }

    static void _optimizeMaterialFlag( EffectID& id )
    {
        // �����ⲿ�����������Ƿ���Ҫ���ʲ���

        // ���normalmap�Ƿ��б�Ҫ����
        if ( id.testFlag(EffectID::EN_NORMMAP) )
        {
            // �ӳ�û��Ҫ����normalmap��g-buffer��ȡ��;��������Ҫ����Ҳû��Ҫ
            if ( id.testFlag(EffectID::HAS_DEFERRED) || !_isNeedNormal(id) ) 
                id.unsetFlag( EffectID::EN_NORMMAP );
        }

        // ���specularmap�Ƿ��б�Ҫ����
        if ( id.testFlag(EffectID::EN_SPECULARMAP) )
        {
            // �ӳ�û��Ҫ����specularmap��g-buffer��ȡ��
            if ( id.testFlag(EffectID::HAS_DEFERRED) )
                id.unsetFlag( EffectID::EN_SPECULARMAP );
        }
    }

    void DefaultBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        // ����id����
        Material* mtr = ra->getImmMaterial();
        id.setValue( _makeMtrShaderID(mtr, false, false) );
        
        // ȫ��id����
        _calcGlobalFlag( ra, id );

        // ����������id
        _calcLightingFlag( ra, id, true, true, true );
        _calcSimpleAmb( id );
        _calcMiscLightFlag( ra, id, true, true, true );

        // �Ż�id
        _optimizeMaterialFlag( id );
    }

    void DefaultBuildStrategy::calcInferID( const EffectID& id, EffectID& inferId  )
    {
        // ��������ƶ�id������׶�id��λֻҪ�о�һ����Ҫ

        // ǰ����̱��
        bool isForwardPass = !id.testFlag(EffectID::HAS_DEFERRED);

        // ��Ҫ��0����������
        if ( id.testFlag(EffectID::EN_BASEMAP | EffectID::EN_SPECULARMAP | EffectID::EN_EMISSMAP |
            EffectID::EN_NORMMAP | EffectID::EN_OPACITYMAP) ) // �л�������ͼ
        {
            inferId.setFlag(EffectID::USE_TEXCOORD0); 
        }

        // ��Ҫ��1����������
        if ( id.testFlag(EffectID::HAS_LIGHTMAP0|EffectID::HAS_LIGHTMAP1) ) // ��light map
        {
            inferId.setFlag(EffectID::USE_TEXCOORD1); 
        }

        // ��Ҫ�з���
        if ( _isNeedNormal(id) )
        {
            inferId.setFlag(EffectID::USE_NORMAL_WORLD); // ����ķ���ָǰ������ӳ�
        }

        if ( isForwardPass ) // ǰ����
        {
            const EffectID::ValueType hasForwardSpecularLighting = EffectID::HAS_SPECULAR_LIGHTING; // ǰ��߹����
            const EffectID::ValueType hasForwardFinitLits = EffectID::HAS_FINIT_LITS; // ǰ�����޵ƹ�
            const EffectID::ValueType hasForwardShadow = EffectID::HAS_SHADOW;    // ǰ����Ӱ

            // �и߹�
            // �����޹�Դ(��Ҫ����˥������)
            // ����Ӱ(��Ҫ���㵽viewproj�ռ䣬��Ҫ����˥��fade)
            if ( id.testFlag(hasForwardSpecularLighting | hasForwardFinitLits | hasForwardShadow) )
            {
                inferId.setFlag(EffectID::USE_POS_WORLD); // ��Ҫ�ж�������λ�ã�NB�����λ���ǽ�ָǰ���λ��
            }

            // ��Ҫ�������
            if ( inferId.testFlag(EffectID::USE_POS_WORLD | EffectID::USE_NORMAL_WORLD) )
            {
                inferId.setFlag( EffectID::USE_MAT_WORLD );
            }

            // �и߹�
            // ����Ӱ(��Ҫ����˥��fade)
            if ( id.testFlag(hasForwardSpecularLighting /*| hasForwardShadow*/) )
            {
                inferId.setFlag(EffectID::USE_CAMERA_POS_WORLD); // ��Ҫ���������λ��
            }

            // ��Ӱ��Ҫpoisson disk
            if ( id.testFlag(hasForwardShadow) )
                inferId.setFlag(EffectID::USE_POISSON_DISK);

            // ��Ӱ��Ҫ���ͼ
            if ( id.testFlag(hasForwardShadow) )
                inferId.setFlag(EffectID::USE_MAP_RANDOM);

            // ��Ҫ������
            // ����Ӱ(��Ҫ����˥��fade)
            if ( id.testFlag(hasForwardSpecularLighting /*| hasForwardShadow*/) )
                inferId.setFlag( EffectID::USE_EYEVEC );

            // ��Ҫenv lut
            if ( id.testFlag(EffectID::HAS_ENVSPEC0 | EffectID::HAS_ENVSPEC1) ) // �о�̬��envmap
                inferId.setFlag( EffectID::USE_MAP_ENVLUT );
        }
        else // ���ӳٵ�ǰ����
        {
            // �ӳ����޹�lit-buff��ao-buffer��defer-shadow/normal g-buffer
            if ( id.testFlag(EffectID::HAS_FINIT_LITS | EffectID::HAS_AO | EffectID::HAS_SHADOW) ||
                inferId.testFlag(EffectID::USE_NORMAL_WORLD) )
                inferId.setFlag(EffectID::USE_UVPROJ);
        }
    }

    KHAOS_BEGIN_DYNAMIC_DEFINE(DefaultBuildStrategy)
        KHAOS_TEST_EFFECT_FLAG(EN_VTXCLR)
        KHAOS_TEST_EFFECT_FLAG(EN_SPECULAR)
        KHAOS_TEST_EFFECT_FLAG(EN_EMISSIVE)
        KHAOS_TEST_EFFECT_FLAG(EN_ALPHATEST)
        KHAOS_TEST_EFFECT_FLAG(EN_BASEMAP)
        KHAOS_TEST_EFFECT_FLAG(EN_SPECULARMAP)
        KHAOS_TEST_EFFECT_FLAG(EN_EMISSMAP)
        KHAOS_TEST_EFFECT_FLAG(EN_NORMMAP)
        KHAOS_TEST_EFFECT_FLAG(EN_OPACITYMAP)
        KHAOS_TEST_EFFECT_FLAG(EN_BAKEDAOMAP)

        KHAOS_TEST_EFFECT_FLAG(HAS_DIR_LITS)
        KHAOS_TEST_EFFECT_FLAG(HAS_POINT_LITS)
        KHAOS_TEST_EFFECT_FLAG(HAS_SPOT_LITS)
        KHAOS_TEST_EFFECT_FLAG(HAS_FINIT_LITS)
        KHAOS_TEST_EFFECT_FLAG(HAS_SIMPLE_AMB)
        KHAOS_TEST_EFFECT_FLAG(HAS_SPECULAR_LIGHTING)

        KHAOS_TEST_EFFECT_FLAG(HAS_DEFERRED)
        KHAOS_TEST_EFFECT_FLAG(HAS_AO)
        //KHAOS_TEST_EFFECT_FLAG(HAS_GAMMA)

        KHAOS_TEST_EFFECT_FLAG(HAS_SHADOW)
        KHAOS_TEST_EFFECT_FLAG(HAS_PSSM4)
        KHAOS_TEST_EFFECT_VALUE_OFFSET(HAS_SHADOW, SHADOW_TAPGROUPS, MASK_SHADOW_TAPGROUPS, BITOFF_SHADOW_TAPGROUPS)

        KHAOS_TEST_EFFECT_FLAG(HAS_ENVSPEC0)
        KHAOS_TEST_EFFECT_FLAG(HAS_ENVSPEC1)
        KHAOS_TEST_EFFECT_FLAG(HAS_ENVDIFF0)
        KHAOS_TEST_EFFECT_FLAG(HAS_ENVDIFF1)
        KHAOS_TEST_EFFECT_FLAG(HAS_LIGHTMAP0)
        KHAOS_TEST_EFFECT_FLAG(HAS_LIGHTMAP1)

        KHAOS_TEST_INFER_FLAG(USE_TEXCOORD0)
        KHAOS_TEST_INFER_FLAG(USE_TEXCOORD1)
        KHAOS_TEST_INFER_FLAG(USE_TEXCOORD01)
        KHAOS_TEST_INFER_FLAG(USE_MAT_WORLD)
        KHAOS_TEST_INFER_FLAG(USE_POS_WORLD)
        KHAOS_TEST_INFER_FLAG(USE_NORMAL_WORLD)
        KHAOS_TEST_INFER_FLAG(USE_UVPROJ)
        KHAOS_TEST_INFER_FLAG(USE_EYEVEC)
    KHAOS_END_DYNAMIC_DEFINE()


    KHAOS_BEGIN_INSTALL_SETTER(DefaultBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_MATERIAL, EffectSurfaceParamsSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_BASEMAP, PT_MATERIAL, EffectBaseMapSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_SPECULARMAP, PT_MATERIAL, EffectSpecularMapSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_SPECULARMAP, PT_MATERIAL, EffectSpecularMaskParamsSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_EMISSMAP, PT_MATERIAL, EffectEmissiveMapSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_NORMMAP, PT_MATERIAL, EffectNormMapSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_OPACITYMAP, PT_MATERIAL, EffectOpacityMapSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_BAKEDAOMAP, PT_MATERIAL, EffectBakedAOMapSetter)

        //KHAOS_TEST_INFER_SETTER_DEFER(USE_NORMAL_WORLD, PT_GLOBAL, EffectTexNormalSetter) // defer use normal g-buff
        //KHAOS_TEST_EFFECT_SETTER(HAS_AO, PT_GLOBAL, EffectTexAOSetter)

        KHAOS_TEST_EFFECT_SETTER(HAS_DIR_LITS, PT_GLOBAL, EffectDirLitSetter)
        KHAOS_TEST_EFFECT_SETTER_FORWARD(HAS_SHADOW, PT_GLOBAL, EffectDirLitShadowMaskSetter(1)) // forward shadow
        //KHAOS_TEST_EFFECT_SETTER_DEFER(HAS_SHADOW, PT_GLOBAL, EffectDirLitShadowDeferUseSetter) // deferred shadow

        KHAOS_TEST_EFFECT_SETTER_FORWARD(HAS_FINIT_LITS, PT_LIGHT, EffectLitSetter) // forward finit lit
        //KHAOS_TEST_EFFECT_SETTER_DEFER(HAS_FINIT_LITS, PT_GLOBAL, EffectTexLitAccASetter) // deferred finit lit
        //KHAOS_TEST_EFFECT_SETTER_DEFER(HAS_FINIT_LITS, PT_GLOBAL, EffectTexLitAccBSetter)

        KHAOS_TEST_EFFECT_SETTER(HAS_SIMPLE_AMB, PT_GLOBAL, EffectAmbParamsSetter)

        KHAOS_TEST_SETTER_CUSTOM( !_OIDHAS(HAS_ENVSPEC1) && _OIDHAS(HAS_ENVSPEC0), PT_GEOM, EffectEnvSpecStaticSetter)  // 01 - static
        KHAOS_TEST_SETTER_CUSTOM( _OIDHAS(HAS_ENVSPEC1) && !_OIDHAS(HAS_ENVSPEC0), PT_GEOM, EffectEnvSpecDynamicSetter) // 10 - dynamic
        KHAOS_TEST_INFER_SETTER( USE_MAP_ENVLUT, PT_GLOBAL, EffectTexEnvLUTSetter ) // envlut

        KHAOS_TEST_SETTER_CUSTOM( !_OIDHAS(HAS_ENVDIFF1) && _OIDHAS(HAS_ENVDIFF0), PT_GEOM, EffectEnvDiffStaticSetter)  // 01 - static

        KHAOS_TEST_SETTER_CUSTOM( !_OIDHAS(HAS_LIGHTMAP1) && _OIDHAS(HAS_LIGHTMAP0), PT_GEOM, EffectLitMapSetter) // 01 - basic litmap
        KHAOS_TEST_SETTER_CUSTOM( _OIDHAS(HAS_LIGHTMAP1) && !_OIDHAS(HAS_LIGHTMAP0), PT_GEOM, EffectLitMapSetter) // 10 - ao litmap
        KHAOS_TEST_SETTER_CUSTOM( _OIDHAS(HAS_LIGHTMAP1) && _OIDHAS(HAS_LIGHTMAP0), PT_GEOM, EffectFullLitMapSetter) // 11 - full litmap

        KHAOS_ALWAYS_BIND_SETTER(PT_GEOM, EffectMatWVPSetter)
        KHAOS_TEST_INFER_SETTER( USE_MAT_WORLD, PT_GEOM, EffectMatWorldSetter)

        KHAOS_TEST_INFER_SETTER(USE_CAMERA_POS_WORLD, PT_GLOBAL, EffectCameraPosWorldSetter)
        //KHAOS_TEST_INFER_SETTER(USE_UVPROJ, PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_TEST_INFER_SETTER(USE_UVPROJ, PT_GLOBAL, EffectScreenScaleSetter)

        KHAOS_TEST_INFER_SETTER(USE_POISSON_DISK, PT_GLOBAL, EffectPoissonDiskSetter)
        KHAOS_TEST_INFER_SETTER(USE_MAP_RANDOM, PT_GLOBAL, EffectTexRandomSetter)
        //KHAOS_TEST_EFFECT_SETTER(HAS_GAMMA, PT_GLOBAL, EffectGammaSetter)

        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectCommStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void ReflectBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        Material* mtr = ra->getImmMaterial();

        // �������̹ر����ԣ�
        // ��Ҫ������ͼ
        // ��Ҫ������Ӱ,��Ҫ���޹�Դ,��Ҫ�߹���գ���Ҫ������ͼ
        id.setValue( _makeMtrShaderID(mtr, false, true) );
        _calcLightingFlag( ra, id, true, false, false );
        _calcMiscLightFlag( ra, id, true, false, false );
    }

    //////////////////////////////////////////////////////////////////////////
    void ShadowPreBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        Material* mtr = ra->getImmMaterial();

        // ֻ����alphatest
        id.setValue( _makeMtrShaderID(mtr, true, false) );
    }

    KHAOS_BEGIN_INSTALL_SETTER(ShadowPreBuildStrategy)
        KHAOS_TEST_EFFECT_SETTER(EN_BASEMAP, PT_MATERIAL, EffectBaseMapSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_OPACITYMAP, PT_MATERIAL, EffectOpacityMapSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_ALPHATEST, PT_MATERIAL, EffectAlphaTestSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GEOM, EffectMatWVPSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectShadowPreStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void DeferPreBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        Material* mtr = ra->getImmMaterial();

        id.setValue( _makeMtrShaderID(mtr, false, false) );

        // ֻ�����ӹ���
        _calcLightingFlag( ra, id, false, false, false );
        _calcMiscLightFlag( ra, id, true, true, true );

        // NB����Ȼû��ֱ�ӹ��գ�Ҳ����ʵ��û�м�ӹ��գ�������������ȻҪ���������Ϣ
        // ���Բ����Ż���
        // _optimizeMaterialFlag( id ); 
    }

    void DeferPreBuildStrategy::calcInferID( const EffectID& id, EffectID& inferId  )
    {
        // ��Ȼ����������ߣ������ڷ��߲�����һЩ��ʱ��Ϣ���ǲ�����Ҫ
        // ������calcEffectID��û���Ż��Ŀ���������ɣ�

        EffectID idbak = id;
        _optimizeMaterialFlag( idbak ); 
        DefaultBuildStrategy::calcInferID( idbak, inferId );
        inferId.setFlag( EffectID::USE_NORMAL_WORLD | EffectID::USE_MAT_WORLD ); // ���ǰ���normal
    }

    KHAOS_BEGIN_INSTALL_SETTER(DeferPreBuildStrategy)
        KHAOS_DERIVED_INSTALL_SETTER(DefaultBuildStrategy)
        //KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectZFarSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexNormalFittingSetter)
        KHAOS_ALWAYS_BIND_SETTER_REPLACE(PT_STATE, EffectDeferPreStateSetter) // ʹ��deferpreר��״̬����
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void LitAccBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        VolumeRenderable* vra = static_cast<VolumeRenderable*>(ra);

        switch ( vra->getNode()->getLightType() )
        {
        case LT_POINT:
            id.setValue( EffectID::HAS_POINT_LITS );
            break;

        case LT_SPOT:
            id.setValue( EffectID::HAS_SPOT_LITS );
            break;
        }

        // �Ƿ�����Ӱ
        if ( vra->getNode()->getLight()->_isShadowCurrActive() )
        {
            id.setFlag( EffectID::EN_BIT1 );
        }
        else // ��û����Ӱ������£���ao
        {
            // �Ƿ�AO
            if ( g_renderSystem->_isSSAOEnabled() )
                id.setFlag( EffectID::EN_BIT2 );
        }
    }

    void LitAccBuildStrategy::calcInferID( const EffectID& id, EffectID& inferId  )
    {
    }

    KHAOS_BEGIN_DYNAMIC_DEFINE(LitAccBuildStrategy)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT1)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT2)
        KHAOS_TEST_EFFECT_FLAG(HAS_POINT_LITS)
        KHAOS_TEST_EFFECT_FLAG(HAS_SPOT_LITS)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(LitAccBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GEOM, EffectMatWVPSetter)
        //KHAOS_ALWAYS_BIND_SETTER(PT_GEOM, EffectMatWorldSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GEOM, EffectLitAccSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectCameraPosWorldSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectCamVolBiasSetter(1))
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectScreenScaleSetter)
        //KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectZFarSetter)
        //KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectGBufDiffuseSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectGBufSpecularSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexDepthSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexNormalSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_BIT2, PT_GLOBAL, EffectTexAOSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectCommStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void VolProbeAccBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        VolProbRenderable* vra = static_cast<VolProbRenderable*>(ra);

    }

    KHAOS_BEGIN_DYNAMIC_DEFINE(VolProbeAccBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(VolProbeAccBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GEOM, EffectMatWVPSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GEOM, EffectVolProbAccSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectCameraPosWorldSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectCamVolBiasSetter(1))
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectScreenScaleSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectGBufDiffuseSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectGBufSpecularSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexDepthSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexNormalSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectCommStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void DeferCompositeBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        // �Ƿ�temp aa
        id.setValue( g_renderSystem->_isAATempEnabled() ? EffectID::EN_BIT0 : 0 );

        // �Ƿ�������Դ
        Light* curDirLit = g_renderSystem->_getCurrDirLit();

        if ( curDirLit )
        {
            id.setFlag( EffectID::HAS_DIR_LITS );

            // �Ƿ�����Ӱ
            if ( curDirLit->_isShadowCurrActive() )
                id.setFlag( EffectID::EN_BIT1 );
        }

        // �Ƿ�AO
        if ( g_renderSystem->_isSSAOEnabled() )
            id.setFlag( EffectID::EN_BIT2 );

        // �򵥻�����
        _calcSimpleAmb( id );
    }

    void DeferCompositeBuildStrategy::calcInferID( const EffectID& id, EffectID& inferId  )
    {
    }

    KHAOS_BEGIN_DYNAMIC_DEFINE(DeferCompositeBuildStrategy)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT0)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT1)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT2)
        KHAOS_TEST_EFFECT_FLAG(HAS_SIMPLE_AMB)
        KHAOS_TEST_EFFECT_FLAG(HAS_DIR_LITS)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(DeferCompositeBuildStrategy)
        KHAOS_TEST_EFFECT_SETTER(HAS_DIR_LITS, PT_GLOBAL, EffectDirLitSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectCameraPosWorldSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectGBufDiffuseSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectGBufSpecularSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexDepthSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexNormalSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_BIT0, PT_GLOBAL, EffectCamJitterParamsSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_BIT0, PT_GLOBAL, EffectMatViewProjPreSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_BIT2, PT_GLOBAL, EffectTexAOSetter)
        KHAOS_TEST_EFFECT_SETTER(HAS_SIMPLE_AMB, PT_GLOBAL, EffectAmbParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostBlendAddStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void ShadowMaskBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        Light* curDirLit = g_renderSystem->_getCurrDirLit();

        // �Ƿ�temp aa
        id.setValue( g_renderSystem->_isAATempEnabled() ? EffectID::EN_BIT0 : 0 );

        // ���ò���tap��
        id.mergeFlag( curDirLit->getShadowTapGroups(), 
            EffectID::MASK_SHADOW_TAPGROUPS, EffectID::BITOFF_SHADOW_TAPGROUPS );

        // ������Ӱ����
        if ( curDirLit->_getShadowInfo()->cascadesCount == 4 )
            id.setFlag( EffectID::EN_BIT1 );
    }

    void ShadowMaskBuildStrategy::calcInferID( const EffectID& id, EffectID& inferId  )
    {
    }

    KHAOS_BEGIN_DYNAMIC_DEFINE(ShadowMaskBuildStrategy)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT0)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT1)
        KHAOS_ALWAYS_EFFECT_VALUE_OFFSET(SHADOW_TAPGROUPS, MASK_SHADOW_TAPGROUPS, BITOFF_SHADOW_TAPGROUPS)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(ShadowMaskBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectDirLitShadowMaskSetter(2))

        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectCameraPosWorldSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexDepthSetter)

        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexRandomSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectPoissonDiskSetter)

        KHAOS_TEST_EFFECT_SETTER(EN_BIT0, PT_GLOBAL, EffectCamJitterParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(DownscaleZBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(DownscaleZBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, DownscaleZParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexInputSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(SSAOBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(SSAOBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoExSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectCamInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexDepthSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexDepthHalfSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexDepthQuarterSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, SSAOParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(SSAOFilterBuildStrategy)  
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(SSAOFilterBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoExSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexDepthSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexInputSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, SSAOFilterSampleOffsetsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(HDRInitLuminBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(HDRInitLuminBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, InitLumParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexInputSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void HDRLuminIterativeBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        const Material* mtr = ra->getImmMaterial();

        // �������ε���
        if ( mtr->isCommStateEnabled(MtrCommState::BIT0) )
            id.setValue( EffectID::EN_BIT0 );
    }

    KHAOS_BEGIN_DYNAMIC_DEFINE(HDRLuminIterativeBuildStrategy)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT0)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(HDRLuminIterativeBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, LumIterParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexInputSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(HDRAdaptedLumBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(HDRAdaptedLumBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, AdaptedLumParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(HDRBrightPassBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(HDRBrightPassBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, BrightPassParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, HDRParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(HDRFlaresPassBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(HDRFlaresPassBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, FlaresPassParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void HDRFinalPassBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        const Material* mtr = ra->getImmMaterial();

        // bit0: gamma adjust
        // bit1: smaa jitter
        id.setValue( mtr->getCommState().getValue() & (MtrCommState::BIT0 | MtrCommState::BIT1) );

        // �Ƿ�дluma
        if ( g_imageProcessManager->getProcessHDR()->_isWriteLuma() )
            id.setFlag( EffectID::EN_BIT2 );
    }

    KHAOS_BEGIN_DYNAMIC_DEFINE(HDRFinalPassBuildStrategy)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT0)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT1)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT2)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(HDRFinalPassBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexDepthSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectZFarSetter)
        //KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, HDRParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, HDRFinalPassParamsSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_BIT0, PT_MATERIAL, EffectGammaAdjSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_BIT1, PT_MATERIAL, EffectCameraPosWorldSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_BIT1, PT_MATERIAL, EffectCamJitterParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(AAEdgeDetectionBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(AAEdgeDetectionBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexInputSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(AABlendingWeightsCalcBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(AABlendingWeightsCalcBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexInputSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectAABlendParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void AAFinalBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        // srgb write����������ֶ�gamma correct
        //if ( g_renderSystem->_getCurrGammaSetting() )
        //    id.setValue( EffectID::EN_BIT0 );
    }

    KHAOS_BEGIN_DYNAMIC_DEFINE(AAFinalBuildStrategy)
        //KHAOS_TEST_EFFECT_FLAG(EN_BIT0)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(AAFinalBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectAAFinalParamsSetter)
        //KHAOS_TEST_EFFECT_SETTER(EN_BIT0, PT_MATERIAL, EffectGammaSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(AAFinal2BuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(AAFinal2BuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectAAFinal2ParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void AATempBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        // ����motion
        //id.setFlag( EffectID::EN_BIT0 );

        // �Ƿ�дluma
        if ( g_imageProcessManager->getProcessPostAA()->_isTempWriteLuma() )
            id.setFlag( EffectID::EN_BIT1 );
    }

    KHAOS_BEGIN_DYNAMIC_DEFINE(AATempBuildStrategy)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT0)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT1)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(AATempBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectCameraPosWorldSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexDepthSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectAATempParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectCamJitterParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(CommScaleBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(CommScaleBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, CommScaleParamsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexInputSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void CommCopyBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        // �Ƿ�дluma
        if ( g_imageProcessManager->getProcessCopy()->isWriteLuma() )
            id.setFlag( EffectID::EN_BIT0 );
    }

    KHAOS_BEGIN_DYNAMIC_DEFINE(CommCopyBuildStrategy)
        KHAOS_TEST_EFFECT_FLAG(EN_BIT0)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(CommCopyBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexInputSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    KHAOS_BEGIN_DYNAMIC_DEFINE(CommBlurBuildStrategy)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(CommBlurBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, CommBlurTexOffsetsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, CommBlurPSWeightsSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_GLOBAL, EffectTexInputSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    void UIBuildStrategy::calcEffectID( Renderable* ra, EffectID& id )
    {
        //if ( g_renderSystem->_getCurrGammaSetting() )
        //    id.setFlag( EffectID::HAS_GAMMA );
    }

    KHAOS_BEGIN_INSTALL_SETTER(UIBuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_MATERIAL, EffectBaseMapSetter)
        //KHAOS_TEST_EFFECT_SETTER(HAS_GAMMA, PT_GLOBAL, EffectGammaSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectCommStateSetter)
    KHAOS_END_INSTALL_SETTER()
}

