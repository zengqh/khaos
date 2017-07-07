

BEGIN_DECLARE_UNIFORMS()
    VAR_UNIFORM(float4, targetInfo)
END_DECLARE_UNIFORMS()

BEGIN_DECLARE_INS()
    VAR_IN(float4, inPos, POSITION)
END_DECLARE_INS()

BEGIN_DECLARE_OUTS()
    VAR_OUT(float4, k_pos, POSITION)
    VAR_OUT(float4, vTex, TEXCOORD0)
END_DECLARE_OUTS()

BEGIN_SECTION()
END_SECTION()

BEGIN_MAIN_FUNCTION()
    // proj to uv
    pOut.vTex.xy = projToTex( pIn.inPos.xy );
    pOut.vTex.zw = pOut.vTex.xy * targetInfo.xy; // xy=screen size

    // dx9 need offset half pixel to map texture
    pOut.k_pos.xy = projToProjOffset( pIn.inPos.xy, targetInfo.zw ); // zw = 1 / screen size
    pOut.k_pos.zw = pIn.inPos.zw;
END_MAIN_FUNCTION()

