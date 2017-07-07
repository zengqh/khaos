

#define MERGE_ONEMAP  1

//#ifndef CASCADES_COUNT
//#define CASCADES_COUNT 4
//#endif

///////////////////////////////////////////////////////////////////////////////
#if SPLIT_FUNC == 0

int getSplitIdx( float3 posworld, inout float3 uvz )
{
#if 0//MERGE_ONEMAP
    float4 posv4 = float4(posworld, 1.0);

    static const float4 rangeuv[4] = 
    {
        float4(0.0, 0.0, 0.5, 0.5),
        float4(0.5, 0.0, 1.0, 0.5),
        float4(0.0, 0.5, 0.5, 1.0),
        float4(0.5, 0.5, 1.0, 1.0)
    };

    // 从最高级的开始扫描
    for ( int i = 0; i < CASCADES_COUNT; ++i )
    {
        uvz = MUL( matShadowProj[i], posv4 ).xyz;

        if ( uvz.z <= 1.0 && 
            all( float4(rangeuv[i].xy, uvz.xy) <= float4(uvz.xy, rangeuv[i].zw) ) )
            return i;
    }

    return CASCADES_COUNT;
#else
    float4 posv4 = float4(posworld, 1.0);

    static const float2 minuv = float2(0.0, 0.0);
    static const float2 maxuv = float2(1.0, 1.0);

    // 从最高级的开始扫描
    for ( int i = 0; i < CASCADES_COUNT; ++i )
    {
        uvz = MUL( matShadowProj[i], posv4 ).xyz;

        if ( uvz.z <= 1.0 && 
            all( float4(minuv, uvz.xy) <= float4(uvz.xy, maxuv) ) )
            return i;
    }

    return CASCADES_COUNT;
#endif
}

#else // #if SPLIT_FUNC == 1

int getSplitIdx( float4 posShadowProj[4], inout float3 uvz )
{
    static const float2 minuv = float2(0.0, 0.0);
    static const float2 maxuv = float2(1.0, 1.0);

    // 从最高级的开始扫描
    for ( int i = 0; i < CASCADES_COUNT; ++i )
    {
        uvz = posShadowProj[i].xyz;

        if ( uvz.z <= 1.0 && 
            all( float4(minuv, uvz.xy) <= float4(uvz.xy, maxuv) ) )
            return i;
    }

    return CASCADES_COUNT;
}
#endif

