#include "KhaosPreHeaders.h"
#include "KhaosSHUtil.h"

namespace Khaos
{
    const float SHMath::SQRT_2   = float( ::sqrt(2.0) );
    const float SHMath::SQRT_3   = float( ::sqrt(3.0) );
    const float SHMath::SQRT_5   = float( ::sqrt(5.0) );
    const float SHMath::SQRT_6   = float( ::sqrt(6.0) );
    const float SHMath::SQRT_7   = float( ::sqrt(7.0) );
    const float SHMath::SQRT_15  = float( ::sqrt(15.0) );
    const float SHMath::SQRT_21  = float( ::sqrt(21.0) );
    const float SHMath::SQRT_35  = float( ::sqrt(35.0) );
    const float SHMath::SQRT_105 = float( ::sqrt(105.0) );

    //////////////////////////////////////////////////////////////////////////
#define mul_sh(i) v1[i] * v2[i]

    float SHMath::shDot1( const float* v1, const float* v2 )
    {
        return mul_sh(0);
    }

    float SHMath::shDot4( const float* v1, const float* v2 )
    {
        return mul_sh(0) + mul_sh(1) + mul_sh(2) + mul_sh(3);
    }

    float SHMath::shDot9( const float* v1, const float* v2 )
    {
        return mul_sh(0) + mul_sh(1) + mul_sh(2) + 
               mul_sh(3) + mul_sh(4) + mul_sh(5) + 
               mul_sh(6) + mul_sh(7) + mul_sh(8) ;
    }

    float SHMath::shDot16( const float* v1, const float* v2 )
    {
        return mul_sh(0)  + mul_sh(1)  + mul_sh(2)  + mul_sh(3)  + 
               mul_sh(4)  + mul_sh(5)  + mul_sh(6)  + mul_sh(7)  +
               mul_sh(8)  + mul_sh(9)  + mul_sh(10) + mul_sh(11) +
               mul_sh(12) + mul_sh(13) + mul_sh(14) + mul_sh(15) ;
    }

#undef mul_sh

    float SHMath::shDotN( const float* v1, const float* v2, int n )
    {
        switch ( n )
        {
        case 16:
            return shDot16( v1, v2 );

        case 9:
            return shDot9( v1, v2 );

        case 4:
            return shDot4( v1, v2 );

        case 1:
            return shDot1( v1, v2 );
        }

        khaosAssert(0);
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
#define init_sh(i) float sv##i = sh_func_##i(x, y, z);
#define mul_sh(i) (v[offset+i] * sv##i)
#define begin_sh(n) for ( int g = 0; g < groups; ++g ) { int offset = g * n; ret[g] = 
#define end_sh() ;}

    void SHMath::reconstructFunction1( const float* v, int groups, float x, float y, float z, float* ret )
    {
        init_sh(0)

        begin_sh(1)
            mul_sh(0)
        end_sh()
    }

    void SHMath::reconstructFunction4(  const float* v, int groups, float x, float y, float z, float* ret )
    {
        init_sh(0) init_sh(1) init_sh(2) init_sh(3)

        begin_sh(4)
            mul_sh(0) + mul_sh(1) + mul_sh(2) + mul_sh(3)
        end_sh()
    }

    void SHMath::reconstructFunction9(  const float* v, int groups, float x, float y, float z, float* ret )
    {
        init_sh(0) init_sh(1) init_sh(2) 
        init_sh(3) init_sh(4) init_sh(5) 
        init_sh(6) init_sh(7) init_sh(8)

        begin_sh(9)
            mul_sh(0) + mul_sh(1) + mul_sh(2) +
            mul_sh(3) + mul_sh(4) + mul_sh(5) +
            mul_sh(6) + mul_sh(7) + mul_sh(8)
        end_sh()
    }

    void SHMath::reconstructFunction16(  const float* v, int groups, float x, float y, float z, float* ret )
    {
        init_sh(0)  init_sh(1)  init_sh(2)  init_sh(3)  
        init_sh(4)  init_sh(5)  init_sh(6)  init_sh(7) 
        init_sh(8)  init_sh(9)  init_sh(10) init_sh(11)
        init_sh(12) init_sh(13) init_sh(14) init_sh(15)

        begin_sh(16)
            mul_sh(0)  + mul_sh(1)  + mul_sh(2)  + mul_sh(3)  + 
            mul_sh(4)  + mul_sh(5)  + mul_sh(6)  + mul_sh(7)  +
            mul_sh(8)  + mul_sh(9)  + mul_sh(10) + mul_sh(11) +
            mul_sh(12) + mul_sh(13) + mul_sh(14) + mul_sh(15)
        end_sh()
    }

    void SHMath::reconstructFunctionN(  const float* v, int n, int groups, float x, float y, float z, float* ret )
    {
        switch ( n )
        {
        case 16:
            return reconstructFunction16( v, groups, x, y, z, ret );

        case 9:
            return reconstructFunction9( v, groups, x, y, z, ret );

        case 4:
            return reconstructFunction4( v, groups, x, y, z, ret );

        case 1:
            return reconstructFunction1( v, groups, x, y, z, ret );
        }

        khaosAssert(0);
    }
}

