

BEGIN_DECLARE_UNIFORMS()
    VAR_UNIFORM(float4x4, matWVP)
    VAR_UNIFORM(float4x4, matWorld)
    //VAR_UNIFORM(float, zFar)
END_DECLARE_UNIFORMS()

BEGIN_DECLARE_INS()
    VAR_IN(float4, inPos, POSITION)
    VAR_IN(float3, inNormal, NORMAL)
    VAR_IN(float4, inTangent, TANGENT)
    VAR_IN(float2, inTex, TEXCOORD0)
    VAR_IN(float2, inTex2, TEXCOORD1)
    IFDEF(EN_VTXCLR)
        VAR_IN(float4, inVtxClr, COLOR)
    ENDIF()
END_DECLARE_INS()

BEGIN_DECLARE_OUTS()
    VAR_OUT(float4, k_pos, POSITION)
    IFDEF(USE_TEXCOORD01)
        VAR_OUT(float4, vTex, TEXCOORD0)
    ENDIF()
    IFDEF(USE_POS_WORLD)
        VAR_OUT(float3, vPosWorld, TEXCOORD1)
    ENDIF()
    IFDEF(!HAS_DEFERRED && USE_NORMAL_WORLD)
        VAR_OUT(float3, vNormalWorld, TEXCOORD2)
    ENDIF()
    IFDEF(EN_NORMMAP)
        VAR_OUT(float4, vTangent, TEXCOORD3)
    ENDIF()
    IFDEF(EN_VTXCLR)
        VAR_OUT(float4, vVtxClr, TEXCOORD4)
    ENDIF()
END_DECLARE_OUTS()

BEGIN_SECTION()
#include<vtxUtil>
END_SECTION()

BEGIN_MAIN_FUNCTION()
    vtxSharedOutput(pOut, pIn);
    //pOut.vNormalWorld.w = pOut.k_pos.w / zFar; // z_view[0,1]
END_MAIN_FUNCTION()