///////////////////////////////////////////////////////////////////////////////
#if MERGE_ONEMAP
float getShadowVal( float2 uv, float z )
#else
float getShadowVal( int splitIdx, float2 uv, float z )
#endif
{
#if MERGE_ONEMAP
    return tex2Dlod(mapShadow0, float4(uv, z, 0.0)).r;
#else
    float4 pt = float4(uv, z, 0.0);

    ATTR_BRANCH 
    if ( splitIdx == 0 )
        return tex2Dlod(mapShadow0, pt).r;
    else 
    {
        ATTR_BRANCH 
        if ( splitIdx == 1 )
            return tex2Dlod(mapShadow1, pt).r;
        else 
        {
        #if CASCADES_COUNT == 4
            ATTR_BRANCH 
            if ( splitIdx == 2 )
                return tex2Dlod(mapShadow2, pt).r;
            else
            {
                //ATTR_BRANCH 
                //if ( splitIdx == 3 )
                    return tex2Dlod(mapShadow3, pt).r;
                //else
                //    return 1.0;
            }
        #else
            return tex2Dlod(mapShadow2, pt).r;
        #endif
        }
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
#if MERGE_ONEMAP
float4 getFetchSMPos( float4 rotMat, float4 rndKernel, float4 uvCenter, float4 rangeuv )
#else
float4 getFetchSMPos( float4 rotMat, float4 rndKernel, float4 uvCenter )
#endif
{
    float4 uvOut = mad(rotMat.xyzw, rndKernel.xxww, mad(rotMat.zwxy, rndKernel.yyzz, uvCenter));

    #if MERGE_ONEMAP
        uvOut = clamp( uvOut, rangeuv.xyxy, rangeuv.zwzw );
    #endif

    return uvOut;
}

///////////////////////////////////////////////////////////////////////////////
#if SPLIT_FUNC == 0
float calcShadow( float3 posWorld )
#else
float calcShadow( float4 posShadowProj[4] )
#endif
{
    // 在阴影范围之外
    //if ( dot(posView, posView) > shadParams.splitZView.x ) // splitZView.x = shadow camera zfar^2
    //    return 1.0;

    // 得到在shadowmap上uv和分段索引
    float3 uvz = float3(0.0, 0.0, 0.0);

    #if SPLIT_FUNC == 0
    int splitIdx = getSplitIdx(posWorld, uvz);
    #else
    int splitIdx = getSplitIdx(posShadowProj, uvz);
    #endif

    if ( splitIdx == CASCADES_COUNT )
        return 1.0;

    // 随机图snap用偏移uv
    float2 offuv = float2(shadParams.alignOffsetA[splitIdx], shadParams.alignOffsetB[splitIdx]);

    #if MERGE_ONEMAP

    static const float2 uvSplits[4] = 
    {
        float2(0.0, 0.0),
        float2(0.5, 0.0),
        float2(0.0, 0.5),
        float2(0.5, 0.5)
    };

    float4 rangeuv = shadParams.clampRanges[splitIdx];
    float4 uvCurr  = mad( uvz.xy, float2(0.5, 0.5), uvSplits[splitIdx] ).xyxy;

    #else
    
    float4 uvCurr = uvz.xyxy;

    #endif

    float zCurr = uvz.z;

    // 该像素点上的随机旋转，注意使用uvz.xy在[0,1]
    // mapRandom uv16格式，不用* 2.0 - 1.0
    // shadowInfo.xy == shadow map size / randDesiredSize
    // shadowInfo.z == possion scale
    float2 rotations = tex2D( mapRandom, mad(uvz.xy, shadParams.shadowInfo.xy, offuv) ).xy * shadParams.shadowInfo.z;

    float4 rotMat = float4(rotations.x, -rotations.y, rotations.y, rotations.x);

    // 按4个一组统计采样
    float shadowIllum = 0.0;
        
    ATTR_UNROLL for ( int i = 0; i < SHADOW_TAPGROUPS; ++i )
    {
        float4 shadowResult;

        #if MERGE_ONEMAP

        float4 fetchPos = getFetchSMPos( rotMat, poissonDisk[i*2], uvCurr, rangeuv );
        shadowResult.x = getShadowVal( fetchPos.xy, zCurr );
        shadowResult.y = getShadowVal( fetchPos.zw, zCurr );

        fetchPos = getFetchSMPos( rotMat, poissonDisk[i*2+1], uvCurr, rangeuv );
        shadowResult.z = getShadowVal( fetchPos.xy, zCurr );
        shadowResult.w = getShadowVal( fetchPos.zw, zCurr );

        #else

        float4 fetchPos = getFetchSMPos( rotMat, poissonDisk[i*2], uvCurr );
        shadowResult.x = getShadowVal( splitIdx, fetchPos.xy, zCurr );
        shadowResult.y = getShadowVal( splitIdx, fetchPos.zw, zCurr );

        fetchPos = getFetchSMPos( rotMat, poissonDisk[i*2+1], uvCurr );
        shadowResult.z = getShadowVal( splitIdx, fetchPos.xy, zCurr );
        shadowResult.w = getShadowVal( splitIdx, fetchPos.zw, zCurr );

        #endif

        shadowIllum += dot( shadowResult, 
            shadParams.shadowInfo.wwww // shadowInfo.w == shadow weight
        );
    }

    return shadowIllum;
}

