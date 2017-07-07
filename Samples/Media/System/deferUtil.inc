

#ifdef USING_FUNC_GETPOSWORLD

float3 getFastPosWorld( float2 uv, float3 camVec )
{
    float fCurFrameDepth = tex2D(mapDepth, uv).r;
	return camVec * fCurFrameDepth + cameraPosWorld;
}

float3 getPosWorldVol( float3 vWBasis[3], float4 wPos, float sceDepth )
{
    return (vWBasis[0] * wPos.x + (vWBasis[1] * wPos.y + vWBasis[2])) * sceDepth + cameraPosWorld;
}

#endif

