

BEGIN_DECLARE_UNIFORMS()
END_DECLARE_UNIFORMS()

BEGIN_DECLARE_INS()
    VAR_IN(float4, inPos, POSITION)
    VAR_IN(float2, inTex, TEXCOORD)
END_DECLARE_INS()

BEGIN_DECLARE_OUTS()
    VAR_OUT(float4, k_pos, POSITION)
    VAR_OUT(float2, vTex, TEXCOORD)
END_DECLARE_OUTS()

BEGIN_SECTION()
END_SECTION()

BEGIN_MAIN_FUNCTION()
    pOut.k_pos = pIn.inPos;
    pOut.vTex = pIn.inTex;
END_MAIN_FUNCTION()
