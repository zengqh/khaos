
#ifdef KHAOS_HLSL
    #define MUL(m, v)       mul((v), (m))

    #define ATTR_LOOP       [loop]
    #define ATTR_UNROLL     [unroll]
    #define ATTR_BRANCH     [branch]
    #define ATTR_FLATTEN    [flatten]
#else
    #define MUL(m, v)       mul((m), (v))
    
    #define ATTR_LOOP
    #define ATTR_UNROLL
    #define ATTR_BRANCH
    #define ATTR_FLATTEN
#endif

#define M_PI 3.141592654

#define SCENE_HDR_MULTIPLIER 32.0

#define DECLARE_NULL_CUSTOMINFO struct CustomInfo{};

#define DECLARE_FRAGINFOPASS \
struct FragInfoPass \
{ \
    PARAMS_IN pIn; \
    float4 cDiff; \
    float4 cSpec; \
    float3 normalWorldVec; \
    float3 normalTangentVec; \
    float  bakedAO; \
    float3 eyeVec; \
    float  eyeVecLen; \
    float3 totalLitDiff; \
    float3 totalLitSpec; \
    CustomInfo userData; \
}; \
struct LightInfoPass \
{ \
    float3 lDir; \
    float3 lDiff; \
};

#define tex2DlodZeroT2(tex, coord) tex2Dlod(tex, float4(coord, 0.0, 0.0))
#define tex2DlodZeroT3(tex, coord) tex2Dlod(tex, float4(coord, 0.0))

float square( float x )
{
    return x * x;
}

float toLumin( float3 clr )
{
    // Y = 0.2126 R + 0.7152 G + 0.0722 B
    return dot(float3(0.2126, 0.7152, 0.0722), clr);
}

float toLumin2( float3 clr )
{
    return dot(float3(0.299, 0.587, 0.114), clr);
}

float4 gammaCorrect( float4 clr, float gammaValInv )
{
    return float4(pow(max(clr.rgb, 0.0), gammaValInv), clr.a);  
}

float gammaCorrectAlpha( float a )
{
    // return sqrt(a);
    return pow( a, 1.0/2.2 );
}

float3 cubeReflectDir( float3 vDir )
{
#ifdef KHAOS_HLSL
    return float3(vDir.xy, -vDir.z); // d3d flip z
#else
    return vDir;
#endif
}

float4 encodeRGBK( in float4 Color, const float fMultiplier, bool bUsePPP = false )
{
    const float4 cScale = float4(float3(1.0, 1.0, 1.0) / fMultiplier, 1.0 / 255.0);
	
    float fMax = saturate(dot(float4(Color.rgb, 1.0), cScale));   // 1 alu

    Color.a = ceil(fMax * 255.0) / 255.0;                       // 3 alu

    Color.xyz /= Color.a * fMultiplier;                         // 2alu

    if ( bUsePPP )
    {
        Color.a = sqrt( Color.a ); // encode just multiplier for performance reasons
    }

    return Color;
}

float4 decodeRGBK( in float4 Color, const float fMultiplier, bool bUsePPP )
{
    if ( bUsePPP )
        Color.rgb *= (Color.a * Color.a) * fMultiplier;
    else
        Color.rgb *= Color.a * fMultiplier;

  return Color;
}

float3 decodeNormal( float3 vNormal )
{
    return normalize(vNormal * 2.0 - 1.0);
}

float2 projToTex( float2 xy )
{
    // [-1, 1] to [0, 1]
    return xy * float2(0.5,-0.5) + 0.5;
}

float2 projToTexOffset( float2 xy, float2 invsz )
{
    // [-1, 1] to [0, 1]
    // dx9 offset half texel
    return invsz * 0.5 + (xy * float2(0.5,-0.5) + 0.5);
}

float2 projToProjOffset( float2 xy, float2 invsz )
{
    // [0,1]空间移动half pixel，那么[-1, 1]空间移动one pixel
    return float2(-invsz.x, invsz.y) + xy;
}

float4 texToProj( in float4 vPos )
{
    vPos.y = 1.0 - vPos.y;
	return float4(vPos.xy*2.0-1.0, vPos.z, 1.0);
}

float getAttenuation(float3 L, float fInvRadius, float fFalloffExponent)
{
  float3 vDist = L * fInvRadius;
  float fFallOff = saturate(1.0 - dot(vDist, vDist));
  return pow(fFallOff, fFalloffExponent);
}

float getSpotAtt( float3 spotDir, float3 vL, float spotAttX, float spotAttY )
{
    return square(saturate((dot(spotDir, vL) - spotAttX) * spotAttY));
}

float getShadowReduce( float NdotL_NoClipped )
{
    return saturate(NdotL_NoClipped * 6.0 - 0.2);
}

float getFinalAO( float mtrAO, float ssAO )
{
    return mtrAO * ssAO;
}

float3 getSkySimpleClr( float3 worldNormal, float3 upperColor, float3 lowerColor )
{
    // float3 skyVector = float3(0, 1, 0);
    // dot(skyVector, worldNormal)
	float normalContribution = worldNormal.y;

	float2 contributionWeightsSqrt = float2(0.5f, -0.5f) * normalContribution + float2(0.5, 0.5f);
	float2 contributionWeights = contributionWeightsSqrt * contributionWeightsSqrt;

	return contributionWeights.x * upperColor +
	        contributionWeights.y * lowerColor;
}

// NB: must left blank line here

