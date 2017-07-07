#include "KhaosPreHeaders.h"
#include "KhaosEffectSetters.h"

#include "KhaosRenderDevice.h"

#include "KhaosSceneGraph.h"
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
    //////////////////////////////////////////////////////////////////////////
    void EffectSurfaceParamsSetter::doSet( EffSetterParams* params )
    {
        // 表面基本参数
        Material* mtr = params->_mtr;

        float buffer[4*3] = {}; // float4 * 3
        UniformPacker pack(buffer, sizeof(buffer));
            
        pack.write( *(Vector3*)mtr->_getCurrBaseColor().ptr() );
        pack.write( mtr->_getCurrAlpha() );

        pack.write( mtr->_getCurrMetallic() );
        pack.write( mtr->_getCurrDSpecular() );
        pack.write( mtr->_getCurrRoughness() );
        pack.write( mtr->_getCurrAlphaTest() );

        pack.writeAsVector4( *(const Vector3*) mtr->_getCurrEmissive().ptr() );

        m_param->setFloatPack( pack.getBlock(), pack.getCurrentSize() );
    }

#if 0
    void _packChannel( UniformPacker& pack, int channel, float val )
    {
        static const Vector3 maskVec[3] = 
        {
            Vector3::UNIT_X, Vector3::UNIT_Y, Vector3::UNIT_Z
        };

        static const Vector4 MASK_VEC_NO = Vector4::ZERO;

        const float MASK_YES = 1.0f;

        if ( channel != SpecularMapAttrib::InvalidChannel )
        {
            pack.write( maskVec[channel] * val );
            pack.write( MASK_YES );
        }
        else
        {
            pack.write( MASK_VEC_NO );
        }
    }
