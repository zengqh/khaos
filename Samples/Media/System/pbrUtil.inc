

// Physically based shading model
// parameterized with the below options

// Microfacet specular = D*G*F / (4*NoL*NoV) = D*Vis*F
// Vis = G / (4*NoL*NoV)

// GGX / Trowbridge-Reitz
// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
float D_GGX( float m, float NoH )
{
	float m2 = m * m;
	float d = ( NoH * m2 - NoH ) * NoH + 1;
	return m2 / ( d*d );
}

// Tuned to match behavior of Vis_Smith
// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
float Vis_Schlick( float m, float NoV, float NoL )
{
	float k = m * 0.5; // unreal

    //float k = (0.353553 * m + 0.565685); // crytek
    //k *= k;

	float Vis_SchlickV = NoV * (1 - k) + k;
	float Vis_SchlickL = NoL * (1 - k) + k;
	return 0.25 / ( Vis_SchlickV * Vis_SchlickL );
}

// Appoximation of joint Smith term for GGX
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
float Vis_SmithJointApprox( float m, float NoV, float NoL )
{
	float Vis_SmithV = NoL * ( NoV * ( 1 - m ) + m );
	float Vis_SmithL = NoV * ( NoL * ( 1 - m ) + m );
	return 0.5 / ( Vis_SmithV + Vis_SmithL );
}

// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
// [Lagarde 2012, "Spherical Gaussian approximation for Blinn-Phong, Phong and Fresnel"]
float3 F_Schlick( float3 SpecularColor, float VoH )
{
	float Fc = pow( 1 - VoH, 5 );							// 1 sub, 3 mul
	//float Fc = exp2( (-5.55473 * VoH - 6.98316) * VoH );	// 1 mad, 1 mul, 1 exp
	
	//return Fc + (1 - Fc) * SpecularColor;					// 1 add, 3 mad
	
	// Anything less than 2% is physically impossible and is instead considered to be shadowing
	return saturate( 50.0 * SpecularColor.g ) * Fc + (1 - Fc) * SpecularColor;
}

float3 calcSpecular( float3 SpecularColor, float Roughness, float NoH, float NoV, float NoL, float VoH )
{
    float m = Roughness * Roughness; // unreal

    //float m = pow( 0.7 * Roughness + 0.3, 6.0 ); // crytek 

    return D_GGX(m, NoH) * Vis_SmithJointApprox(m, NoV, NoL) * F_Schlick(SpecularColor, VoH);
}

float3 standShading( float4 mtrDiffuse, float4 mtrSpecular, 
        float3 objPosWorld, float3 objNormWorld, float3 camPosWorld, 
        float3 lDir, float3 lDiff )
{
    float NdotL = saturate(dot(objNormWorld, lDir));

    //ATTR_BRANCH
    //if ( NdotL > 0 )
    {
        // specular
        float3 vEye = normalize(camPosWorld - objPosWorld);
        float3 vH   = normalize(vEye + lDir);
        
        float  NdotH = saturate(dot(objNormWorld, vH));
        float  NdotV = saturate(dot(objNormWorld, vEye));
        float  VdotH = saturate(dot(vEye, vH));
        
        // total
        float3 brdf = mtrDiffuse.rgb + calcSpecular(mtrSpecular.rgb, mtrSpecular.a, NdotH, NdotV, NdotL, VdotH);
        return brdf * lDiff * NdotL;
    }
    //else
    {
    //    return 0;
    }
}

