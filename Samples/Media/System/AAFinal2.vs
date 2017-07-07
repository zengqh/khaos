

BEGIN_DECLARE_UNIFORMS()
    VAR_UNIFORM(float2, invTargetSize)
END_DECLARE_UNIFORMS()

BEGIN_DECLARE_INS()
    VAR_IN(float4, inPos, POSITION)
END_DECLARE_INS()

BEGIN_DECLARE_OUTS()
    VAR_OUT(float4, k_pos, POSITION)
    VAR_OUT(float2, vTex, TEXCOORD0)
END_DECLARE_OUTS()

BEGIN_SECTION()
END_SECTION()

BEGIN_MAIN_FUNCTION()
    // proj to uv
    pOut.vTex = projToTex( pIn.inPos.xy );

    // dx9 need offset half pixel to map texture
    pOut.k_pos.xy = projToProjOffset( pIn.inPos.xy, invTargetSize );
    pOut.k_pos.zw = pIn.inPos.zw;

END_MAIN_FUNCTION()

