

BEGIN_DECLARE_UNIFORMS()
    VAR_UNIFORM(float4x4, matWVP)
END_DECLARE_UNIFORMS()

BEGIN_DECLARE_INS()
    VAR_IN(float4, inPos, POSITION)
    VAR_IN(float2, inTex, TEXCOORD)
END_DECLARE_INS()

BEGIN_DECLARE_OUTS()
    VAR_OUT(float4, k_pos, POSITION)
    IFDEF(USE_TEXCOORD0)
        VAR_OUT(float2, vTex, TEXCOORD0)
    ENDIF()
END_DECLARE_OUTS()

BEGIN_SECTION()
END_SECTION()

BEGIN_MAIN_FUNCTION()
    pOut.k_pos = MUL(matWVP, pIn.inPos);
    
#ifdef USE_TEXCOORD0
    pOut.vTex = pIn.inTex;
#endif
END_MAIN_FUNCTION()