#endif

    void EffectSpecularMaskParamsSetter::doSet( EffSetterParams* params )
    {
        // specular map的掩码用途
        Material* mtr = params->_mtr;
        SpecularMapAttrib* attr = mtr->getAttrib<SpecularMapAttrib>();

        Vector3 maskVal;
        maskVal.x = attr->isMetallicEnabled()  ? 1.0f : 0.0f;
        maskVal.y = attr->isDSpecularEnabled() ? 1.0f : 0.0f;
        maskVal.z = attr->isRoughnessEnabled() ? 1.0f : 0.0f;

        m_param->setFloat3( maskVal.ptr() );

#if 0
        float buffer[4*3] = {}; // float4 * 3
        UniformPacker pack(buffer, sizeof(buffer));

        _packChannel( pack, attr->getMetallicChannel(), mtr->_getCurrMetallic() );
        _packChannel( pack, attr->getDSpecularChannel(), mtr->_getCurrDSpecular() );
        _packChannel( pack, attr->getRoughnessChannel(), mtr->_getCurrRoughness() );

        m_param->setFloatPack( pack.getBlock(), pack.getCurrentSize() );
#endif
    }

    void EffectAlphaTestSetter::doSet( EffSetterParams* params )
    {
        // alpha ref
        m_param->setFloat( params->_mtr->getAttrib<AlphaTestAttrib>()->getValue() );
    }

    Texture* EffectBaseMapSetter::_getTex( EffSetterParams* params )
    {
        return params->_mtr->getAttrib<BaseMapAttrib>()->getTexture();
    }

    Texture* EffectSpecularMapSetter::_getTex( EffSetterParams* params )
    {
        return params->_mtr->getAttrib<SpecularMapAttrib>()->getTexture();
    }

    Texture* EffectEmissiveMapSetter::_getTex( EffSetterParams* params )
    {
        return params->_mtr->getAttrib<EmissiveMapAttrib>()->getTexture();
    }

    Texture* EffectNormMapSetter::_getTex( EffSetterParams* params )
    {
        return params->_mtr->getAttrib<NormalMapAttrib>()->getTexture();
    }

    Texture* EffectOpacityMapSetter::_getTex( EffSetterParams* params )
    {
        return params->_mtr->getAttrib<OpacityMapAttrib>()->getTexture();
    }

    Texture* EffectBakedAOMapSetter::_getTex( EffSetterParams* params )
    {
        return params->_mtr->getAttrib<BakedAOMapAttrib>()->getTexture();
    }

    //////////////////////////////////////////////////////////////////////////
    Texture* EffectLitMapSetter::_getTex( EffSetterParams* params )
    {
        // ao & basic lightmap
        int id = params->ra->getRDSharedData()->getLightMapID();
        return g_renderSystem->_getCurrSceneGraph()->getInstSharedDataMgr().getLightmap(id)->getMap().getTexture();
    }

    void EffectFullLitMapSetter::doSet( EffSetterParams* params )
    {
        // full lightmap
        int id = params->ra->getRDSharedData()->getLightMapID();

        LightmapItem* litItem = g_renderSystem->_getCurrSceneGraph()->getInstSharedDataMgr().getLightmap(id);
        
        m_litMapA->setTextureSamplerState( litItem->getMap().getTexture() );
        m_litMapB->setTextureSamplerState( litItem->getMapB().getTexture() );
    }

    void EffectEnvDiffStaticSetter::doSet( EffSetterParams* params )
    {
        int probeID = params->ra->getRDSharedData()->getEnvProbeID();
        EnvProbeData* probe = g_renderSystem->_getCurrSceneGraph()->getInstSharedDataMgr().getEnvProbe(probeID);

        Texture* tex = probe->getDiffuseTex();
        m_envMapDiff->setTextureSamplerState( tex );
    }

    void EffectEnvSpecStaticSetter::doSet( EffSetterParams* params )
    {
        int probeID = params->ra->getRDSharedData()->getEnvProbeID();
        EnvProbeData* probe = g_renderSystem->_getCurrSceneGraph()->getInstSharedDataMgr().getEnvProbe(probeID);

        Texture* texSpec = probe->getSpecularTex();
        m_envMapSpec->setTextureSamplerState( texSpec );

        Vector4 infoa( 1, 1, 1, (float)(texSpec->getLevels() - 1) );
        m_mapInfoA->setFloat4( infoa.ptr() );
    }

    //////////////////////////////////////////////////////////////////////////
    void EffectAmbParamsSetter::doSet( EffSetterParams* params )
    {
        float buffer[3 * 4] = {}; // 3个参数float4
        UniformPacker pack(buffer, sizeof(buffer));

        SceneGraph* sg = g_renderSystem->_getCurrSceneGraph();
        SkyEnv& sky = sg->getSkyEnv();

        pack.writeAsVector4( sky.getAmbConstClr() );
        pack.writeAsVector4( sky.getAmbUpperClr() );
        pack.writeAsVector4( sky.getAmbLowerClr() );

        m_param->setFloatPack( buffer, pack.getCurrentSize() );
    }

    void EffectDirLitSetter::doSet( EffSetterParams* params )
    {
        // 平行光打包发送
        float buffer[MAX_DIRECTIONAL_LIGHTS * 2 * 4] = {}; // max个灯，2个参数float4
        UniformPacker pack(buffer, sizeof(buffer));

        Light* lit = g_renderSystem->_getCurrDirLit();
        pack.writeAsVector4( -(lit->getDirection()) );
        pack.write( lit->getDiffuse() );
        m_dirLits->setFloatPack( buffer, pack.getCurrentSize() );
    }

    void EffectDirLitShadowDeferUseSetter::doSet( EffSetterParams* params )
    {
        // 阴影相关参数
        Light* lit = g_renderSystem->_getCurrDirLit();
        khaosAssert( lit->_isShadowCurrActive() );
        if ( !lit->_isShadowCurrActive() ) // has direct lit shadow active
            return;

        // 正式绘制：延迟阴影
        TextureObj* normBuff = g_renderSystem->_getGBuf().getNormalBuffer(); // normal g-buffer
        Texture* tmp = g_sysResManager->getTexPointTmp(0);
        tmp->bindTextureObj( normBuff );
        m_shadowMap0->setTextureSamplerState( tmp );
    }

    void EffectDirLitShadowMaskSetter::doSet( EffSetterParams* params )
    {
        // 阴影相关参数
        Light* lit = g_renderSystem->_getCurrDirLit();
        khaosAssert( lit->_isShadowCurrActive() );
        if ( !lit->_isShadowCurrActive() ) // has direct lit shadow active
            return;

        khaosAssert( m_flag == 1 || m_flag == 2 );  // 正式绘制前向阴影或者延迟阴影阶段
        {
            ShadowInfo* shdwInfo = lit->_getShadowInfo();
            ShadowTechDirect* shdwTeck = static_cast<ShadowTechDirect*>(lit->_getShadowTeck());

            Matrix4* matViewProjTexBias;

            //if ( m_flag == 1 ) // 前向
            matViewProjTexBias = shdwInfo->matViewProjTexBiasFor;
            //else // 延迟mask
            //    matViewProjTexBias = shdwInfo->matViewProjTexBias;

            if ( shdwInfo->type == ST_PSSM )
                m_matShadowProj->setMatrix4Array( matViewProjTexBias, shdwInfo->cascadesCount );
            else
                m_matShadowProj->setMatrix4( *matViewProjTexBias );

            {
                float buffer[9 * 4] = {};
                UniformPacker pack(buffer, sizeof(buffer));

                pack.writeAsVector4( shdwTeck->getShadowInfo() );
                pack.writeAsVector4( shdwInfo->getFade() );

                pack.writeAsVector4( shdwTeck->getSplitInfo() );

                pack.write( *(Vector4*)shdwTeck->getAlignOffsetX() );
                pack.write( *(Vector4*)shdwTeck->getAlignOffsetY() );

                pack.writeVector4Array( shdwTeck->getClampRanges(), 4 );

                m_shadowParams->setFloatPack( pack.getBlock(), pack.getCurrentSize() );
            }

            ShaderParamEx* spSM[] = { m_shadowMap0, m_shadowMap1, m_shadowMap2, m_shadowMap3 };

#if MERGE_ONEMAP
            int texCount = 1;
#else
            int texCount = shdwInfo->cascadesCount;
#endif
            for ( int i = 0; i < texCount; ++i )
                spSM[i]->setTextureSamplerState( shdwInfo->getSMTexture(i) );
        }
    }

    const int EffectLitSetterBase::POINT_PACK_FLOATS = MAX_POINT_LIGHTS * 2 * 4; // max个灯，2个参数float4
    const int EffectLitSetterBase::SPOT_PACK_FLOATS  = MAX_SPOT_LIGHTS * 4 * 4; // max个灯，4个参数float4

    void EffectLitSetterBase::_packPoint( UniformPacker& pack, Light* lit1 )
    {
        PointLight* lit = static_cast<PointLight*>(lit1);

        //if ( g_renderSystem->_getRenderPassStage() != MP_LITACC )
        {
            pack.write( lit->getPosition() );
        }
        //else
        //{
        //    Vector3 posView = g_renderSystem->_getCurrCamera()->getViewMatrixRD().transformAffine( lit->getPosition() );
        //    pack.write( posView );
        //}

        pack.write( 1.0f / lit->getRange() );

        pack.write( *(Vector3*) lit->getDiffuse().ptr() );
        pack.write( lit->getFadePower() );
    }

    void EffectLitSetterBase::_packSpot( UniformPacker& pack, Light* lit1 )
    {
        SpotLight* lit = static_cast<SpotLight*>(lit1);

        //if ( g_renderSystem->_getRenderPassStage() != MP_LITACC )
        {
            pack.write( lit->getPosition() );
        }
        //else
        //{
        //    Vector3 posView = g_renderSystem->_getCurrCamera()->getViewMatrixRD().transformAffine( lit->getPosition() );
        //    pack.write( posView );
        //}

        pack.write( 1.0f / lit->getRange() );

        //if ( g_renderSystem->_getRenderPassStage() != MP_LITACC )
        {
            pack.writeAsVector4( -(lit->getDirection()) );
        }
        //else
        //{
        //    Vector3 dirView = g_renderSystem->_getCurrCamera()->getViewMatrixRD().transformAffineNormal(
        //        -(lit->getDirection())
        //        );
        //    pack.writeAsVector4( dirView );
        //}

        pack.write( lit->getDiffuse() );

        pack.write( lit->_getSpotAngles() );
        pack.write( Vector2(lit->getFadePower(), 0) );
    }

    void EffectLitSetter::_checkPointLit( EffSetterParams* params )
    {
        if ( !m_context->getEffectID().testFlag( EffectID::HAS_POINT_LITS ) )
            return;

        // 点光源打包发送
        LightsInfo::LightListInfo*  llinfo = params->_litsInfo->getLightListInfo(LT_POINT);
        LightsInfo::LightListInfo*& llinfoLast = params->_litListInfo[LT_POINT];
        int litsPointCnt = llinfo->maxN;

        if ( !llinfoLast || !llinfoLast->isGreaterEqual(*llinfo) ) // 当无法包容
        {
            // 那么重新打包
            llinfoLast = llinfo;
            LightsInfo::LightList& litsPoint = llinfo->litList;

            float buffer[POINT_PACK_FLOATS] = {};
            UniformPacker pack(buffer, sizeof(buffer));

            for ( int i = 0; i < litsPointCnt; ++i )
                _packPoint( pack, litsPoint[i].lit );

            m_pointLits->setFloatPack( buffer, pack.getCurrentSize() );
        }

        m_pointLitsCount->setInt( litsPointCnt );
    }

    void EffectLitSetter::_checkSpotLit( EffSetterParams* params )
    {
        if ( !m_context->getEffectID().testFlag( EffectID::HAS_SPOT_LITS ) )
            return;

        // 聚光灯打包发送
        LightsInfo::LightListInfo* llinfo = params->_litsInfo->getLightListInfo(LT_SPOT);
        LightsInfo::LightListInfo*& llinfoLast = params->_litListInfo[LT_SPOT];
        int litsSpotCnt = llinfo->maxN;

        if ( !llinfoLast || !llinfoLast->isGreaterEqual(*llinfo) ) // 当无法包容
        {
            // 那么重新打包
            llinfoLast = llinfo;
            LightsInfo::LightList& litsSpot = llinfo->litList;

            float buffer[SPOT_PACK_FLOATS] = {};
            UniformPacker pack(buffer, sizeof(buffer));

            for ( int i = 0; i < litsSpotCnt; ++i )
                _packSpot( pack, litsSpot[i].lit );

            m_spotLits->setFloatPack( buffer, pack.getCurrentSize() );
        }

        m_spotLitsCount->setInt( litsSpotCnt );
    }

    void EffectLitSetter::doSet( EffSetterParams* params )
    {
        _checkPointLit( params );
        _checkSpotLit( params );
    }

    void EffectLitAccSetter::doSet( EffSetterParams* params )
    {
        VolumeRenderable* vr = static_cast<VolumeRenderable*>(params->ra);

        if ( m_context->getEffectID().testFlag( EffectID::HAS_POINT_LITS ) )
        {
            float buffer[POINT_PACK_FLOATS] = {};
            UniformPacker pack(buffer, sizeof(buffer));
            _packPoint( pack, vr->getNode()->getLight() );
            m_pointLits->setFloatPack( buffer, pack.getCurrentSize() );
        }

        if ( m_context->getEffectID().testFlag( EffectID::HAS_SPOT_LITS ) )
        {
            float buffer[SPOT_PACK_FLOATS] = {};
            UniformPacker pack(buffer, sizeof(buffer));
            _packSpot( pack, vr->getNode()->getLight() );
            m_spotLits->setFloatPack( buffer, pack.getCurrentSize() );
        }
    }

    void EffectVolProbAccSetter::doSet( EffSetterParams* params )
    {
        VolProbRenderable* vr = static_cast<VolProbRenderable*>(params->ra);

        // 世界到体积光探的变换
        m_matWorldToVol->setMatrix4( vr->getNode()->getDerivedMatrix().inverseAffine() );

        // volmap
        VolumeProbe* probe = vr->getNode()->getProbe();
        
        VolumeProbeData* data =
            g_renderSystem->_getCurrSceneGraph()->getInstSharedDataMgr().getVolumeProbe( probe->getVolID() );

        m_mapVolR->setTextureSamplerState( data->getMapR().getTexture() );
        m_mapVolG->setTextureSamplerState( data->getMapG().getTexture() );
        m_mapVolB->setTextureSamplerState( data->getMapB().getTexture() );
    }

    //////////////////////////////////////////////////////////////////////////
    void EffectMatWorldSetter::doSet( EffSetterParams* params )
    {
        // mat world
        m_param->setMatrix4( params->ra->getImmMatWorld() );
    }

    void EffectMatWVPSetter::doSet( EffSetterParams* params )
    {
        // mat wvp
        const Matrix4& matWorld = params->ra->getImmMatWorld();
        Matrix4 matWVP = g_renderSystem->_getCurrCamera()->getViewProjMatrixRD() * matWorld;
        m_param->setMatrix4( matWVP );
    }

    void EffectMatViewProjPreSetter::doSet( EffSetterParams* params )
    {
        // mat view proj at last frame
        const Matrix4& mat = g_imageProcessManager->getProcessPostAA()->getMatViewProjPre();
        m_param->setMatrix4( mat );
    }

    //////////////////////////////////////////////////////////////////////////
    void EffectCameraPosWorldSetter::doSet( EffSetterParams* params )
    {
        // camera pos world
        const Vector3& pos = g_renderSystem->_getCurrCamera()->getEyePos();
        m_param->setFloat3( pos.ptr() );
    }

    void EffectZFarSetter::doSet( EffSetterParams* params )
    {
        // camera z far
        float zfar = g_renderSystem->_getCurrCamera()->getZFar();
        m_param->setFloat( zfar );
    }

    void EffectZFarFovSetter::doSet( EffSetterParams* params )
    {
        // camera z far fov
        Vector2 zFarFov;
        zFarFov.x = g_renderSystem->_getCurrCamera()->getZFar();
        zFarFov.y = g_renderSystem->_getCurrCamera()->getFov();
        m_param->setFloat2( zFarFov.ptr() );
    }

    void EffectCamInfoSetter::doSet( EffSetterParams* params )
    {
        // camera z far fov
        Camera* cam = g_renderSystem->_getCurrCamera();
        Vector4 camInfo;
        camInfo.x = cam->getZNear();
        camInfo.y = cam->getZFar();
        camInfo.z = cam->getFov();
        camInfo.w = 1;
        m_param->setFloat4( camInfo.ptr() );
    }

    void EffectTargetInfoSetter::doSet( EffSetterParams* params )
    {
        Vector2 ti;
        Camera* cam = g_renderSystem->_getCurrCamera();

        ti.x = 1.0f / cam->getViewportWidth();
        ti.y = 1.0f / cam->getViewportHeight();

        m_param->setFloat2( ti.ptr() );
    }

    void EffectTargetInfoExSetter::doSet( EffSetterParams* params )
    {
        Vector4 ti;
        Camera* cam = g_renderSystem->_getCurrCamera();

        ti.x = (float) cam->getViewportWidth();
        ti.y = (float) cam->getViewportHeight();
        ti.z = 1.0f / ti.x;
        ti.w = 1.0f / ti.y;

        m_param->setFloat4( ti.ptr() );
    }

    void EffectScreenScaleSetter::doSet( EffSetterParams* params )
    {
        Vector4 ti;
        Camera* cam = g_renderSystem->_getCurrCamera();

        ti.x = 1.0f / (float) cam->getViewportWidth();
        ti.y = 1.0f / (float) cam->getViewportHeight();
        ti.z = 0.5f / (float) cam->getViewportWidth();
        ti.w = 0.5f / (float) cam->getViewportHeight();

        m_param->setFloat4( ti.ptr() );
    }

    void EffectCamVolBiasSetter::doSet( EffSetterParams* params )
    {
        khaosAssert( g_renderSystem->_getCurrCamera() == g_renderSystem->_getMainCamera() );

        const Vector3* volBias = g_renderSystem->_getCurrCamera()->getVolBasis();

        Vector3 bias[3] =
        {
            volBias[0] / (float)g_renderSystem->_getCurrCamera()->getViewportWidth(), // for vpos
            volBias[1] / (float)g_renderSystem->_getCurrCamera()->getViewportHeight(),
            volBias[2]
        };

        if ( m_flag == 1 ) // 需要jitter
        {
            if ( g_renderSystem->_isAATempEnabled() )
            {
                // far平面上撤掉jitter
                bias[2] -= g_imageProcessManager->getProcessPostAA()->getJitterWorld(
                    g_renderSystem->_getCurrCamera() );
            }
        }

        m_camVolBias->setFloat3Array( bias->ptr(), 3 );
    }

    void EffectCamJitterParamsSetter::doSet( EffSetterParams* params )
    {
        khaosAssert( g_renderSystem->_getCurrCamera() == g_renderSystem->_getMainCamera() );

        if ( m_jitterUV )
        {
            Vector2 jitterUV =
                g_imageProcessManager->getProcessPostAA()->getJitterUV( g_renderSystem->_getCurrCamera() );

            m_jitterUV->setFloat2( jitterUV.ptr() );
        }

        if ( m_jitterWorld )
        {
            Vector3 jitterWorld =
                g_imageProcessManager->getProcessPostAA()->getJitterWorld( g_renderSystem->_getCurrCamera() );

            m_jitterWorld->setFloat3( jitterWorld.ptr() );
        }
    }

    void EffectGammaSetter::doSet( EffSetterParams* params )
    {
        GammaSetting* gm = g_renderSystem->_getCurrGammaSetting();
        khaosAssert( gm );
        float gammaInv = 1.0f / gm->getGammaValue();
        m_param->setFloat( gammaInv );
    }

    void EffectGammaAdjSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageProcessFinalScene* process = static_cast<ImageProcessFinalScene*>(pin->getOwner());

        float v = process->getGammaAdjVal();
        m_param->setFloat( v );
    }

    //////////////////////////////////////////////////////////////////////////
    void EffectPoissonDiskSetter::doSet( EffSetterParams* params )
    {
        // poisson disk
        const int possionDiskGroups = 8;
        static float s_poissonDisk[4*possionDiskGroups] = // float4 * possionDiskGroups
        {
            -0.94201624f,  -0.39906216f, 
            0.94558609f,  -0.76890725f, 
            -0.094184101f, -0.92938870f, 
            0.34495938f,   0.29387760f, 
            -0.91588581f,   0.45771432f, 
            -0.81544232f,  -0.87912464f, 
            -0.38277543f,   0.27676845f, 
            0.97484398f,   0.75648379f, 
            0.44323325f,  -0.97511554f, 
            0.53742981f,  -0.47373420f, 
            -0.26496911f,  -0.41893023f, 
            0.79197514f,   0.19090188f, 
            -0.24188840f,   0.99706507f, 
            -0.81409955f,   0.91437590f, 
            0.19984126f,   0.78641367f, 
            0.14383161f,  -0.14100790f 
        };

        // use crytek data
        static bool s_init = false;
        if ( !s_init )
        {
            PoissonDiskGen::setKernelSize( 4 * possionDiskGroups / 2 );
            khaosAssert( sizeof(Vector2) * PoissonDiskGen::getKernelSize() == sizeof(s_poissonDisk) );
            memcpy( s_poissonDisk, PoissonDiskGen::getSample(0).ptr(), sizeof(s_poissonDisk) );
            s_init = true;
        }

        KhaosStaticAssert( MAX_POISSON_DISK <= possionDiskGroups );
        m_param->setFloatPack( s_poissonDisk, sizeof(float) * 4 * MAX_POISSON_DISK );
    }

    Texture* EffectTexEnvLUTSetter::_getTex( EffSetterParams* params )
    {
        return g_sysResManager->getTexEnvLUT();
    }

    Texture* EffectTexRandomSetter::_getTex( EffSetterParams* params )
    {
        return g_sysResManager->getTexRandom();
    }

    Texture* EffectTexNormalFittingSetter::_getTex( EffSetterParams* params )
    {
        return g_sysResManager->getTexNormalFitting();
    }

    Texture* EffectGBufDiffuseSetter::_getTex( EffSetterParams* params )
    {
        Texture* tmp = g_sysResManager->getTexPointTmp();
        tmp->bindTextureObj( g_renderSystem->_getGBuf().getDiffuseBuffer() );
        return tmp;
    }

    Texture* EffectGBufSpecularSetter::_getTex( EffSetterParams* params )
    {
        Texture* tmp = g_sysResManager->getTexPointTmp();
        tmp->bindTextureObj( g_renderSystem->_getGBuf().getSpecularBuffer() );
        return tmp;
    }

    Texture* EffectTexDepthSetter::_getTex( EffSetterParams* params )
    {
        Texture* tmp = g_sysResManager->getTexPointTmp();
        tmp->bindTextureObj( g_renderSystem->_getGBuf().getDepthBuffer() );
        return tmp;
    }

    Texture* EffectTexDepthHalfSetter::_getTex( EffSetterParams* params )
    {
        Texture* tmp = g_sysResManager->getTexPointTmp();
        tmp->bindTextureObj( g_renderSystem->_getDepthHalf() );
        return tmp;
    }

    Texture* EffectTexDepthQuarterSetter::_getTex( EffSetterParams* params )
    {
        Texture* tmp = g_sysResManager->getTexPointTmp();
        tmp->bindTextureObj( g_renderSystem->_getDepthQuarter() );
        return tmp;
    }

    Texture* EffectTexNormalSetter::_getTex( EffSetterParams* params )
    {
        Texture* tmp = g_sysResManager->getTexPointTmp();
        tmp->bindTextureObj( g_renderSystem->_getGBuf().getNormalBuffer() );
        return tmp;
    }

    Texture* EffectTexAOSetter::_getTex( EffSetterParams* params )
    {
        Texture* tmp = g_sysResManager->getTexPointTmp(); // 1:1
        tmp->bindTextureObj( g_renderSystem->_getAoBuf() );
        return tmp;
    }

    Texture* EffectTexInputSetter::_getTex( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();

        Texture* tmp = g_sysResManager->getTexCustomTmp();

        // 目前总是clamp
        tmp->getSamplerState().setAddress( TextureAddressSet::CLAMP );

        // 过滤类型
        tmp->getSamplerState().setFilter( pin->getInputFilter() );

        // 绑定输入
        tmp->bindTextureObj( pin->getInput() );

        return tmp;
    }

    //////////////////////////////////////////////////////////////////////////
    void CommScaleParamsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageScaleProcess* process = static_cast<ImageScaleProcess*>(pin->getOwner());

        float buffer[8] = {};
        UniformPacker pack(buffer, sizeof(buffer));
        pack.write( process->getParams0() );
        pack.write( process->getParams1() );

        m_scaleParams->setFloatPack( pack.getBlock(), pack.getCurrentSize() );
    }

    void CommBlurTexOffsetsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageBlurProcess* process = static_cast<ImageBlurProcess*>(pin->getOwner());

        bool isHorz = pin->getFlag() == 0;

        if ( isHorz )
            m_param->setFloat4Array( process->getOffsetsH()->ptr(), ImageBlurProcess::HALF_SAMPLES_COUNT );
        else
            m_param->setFloat4Array( process->getOffsetsV()->ptr(), ImageBlurProcess::HALF_SAMPLES_COUNT );
    }

    void CommBlurPSWeightsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageBlurProcess* process = static_cast<ImageBlurProcess*>(pin->getOwner());

        m_param->setFloat4Array( process->getWeights()->ptr(), ImageBlurProcess::HALF_SAMPLES_COUNT );
    }

    void DownscaleZParamsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageProcessDownscaleZ* process = static_cast<ImageProcessDownscaleZ*>(pin->getOwner());

        m_paramA->setFloat4( process->getParams0().ptr() );
        m_paramB->setFloat4( process->getParams1().ptr() );
    }

    //////////////////////////////////////////////////////////////////////////
    void SSAOParamsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageProcessGeneralAO* process = static_cast<ImageProcessGeneralAO*>(pin->getOwner());

        m_param1->setFloat4( process->getParam1().ptr() );
        m_param2->setFloat4( process->getParam2().ptr() );
        m_param3->setFloat4( process->getParam3().ptr() );
    }

    void SSAOFilterSampleOffsetsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageProcessBlurAO* process = static_cast<ImageProcessBlurAO*>(pin->getOwner());
        bool isHorz = pin->getFlag() == 0;

        m_param1->setFloat4( process->getParam1().ptr() );
        m_param2->setFloat4( process->getParam2(isHorz).ptr() );
    }

    //////////////////////////////////////////////////////////////////////////
    void InitLumParamsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageProcessInitLumin* process = static_cast<ImageProcessInitLumin*>(pin->getOwner());

        float buffer[8] = {};
        UniformPacker pack(buffer, sizeof(buffer));
        pack.write( process->getParams0() );
        pack.write( process->getParams1() );

        m_offsetParams->setFloatPack( pack.getBlock(), pack.getCurrentSize() );
    }

    void LumIterParamsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageProcessLuminIter* process = static_cast<ImageProcessLuminIter*>(pin->getOwner());

        m_offsetParams->setFloat4Array( process->getParams()->ptr(), 4 );
    }

    void AdaptedLumParamsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageProcessAdaptedLum* process = static_cast<ImageProcessAdaptedLum*>(pin->getOwner());

        Texture* tmp0 = g_sysResManager->getTexPointTmp(0);
        Texture* tmp1 = g_sysResManager->getTexPointTmp(1);

        tmp0->bindTextureObj( process->getLum0() );
        tmp1->bindTextureObj( process->getLum1() );

        m_lum0->setTextureSamplerState( tmp0 );
        m_lum1->setTextureSamplerState( tmp1 );

        m_offsetParams->setFloat4( process->getParam().ptr() );
    }

    void BrightPassParamsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageProcessBrightPass* process = static_cast<ImageProcessBrightPass*>(pin->getOwner());

        Texture* tmp0 = g_sysResManager->getTexPointTmp(0);
        Texture* tmp1 = g_sysResManager->getTexPointTmp(1);

        tmp0->bindTextureObj( process->getSrcTex() );
        tmp1->bindTextureObj( process->getCurLumTex() );

        m_target->setTextureSamplerState( tmp0 );
        m_lum1->setTextureSamplerState( tmp1 );
    }

    void FlaresPassParamsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageProcessFlaresPass* process = static_cast<ImageProcessFlaresPass*>(pin->getOwner());

        Texture* tmp0 = g_sysResManager->getTexPointTmp(0);
        Texture* tmp1 = g_sysResManager->getTexLinearTmp(0);
        Texture* tmp2 = g_sysResManager->getTexLinearTmp(1);

        tmp0->bindTextureObj( process->getBlooms(0) );
        tmp1->bindTextureObj( process->getBlooms(1) );
        tmp2->bindTextureObj( process->getBlooms(2) );

        m_target_4->setTextureSamplerState( tmp0 );
        m_target_8->setTextureSamplerState( tmp1 );
        m_target_16->setTextureSamplerState( tmp2 );
    }

    void HDRParamsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageProcessHDR* procHDR = (ImageProcessHDR*) pin->getOwner()->getUserData();

