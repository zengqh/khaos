

#define SQRT_6  2.4494897
#define SQRT_2  1.4142136
#define SQRT_3  1.7320508

void getIndirectLight( float3 norm, float3 lmClr, float3 lmDir, out float3 lmRet )
{
    static const float3 hlBasis[3] =
    {
        float3(-1.0/SQRT_6,  1.0/SQRT_2, 1.0/SQRT_3),
        float3(-1.0/SQRT_6, -1.0/SQRT_2, 1.0/SQRT_3),
        float3(SQRT_2/SQRT_3, 0, 1.0/SQRT_3)
    };

    float3 dp = saturate( float3( dot(norm, hlBasis[0]), dot(norm, hlBasis[1]), dot(norm, hlBasis[2]) ) );
    dp *= dp;

    float sum = dot( dp, float3(1.0, 1.0, 1.0) );
    float indirect = dot(dp, lmDir) / sum;

    float lmLum = toLumin2(lmClr);
    lmRet = lmClr * (indirect / (lmLum + 0.0001));
}

void fragUnify( inout FragInfoPass pPass, inout PARAMS_IN pIn )
{
    // copy params in
    pPass.pIn = pIn;

    #ifdef USE_UVPROJ // 需要screenTC
        pPass.pIn.wPos.xy = pIn.wPos.xy * screenScale.xy + screenScale.zw;
    #endif

    // 使用法线
    material_NormalVec( pPass.normalWorldVec, pPass.normalTangentVec, pPass.pIn ); // NB:这里是pPass.pIn会用到wPos

    // 设置基本diffuse和specular
    material_DiffuseAndSpecular( pPass.cDiff, pPass.cSpec,
        #ifdef MTR_NEED_ORIPARAS_
        pPass.userData.cBase,
        pPass.userData.msr,
        #endif
        pIn );

    
    // baked ao
    #if defined(HAS_LIGHTMAP1) && !defined(HAS_LIGHTMAP0) // 10 - ao map
        pPass.bakedAO = tex2D(mapLight, pPass.pIn.vTex.zw).r; // NB: this feature supported only for test basic bake
    #else
        material_BakedAO( pPass.bakedAO, pIn ); // baked ao, from material
    #endif

    // 使用眼向量
    #ifdef USE_EYEVEC
        pPass.eyeVec     = cameraPosWorld - pIn.vPosWorld.xyz;
        pPass.eyeVecLen  = length(pPass.eyeVec);
        pPass.eyeVec    /= pPass.eyeVecLen;
    #endif
}

float3 fixCubeSeem( float3 v, float cube_size_nrev, float lod ) 
{
    float3 v_abs = abs(v);
	float M = max(max(v_abs.x, v_abs.y), v_abs.z);
	float scale = exp2(lod) * cube_size_nrev + 1.0;
    return v * (v_abs == M ? (1.0).xxx : scale.xxx);
}

void fragCustomIndirectLight( inout FragInfoPass pPass )
{
    // indirect light here...
    pPass.totalLitDiff = 0;
    pPass.totalLitSpec = 0;

    // light map
    #if defined(HAS_LIGHTMAP1) && defined(HAS_LIGHTMAP0) // 11 - full lightmap
        float3 curLitmapClr = tex2D(mapLight, pPass.pIn.vTex.zw).rgb;
        float3 curLitmapDir = tex2D(mapLightB, pPass.pIn.vTex.zw).rgb;
        getIndirectLight( pPass.normalTangentVec, curLitmapClr, curLitmapDir, pPass.totalLitDiff );
    #elif !defined(HAS_LIGHTMAP1) && defined(HAS_LIGHTMAP0) // 01 - basic lightmap
        pPass.totalLitDiff = tex2D(mapLight, pPass.pIn.vTex.zw).rgb;
    #endif

    // simple amb light, this only in forward phase. maybe also apply ssao for light in deferred phase.
    #if defined(HAS_SIMPLE_AMB)
        float3 simpleAmbLight = ambParas.constColor +
            getSkySimpleClr( pPass.normalWorldVec, ambParas.upperColor, ambParas.lowerColor );
        pPass.totalLitDiff += simpleAmbLight * pPass.bakedAO;
    #endif

    // env diffuse lighting
    #if defined(HAS_ENVDIFF0) && !defined(HAS_ENVDIFF1) // static envmap diffuse
        float3 vRefl = cubeReflectDir(reflect(-pPass.eyeVec, pPass.normalWorldVec));
        float3 envDiffLight = texCUBElod(mapEnvDiff, float4(vRefl, 0)).rgb;
        pPass.totalLitDiff += envDiffLight * pPass.bakedAO;
        #define _VREFL_DEFINED
    #endif

    #ifdef HAS_SPECULAR_LIGHTING
        #if defined(HAS_ENVSPEC0) && !defined(HAS_ENVSPEC1) // static envmap spec
            #if !defined(_VREFL_DEFINED)
                float3 vRefl = cubeReflectDir(reflect(-pPass.eyeVec, pPass.normalWorldVec));
            #endif
            
            float  fLod = pPass.cSpec.w * envMapInfoA.w; // cSpec.w = roughness, envMapInfoA.w = mipCount-1

            //vRefl = fixCubeSeem( vRefl, -1.0/256.0, fLod );

            float3 envSpecLight = texCUBElod(mapEnvSpec, float4(vRefl, fLod)).rgb * pPass.bakedAO;

            float ndotv = saturate(dot(pPass.normalWorldVec, pPass.eyeVec));
            float2 AB = tex2DlodZeroT2(mapEnvLUT, float2(ndotv, pPass.cSpec.w)).rg;

            // Anything less than 2% is physically impossible and is instead considered to be shadowing 
	        float3 GF = pPass.cSpec.rgb * AB.x + saturate(50.0 * pPass.cSpec.g) * AB.y;

            pPass.totalLitSpec += envSpecLight * GF;
        #endif
    #endif
}

