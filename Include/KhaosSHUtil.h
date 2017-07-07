#pragma once
#include "KhaosVector3.h"
#include "KhaosMath.h"

namespace Khaos
{
    class SHMath
    {
    public:
        static const int MAX_COEFFS = 16;

        static const float SQRT_2;
        static const float SQRT_3;
        static const float SQRT_5;
        static const float SQRT_6;
        static const float SQRT_7;
        static const float SQRT_15;
        static const float SQRT_21;
        static const float SQRT_35;
        static const float SQRT_105;

    public:
        static int getBasisIndex( int L, int M )
        {
            /** Returns the basis index of the SH basis L,M. */
            return L * (L + 1) + M;
        }

        static int getOrderCoeffsCount( int L )
        {
            // M in [-L, L]
            return L * 2 + 1;
        }

        static int getTotalCoeffsCount( int LCount )
        {
            // LCount = Lmax + 1
            return LCount * LCount;
        }

        static float shDot1( const float* v1, const float* v2 );
        static float shDot4( const float* v1, const float* v2 );
        static float shDot9( const float* v1, const float* v2 );
        static float shDot16( const float* v1, const float* v2 );
        static float shDotN( const float* v1, const float* v2, int n );

        static void reconstructFunction1( const float* v, int groups, float x, float y, float z, float* ret );
        static void reconstructFunction4( const float* v, int groups, float x, float y, float z, float* ret );
        static void reconstructFunction9( const float* v, int groups, float x, float y, float z, float* ret );
        static void reconstructFunction16( const float* v, int groups, float x, float y, float z, float* ret );
        static void reconstructFunctionN( const float* v, int n, int groups, float x, float y, float z, float* ret );

        static Vector3 toXYZ( float theta, float phi )
        {
            float sin_theta = Math::sin(theta);

            return Vector3( 
                sin_theta * Math::cos(phi),
                Math::cos(theta),
                sin_theta * Math::sin(phi)
            );
        }

    public:
        // NB: from stupid sh, switch y and z

        // order 1
        static float sh_func_0( float x, float y, float z )
        {
            return 1 / (2*Math::SQRT_PI);
        }

        // order 2
        static float sh_func_1( float x, float y, float z )
        {
            return -SQRT_3 / (2*Math::SQRT_PI) * z;
        }

        static float sh_func_2( float x, float y, float z )
        {
            return SQRT_3 / (2*Math::SQRT_PI) * y;
        }

        static float sh_func_3( float x, float y, float z )
        {
            return -SQRT_3 / (2*Math::SQRT_PI) * x;
        }

        // order 3
        static float sh_func_4( float x, float y, float z )
        {
            return SQRT_15 / (2*Math::SQRT_PI) * z * x;
        }

        static float sh_func_5( float x, float y, float z )
        {
            return -SQRT_15 / (2*Math::SQRT_PI) * z * y;
        }

        static float sh_func_6( float x, float y, float z )
        {
            return SQRT_5 / (4*Math::SQRT_PI) * (3*y*y - 1);
        }

        static float sh_func_7( float x, float y, float z )
        {
            return -SQRT_15 / (2*Math::SQRT_PI) * x * y;
        }

        static float sh_func_8( float x, float y, float z )
        {
            return SQRT_15 / (4*Math::SQRT_PI) * (x*x - z*z);
        }

        // order 4
        static float sh_func_9( float x, float y, float z )
        {
            return -SQRT_2 * SQRT_35 / (8*Math::SQRT_PI) * z * (3*x*x - z*z);
        }

        static float sh_func_10( float x, float y, float z )
        {
            return SQRT_105 / (2*Math::SQRT_PI) * z * x * y;
        }

        static float sh_func_11( float x, float y, float z )
        {
            return -SQRT_2 * SQRT_21 / (8*Math::SQRT_PI) * z * (-1 + 5*y*y);
        }

        static float sh_func_12( float x, float y, float z )
        {
            return SQRT_7 / (4*Math::SQRT_PI) * y * (5*y*y - 3);
        }

        static float sh_func_13( float x, float y, float z )
        {
            return -SQRT_2 * SQRT_21 / (8*Math::SQRT_PI) * x * (-1 + 5*y*y);
        }

        static float sh_func_14( float x, float y, float z )
        {
            return SQRT_105 / (4*Math::SQRT_PI) * (x*x - z*z) * y;
        }

        static float sh_func_15( float x, float y, float z )
        {
            return -SQRT_2 * SQRT_35 / (8*Math::SQRT_PI) * x * (x*x - 3*z*z);
        }
       
        static float sh_func( int i, float x, float y, float z )
        {
            typedef float (*shfunctype)(float,float,float);

            static shfunctype funcs[16] =
            {
                sh_func_0,  sh_func_1,  sh_func_2,  sh_func_3,
                sh_func_4,  sh_func_5,  sh_func_6,  sh_func_7,
                sh_func_8,  sh_func_9,  sh_func_10, sh_func_11,
                sh_func_12, sh_func_13, sh_func_14, sh_func_15
            };

            return funcs[i]( x, y, z );
        }
    };
  
    //////////////////////////////////////////////////////////////////////////
    class HLMath
    {
    public:
        static const Vector3& getHLBasis( int index )
        {
            static const Vector3 s_b[3] =
            {
                Vector3(-1/SHMath::SQRT_6,  1/SHMath::SQRT_2, 1/SHMath::SQRT_3),

                Vector3(-1/SHMath::SQRT_6, -1/SHMath::SQRT_2, 1/SHMath::SQRT_3),

                Vector3(SHMath::SQRT_2/SHMath::SQRT_3, 0, 1/SHMath::SQRT_3)
            };

            khaosAssert( 0 <= index && index < 3 );
            return s_b[index];
        }
    };
}

