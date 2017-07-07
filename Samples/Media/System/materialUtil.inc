
#ifndef MTR_ENSURE_DEFAULT_VALUE_
#define MTR_ENSURE_DEFAULT_VALUE_ 0
#endif

#if !defined(MTR_FORBID_ALL_) || defined(MTR_NEED_FUNC_BASEMAPCOLOR_)
float4 material_BaseMapColor( inout PARAMS_IN pIn, float testRef )
{
#ifdef EN_BASEMAP
    float4 baseClr = tex2D(mapBase, pIn.vTex.xy);
#else
    float4 baseClr = 1.0;
#endif

#ifdef EN_OPACITYMAP
    baseClr.a = tex2D(mapOpacity, pIn.vTex.xy).r;
#endif

#ifdef EN_ALPHATEST
    clip( baseClr.a - testRef );
#endif

    return baseClr;
}
#endif

#if !defined(MTR_FORBID_ALL_) || defined(MTR_NEED_FUNC_SPECINFO_)
#ifdef EN_SPECULAR
float3 material_SpecInfo( inout PARAMS_IN pIn )
{
    #ifdef EN_SPECULARMAP
        return surfaceInfo.specularA.rgb * lerp( 1.0, tex2D(mapSpecular, pIn.vTex.xy).xyz, maskSpecular.msrMask.xyz );
    #else
        return surfaceInfo.specularA.rgb; // r = metallic, g = dspecular, b = roughness
    #endif
}
#endif
#endif

#if !defined(MTR_FORBID_ALL_) || defined(MTR_NEED_FUNC_BASECONV_)
void material_BaseConv( float4 baseClr, float3 specInfo, out float4 cDiff, out float4 cSpec )
{
    cDiff.rgb = baseClr.rgb - baseClr.rgb * specInfo.x;
    cDiff.a   = baseClr.a;

	cSpec.rgb = lerp( specInfo.yyy * 0.08, baseClr.rgb, specInfo.xxx );
    cSpec.a   = specInfo.z;
}
#endif

#if !defined(MTR_FORBID_ALL_) || defined(MTR_NEED_FUNC_DIFFUSEANDSPECULAR_)
void material_DiffuseAndSpecular( out float4 cDiff, inout float4 cSpec,
    #ifdef MTR_NEED_ORIPARAS_
    out float4 cBase,
    out float3 msr,
    #endif
    inout PARAMS_IN pIn )
{
    // 基本色
    float4 baseClr = material_BaseMapColor( pIn, surfaceInfo.specularA.a ) * surfaceInfo.baseColorA;

#ifdef EN_VTXCLR // 有顶点色
    baseClr *= pIn.vVtxClr;
#endif

#ifndef EN_SPECULAR // 无高光材质

    // metallic == 0 && DSpecular == 0
    cDiff = baseClr;

    #if MTR_ENSURE_DEFAULT_VALUE_ != 0
        cSpec = 0;
    #endif

    #ifdef MTR_NEED_ORIPARAS_ // 输出原始信息
        cBase = baseClr;
        msr   = float3(0, 0, 1);
    #endif

#else // 有高光材质

    float3 specInfo = material_SpecInfo( pIn );

    // glossy fix
    #if 0
        // 简单限制高光走样
        float3 ddxN = ddx(normalWorldVec);
	    float3 ddyN = ddy(normalWorldVec);
	    float curv2 = max( dot( ddxN, ddxN ), dot( ddyN, ddyN ) );
	    float roughness_min = 1.0909 + 0.0909 * log2( 0.5 * curv2 );

	    specInfo.z = max( specInfo.z, roughness_min );
    #endif
    //

    material_BaseConv( baseClr, specInfo, cDiff, cSpec );

    #ifdef MTR_NEED_ORIPARAS_ // 输出原始信息
        cBase = baseClr;
        msr   = specInfo;
    #endif

#endif
}
#endif

#if !defined(MTR_FORBID_ALL_) || defined(MTR_NEED_FUNC_EMISSIVE_)
void material_Emissive( inout float3 cEmiss, inout PARAMS_IN pIn )
{
#ifdef EN_EMISSIVE
    cEmiss = surfaceInfo.emissive;
    
    #ifdef EN_EMISSMAP
        cEmiss *= tex2D(mapEmiss, pIn.vTex.xy).rgb
    #endif
#else
    //cEmiss = 0;
#endif
}
#endif

#if !defined(MTR_FORBID_ALL_) || defined(MTR_NEED_FUNC_NORMALVEC_)
void material_NormalVec( inout float3 normalWorldVec, inout float3 normalTangentVec, inout PARAMS_IN pIn )
{
#if defined(HAS_LIGHTMAP1) && defined(HAS_LIGHTMAP0) 
    #define _NEED_NORMAL_TANGENT
#endif

#ifdef USE_NORMAL_WORLD
    #ifdef HAS_DEFERRED // 延迟从g-buffer取
        normalWorldVec = decodeNormal( tex2D(mapNormal, pIn.wPos.xy).xyz );
        #ifdef _NEED_NORMAL_TANGENT
            normalTangentVec = float3(0,0,1);
        #endif
    #else // 前向
        #ifdef EN_NORMMAP // 法线贴图取
            float3x3 tanToWSpace = 
            { 
                pIn.vTangent.xyz, 
                cross(pIn.vNormalWorld.xyz, pIn.vTangent.xyz) * pIn.vTangent.w, 
                pIn.vNormalWorld.xyz 
            };
            float3 bumpVec = tex2D(mapBump, pIn.vTex.xy).xyz * 2.0 - 1.0;
            normalWorldVec = normalize(MUL(tanToWSpace, bumpVec));
            #ifdef _NEED_NORMAL_TANGENT
                normalTangentVec = bumpVec;
            #endif
        #else // 顶点法线取
            normalWorldVec = normalize(pIn.vNormalWorld.xyz);
            #ifdef _NEED_NORMAL_TANGENT
                normalTangentVec = float3(0,0,1);
            #endif
        #endif
    #endif
#else
    //normalWorldVec = 0;
    //normalTangentVec = 0;
#endif
}
#endif

#if !defined(MTR_FORBID_ALL_) || defined(MTR_NEED_FUNC_BAKEDAO_)
void material_BakedAO( inout float bakedAO, inout PARAMS_IN pIn )
{
#ifdef EN_BAKEDAOMAP
    bakedAO = tex2D(mapBakedAO, pIn.vTex.xy).r;
#else
    bakedAO = 1.0;
#endif
}
#endif

