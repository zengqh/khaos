

BEGIN_DECLARE_UNIFORMS()
    VAR_UNIFORM(float2, invTargetSize)
    VAR_UNIFORM(float4, texToTexParams0)
    VAR_UNIFORM(float4, texToTexParams1)
END_DECLARE_UNIFORMS()

BEGIN_DECLARE_INS()
    VAR_IN(float4, inPos, POSITION)
END_DECLARE_INS()

BEGIN_DECLARE_OUTS()
    VAR_OUT(float4, k_pos, POSITION)
    VAR_OUT(float4, vTexA, TEXCOORD0)
    VAR_OUT(float4, vTexB, TEXCOORD1)
END_DECLARE_OUTS()

BEGIN_SECTION()
END_SECTION()

BEGIN_MAIN_FUNCTION()
    // dx9 need offset half pixel to map texture
    pOut.k_pos.xy = projToProjOffset( pIn.inPos.xy, invTargetSize );
    pOut.k_pos.zw = pIn.inPos.zw;

    // proj to uv
    float2 vTex = projToTex( pIn.inPos.xy );
    pOut.vTexA = vTex.xyxy + texToTexParams0;
    pOut.vTexB = vTex.xyxy + texToTexParams1;
END_MAIN_FUNCTION()

