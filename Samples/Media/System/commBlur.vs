

BEGIN_DECLARE_UNIFORMS()
    VAR_UNIFORM(float2, invTargetSize)
    VAR_UNIFORM(float4, texOffsets, 8)
END_DECLARE_UNIFORMS()

BEGIN_DECLARE_INS()
    VAR_IN(float4, inPos, POSITION)
END_DECLARE_INS()

BEGIN_DECLARE_OUTS()
    VAR_OUT(float4, k_pos, POSITION)
    VAR_OUT(float4, vTex0, TEXCOORD0)
    VAR_OUT(float4, vTex1, TEXCOORD1)
    VAR_OUT(float4, vTex2, TEXCOORD2)
    VAR_OUT(float4, vTex3, TEXCOORD3)
END_DECLARE_OUTS()

BEGIN_SECTION()
END_SECTION()

BEGIN_MAIN_FUNCTION()
    // proj to uv
    float2 vTexCenter = projToTex( pIn.inPos.xy );

    pOut.vTex0.xy = vTexCenter + texOffsets[0].xy;
    pOut.vTex0.zw = vTexCenter + texOffsets[1].xy;

    pOut.vTex1.xy = vTexCenter + texOffsets[2].xy;
    pOut.vTex1.zw = vTexCenter + texOffsets[3].xy;

    pOut.vTex2.xy = vTexCenter + texOffsets[4].xy;
    pOut.vTex2.zw = vTexCenter + texOffsets[5].xy;

    pOut.vTex3.xy = vTexCenter + texOffsets[6].xy;
    pOut.vTex3.zw = vTexCenter + texOffsets[7].xy;

    // dx9 need offset half pixel to map texture
    pOut.k_pos.xy = projToProjOffset( pIn.inPos.xy, invTargetSize );
    pOut.k_pos.zw = pIn.inPos.zw;
END_MAIN_FUNCTION()