#define _check_and_set_hdr_param(i)  if ( m_param##i ) m_param##i##->setFloat4( procHDR->_getParam(i).ptr() )

        _check_and_set_hdr_param(0);
        _check_and_set_hdr_param(1);
        _check_and_set_hdr_param(5);
        _check_and_set_hdr_param(8);

    }

    void HDRFinalPassParamsSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageProcessHDR* procHDR = (ImageProcessHDR*) pin->getOwner()->getUserData();

        Texture* tmp0 = g_sysResManager->getTexPointTmp(0);
        Texture* tmp1 = g_sysResManager->getTexPointTmp(1);
        Texture* tmp2 = g_sysResManager->getTexLinearTmp(0);

        tmp0->bindTextureObj( procHDR->_getInBuf() );
        tmp1->bindTextureObj( procHDR->_getCurrLumin() );
        tmp2->bindTextureObj( procHDR->_getFinalBloomQuarter() );

        m_targetFull->setTextureSamplerState( tmp0 );
        m_lum1->setTextureSamplerState( tmp1 );
        m_bloom->setTextureSamplerState( tmp2 );

        //if ( g_renderSystem->_isAATempEnabled() )
        {
            //m_viewProjPrev->setMatrix4( g_imageProcessManager->getProcessPostAA()->getMatViewProjPre() );
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void EffectAABlendParamsSetter::doSet( EffSetterParams* params )
    {
        m_area->setTextureSamplerState( g_sysResManager->getTexArea() );
        m_search->setTextureSamplerState( g_sysResManager->getTexSearch() );
        m_subIdx->setFloat4( g_imageProcessManager->getProcessPostAA()->getSubIdx() );
    }

    void EffectAAFinalParamsSetter::doSet( EffSetterParams* params )
    {
        // color buffer need linear input here!
        Texture* tmp = g_sysResManager->getTexCustomTmp();
        tmp->getSamplerState().setAddress( TextureAddressSet::CLAMP );
        tmp->getSamplerState().setFilter( TextureFilterSet::BILINEAR );
        tmp->getSamplerState().setSRGB( true ); // linear

        tmp->bindTextureObj( g_imageProcessManager->getProcessPostAA()->_getColorBuf() );
        m_color->setTextureSamplerState( tmp );

        // blend buffer
        tmp->getSamplerState().setFilter( TextureFilterSet::TRILINEAR ); // 不知道为何三线性
        tmp->getSamplerState().setSRGB( false ); // linear

        TextureObj* texBlendAABuf = g_imageProcessManager->getProcessPostAA()->_getBlendAABuf();
        tmp->bindTextureObj( texBlendAABuf );
        m_blend->setTextureSamplerState( tmp );
    }

    void EffectAAFinal2ParamsSetter::doSet( EffSetterParams* params )
    {
        Texture* tmp = g_sysResManager->getTexCustomTmp();
        tmp->getSamplerState().setAddress( TextureAddressSet::CLAMP );
        tmp->getSamplerState().setFilter( TextureFilterSet::BILINEAR );
        tmp->getSamplerState().setSRGB( false ); // fxaa要求gamma space

        tmp->bindTextureObj( g_imageProcessManager->getProcessPostAA()->_getColorBuf() );
        m_color->setTextureSamplerState( tmp );
    }

    void EffectAATempParamsSetter::doSet( EffSetterParams* params )
    {
        ImageProcessPostAA* proc = g_imageProcessManager->getProcessPostAA();

        Texture* tmp = g_sysResManager->getTexCustomTmp();
        tmp->getSamplerState().setAddress( TextureAddressSet::CLAMP );
        tmp->getSamplerState().setFilter( TextureFilterSet::BILINEAR );
        
        tmp->bindTextureObj( proc->getFrame(true) );
        m_frameCurr->setTextureSamplerState( tmp );

        tmp->bindTextureObj( proc->getFrame(false) );
        m_framePre->setTextureSamplerState( tmp );

        m_viewProjPrev->setMatrix4( proc->getMatViewProjPre() );
    }

    //////////////////////////////////////////////////////////////////////////
    void EffectEmptyStateSetter::doSet( EffSetterParams* params )
    {
        khaosAssert(0);
    }

    void EffectCommStateSetter::doSet( EffSetterParams* params )
    {
        Material*        mtr        = params->_mtr;
        MaterialStateSet mtrState   = mtr->getMaterialState();
        BlendStateSet    blendState = mtr->getBlendState();

        g_renderDevice->setMaterialStateSet( mtrState );
        g_renderDevice->setBlendStateSet( blendState );
    }

    void EffectShadowPreStateSetter::doSet( EffSetterParams* params )
    {
        MaterialStateSet mtrState = params->_mtr->getMaterialState();
        khaosAssert( !g_renderDevice->isBlendStateEnabled() );
        g_renderDevice->setMaterialStateSet( mtrState );
    }

    void EffectDeferPreStateSetter::doSet( EffSetterParams* params )
    {
        MaterialStateSet mtrState = params->_mtr->getMaterialState();
        khaosAssert( !g_renderDevice->isBlendStateEnabled() );

        const LightsInfo::LightItem* item = 
            (const LightsInfo::LightItem*)params->ra->findCurrDirLitInfoItem();
        bool inSM = ( item && item->inSm );

        // 分类
        static StencilStateSet stIs_Opa( RenderSystem::RK_OPACITY, 
            CMP_ALWAYS, -1, -1, STENCILOP_KEEP, STENCILOP_KEEP, STENCILOP_REPLACE );

        static StencilStateSet stIs_Opa_Shad( RenderSystem::RK_OPACITY | RenderSystem::RK_SHADOW, 
            CMP_ALWAYS, -1, -1, STENCILOP_KEEP, STENCILOP_KEEP, STENCILOP_REPLACE );

        g_renderDevice->setStencilStateSet( inSM ? stIs_Opa_Shad : stIs_Opa );
        g_renderDevice->setMaterialStateSet( mtrState );
    }

    void EffectPostStateSetter::doSet( EffSetterParams* params )
    {
        g_renderDevice->setMaterialStateSet( MaterialStateSet::FRONT_DRAW ); // 永远frontdraw
        khaosAssert( !g_renderDevice->isBlendStateEnabled() );
    }

    void EffectPostBlendAddStateSetter::doSet( EffSetterParams* params )
    {
        khaosAssert( g_renderDevice->isBlendStateEnabled() );
        g_renderDevice->setMaterialStateSet( MaterialStateSet::FRONT_DRAW );
        g_renderDevice->setBlendStateSet( BlendStateSet::ADD );
    }
}

