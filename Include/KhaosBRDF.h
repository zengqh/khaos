#pragma once
#include "KhaosMath.h"

namespace Khaos
{
    inline float M_FromR( float roughness )
    {
        return roughness * roughness;
    }

    inline float D_GGX( float m, float NoH )
    {
        float m2 = m * m;
        float d = ( NoH * m2 - NoH ) * NoH + 1;
        return m2 / ( d*d*Math::PI ); // PI for integration
    }

    inline float Vis_SmithJointApprox( float m, float NoV, float NoL )
    {
        float Vis_SmithV = NoL * ( NoV * ( 1 - m ) + m );
        float Vis_SmithL = NoV * ( NoL * ( 1 - m ) + m );
        return 0.5f / ( Vis_SmithV + Vis_SmithL );
    }

    inline float Fc_Schlick( float VoH )
    {
        return Math::pow( 1.0f - VoH, 5.0f );
    }
}