void fragCustomEmissive( inout FragInfoPass pPass )
{
#ifdef EN_EMISSIVE
    float3 cEmiss;
    material_Emissive( cEmiss, pPass.pIn );
    pPass.totalLitSpec += cEmiss; // 放在totalLitSpec因为这个已经是最终结果，不用再乘mtrdiff
#endif
}

void fragFinal( inout FragInfoPass pPass, inout float4 outClr )
{
    // 光照材质结合
    outClr.rgb = pPass.totalLitDiff * pPass.cDiff.rgb + pPass.totalLitSpec;

    // alpha
    outClr.a = pPass.cDiff.a;
}

float4 fragSharedOutput( inout FragInfoPass pPass, inout PARAMS_IN pIn )
{
    // 初始化
    fragUnify( pPass, pIn );

    // 计算间接光照
    fragCustomIndirectLight( pPass );
        
    // 计算平行光
    #ifdef HAS_DIR_LITS
    {
        LightInfoPass litPass;

        // 平行光阴影
        #ifdef HAS_SHADOW    
            #ifdef HAS_DEFERRED // 延迟
                float lightIllum = tex2D(mapNormal, pPass.pIn.wPos.xy).a;
            #else // 前向
                float lightIllum = calcShadow(pIn.vPosShadowProj);
            #endif
            // greatly reduces shadow mapping artifacts
            lightIllum *= getShadowReduce( dot(pPass.normalWorldVec, dirLits.direction) );
        #else
            float lightIllum = 1.0;
        #endif

        litPass.lDir  = dirLits.direction;
        litPass.lDiff = dirLits.diffuse * lightIllum;
            
        fragCustomLit( pPass, litPass );
    }
    #endif // HAS_DIR_LITS
    
    #ifndef HAS_DEFERRED // forward only
        // 计算点光源前向
        #ifdef HAS_POINT_LITS
        {
            LightInfoPass litPass;

            ATTR_LOOP for ( int itPt = 0; itPt < pointLitsCount; ++itPt )
            {
                float4 posAtt = pointLits[itPt].position;
                float4 lDiff  = pointLits[itPt].diffuse;

                litPass.lDir  = posAtt.xyz - pIn.vPosWorld.xyz;
                litPass.lDiff = lDiff.rgb * getAttenuation(litPass.lDir, posAtt.w, lDiff.w);
		        litPass.lDir  = normalize(litPass.lDir);

                fragCustomLit( pPass, litPass );
            }
        }
        #endif // HAS_POINT_LITS

        // 聚光灯前向
        #ifdef HAS_SPOT_LITS
        {
            LightInfoPass litPass;

            ATTR_LOOP for ( int itSpt = 0; itSpt < spotLitsCount; ++itSpt )
            {
                float4 posAtt      = spotLits[itSpt].position;
                float3 spotAtts    = spotLits[itSpt].spotAtts;

                litPass.lDir = posAtt.xyz - pIn.vPosWorld.xyz;

                float fallOff = getAttenuation(litPass.lDir, posAtt.w, spotAtts.z); // distance att
                    
                litPass.lDir = normalize(litPass.lDir);

                fallOff *=
                    getSpotAtt( spotLits[itSpt].direction, litPass.lDir, spotAtts.x, spotAtts.y ); // spot att

                litPass.lDiff = spotLits[itSpt].diffuse * fallOff;

                fragCustomLit( pPass, litPass );
            }
        }
        #endif // HAS_SPOT_LITS

    #else // deferred lighting
        assert(0);
        #ifdef HAS_FINIT_LITS
            pPass.totalLitDiff += tex2D(mapLitAccA, pPass.pIn.wPos.xy).rgb;
            #ifdef HAS_SPECULAR_LIGHTING
                pPass.totalLitSpec += tex2D(mapLitAccB, pPass.pIn.wPos.xy).rgb;
            #endif
        #endif
    #endif

    // 计算自发光
    fragCustomEmissive( pPass );
    
    // 自定义结束
    float4 outClr;
    fragFinal( pPass, outClr );
    return outClr;
}

