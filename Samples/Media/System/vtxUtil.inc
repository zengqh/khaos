

void vtxSharedOutput( inout PARAMS_OUT pOut, inout PARAMS_IN pIn )
{
    pOut.k_pos = MUL(matWVP, pIn.inPos);
    
#if defined(USE_TEXCOORD0) && defined(USE_TEXCOORD1)
    pOut.vTex = float4(pIn.inTex, pIn.inTex2);
#elif defined(USE_TEXCOORD0)
    pOut.vTex = float4(pIn.inTex, 0.0, 0.0);
#elif defined(USE_TEXCOORD1)
    pOut.vTex = float4(0.0, 0.0, pIn.inTex2);
#endif

#ifdef USE_POS_WORLD
    float4 posWorld = MUL(matWorld, pIn.inPos);
    pOut.vPosWorld.xyz = posWorld.xyz;
#endif

#ifdef USE_NORMAL_WORLD
    #ifdef HAS_DEFERRED // 延迟从normal g-buffer取
    #else // 前向法线
        #define TRANSFER_NORMALWORLD_
    #endif
#endif

#ifdef EN_NORMMAP
    #define TRANSFER_NORMALWORLD_
#endif

#ifdef TRANSFER_NORMALWORLD_
    pOut.vNormalWorld.xyz = MUL((float3x3)(matWorld), pIn.inNormal);

    #ifdef EN_NORMMAP // tangent space normalized here
        pOut.vNormalWorld.xyz = normalize( pOut.vNormalWorld.xyz );
    #endif
#endif

#ifdef EN_NORMMAP
    pOut.vTangent = float4( normalize(MUL((float3x3)(matWorld), pIn.inTangent.xyz)), pIn.inTangent.w );
#endif

#ifdef EN_VTXCLR
    pOut.vVtxClr = pIn.inVtxClr;
#endif

#if !defined(HAS_DEFERRED) && defined(HAS_SHADOW) // 前向阴影
    pOut.vPosShadowProj[0] = MUL(matShadowProj[0], posWorld); // 平行w=1
    pOut.vPosShadowProj[1] = MUL(matShadowProj[1], posWorld);
    pOut.vPosShadowProj[2] = MUL(matShadowProj[2], posWorld);
    pOut.vPosShadowProj[3] = MUL(matShadowProj[3], posWorld);
#endif
}

