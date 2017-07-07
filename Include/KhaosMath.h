#pragma once
#include "KhaosStdTypes.h"
#include "KhaosDebug.h"
#include "KhaosRect.h"

namespace Khaos
{
    class AxisAlignedBox;
    class Plane;
    class Sphere;
    class Ray;
    class Vector2;
    class Vector3;
    class Vector4;
    class Matrix4;

    class Math
    {
    public:
        // 常数
        static const float POS_INFINITY;
        static const float NEG_INFINITY;
        static const float PI;
        static const float TWO_PI;
        static const float HALF_PI;
        static const float SQRT_PI;

    private:
        static const float s_deg2Rad;
        static const float s_rad2Deg;

        static bool s_gramSchmidtOrthogonalizeFlip;

    public:
        // 基本数值运算
        static bool isNaN( float f )
        {
            return f != f;
        }

        static int iabs( int x )
        {
            return x >= 0 ? x : -x;
        }

        static float fabs( float x )
        {
            return ::fabs( x );
        }

        static float sqrt( float x )
        {
            return ::sqrt( x );
        }

        static float sqr( float fValue )
        {
            return fValue * fValue;
        }

        static float invSqrt( float x )
        {
            return (float)1.0 / ::sqrt( x );
        }

        static float pow( float x, float y )
        {
            return ::pow( x, y );
        }

        static float exp( float x )
        {
            return ::exp( x );
        }

        static float log( float x )
        {
            return ::log( x );
        }

        static float log2( float x )
        {
            static float s_v = log(2.0f);
            return log(x) / s_v;
        }

        static int log2i( int x )
        {
            int k = 0;

            x = x >> 1;

            while ( x > 0 )
            {
                x = x >> 1;
                ++k;
            }

            return k;
        }

        static float toRadians( float degrees )
        {
            return degrees * s_deg2Rad; 
        }

        static float toDegrees( float radians )
        {
            return radians * s_rad2Deg; 
        }

        static bool realEqual( float a, float b, float tolerance = std::numeric_limits<float>::epsilon() )
        {
            if ( fabs(b-a) <= tolerance )
                return true;
            else
                return false;
        }

        template<typename T>
        static T maxVal( T a, T b )
        {
            return a > b ? a : b;
        }

        template<typename T>
        static T minVal( T a, T b )
        {
            return a < b ? a : b;
        }  

        template <typename T>
        static T clamp( T val, T minval, T maxval )
        {
            khaosAssert( minval <= maxval );
            return maxVal(minVal(val, maxval), minval);
        }

        template<typename T>
        static T saturate( T val )
        {
            return clamp( val, (T)0, (T)1 );
        }

        static float floor( float t )
        {
            return ::floorf( t );
        }

        static float ceil( float t )
        {
            return ::ceilf( t );
        }

        static float modF( float x, float y )
        {
            return ::fmodf( x, y );
        }

        static float frac( float x )
        {
            return x - floor(x);
        }

        static int roundI( float t )
        {
            if ( t > 0 )
                return int(t + 0.5f);
            else
                return int(t - 0.5f);
        }

        static float lerp( float x, float y, float s )
        {
            return  x + s * (y - x);
        }

        static float lerp( float x, float y, float s1, float s2, float s )
        {
            return x + (y - x) * ((s - s1) / (s2 - s1));
        }

        // 随机数
        static void srand( uint s )
        {
            ::srand( s );
        }

        static int randInt()
        {
            // [0, 0x7fff]
            return ::rand();
        }

        static float unitRandom()
        {
            // [0, 1]
            return ::rand() / (float)RAND_MAX;
        }

        static float symmetricRandom()
        {
            // [-1, 1]
            return (float)2.0 * unitRandom() - (float)1.0;
        }

        static float rangeRandom( float low, float high )
        {
            // [low, high]
            return (high - low) * unitRandom() + low;
        }

        static uint32 reverseBase2Fast( uint32 bits );
        static float base2ToFlt( uint32 bits );
        static float reverseBase2FltFast( uint32 bits );

        //static float reverseBase2Flt( uint32 i );

        static UIntVector2 scrambleTEA( const UIntVector2& v, uint IterationCount = 3 );

        // 三角函数
        static float sin( float x )
        {
            return ::sin(x);
        }

        static float cos( float x )
        {
            return ::cos(x);
        }

        static float saturateCos( float x )
        {
            return maxVal( cos(x), 0.0f );
        }

        static float tan( float x )
        {
            return ::tan( x );
        }

        static float asin( float x )
        {
            if ( (float)-1.0 < x )
            {
                if ( x < (float)1.0 )
                    return ::asin(x);
                else
                    return HALF_PI;
            }
            else
            {
                return -HALF_PI;
            }
        }

        static float acos( float x )
        {
            if ( (float)-1.0 < x )
            {
                if ( x < (float)1.0 )
                    return ::acos(x);
                else
                    return (float)0.0;
            }
            else
            {
                return PI;
            }
        }

        static float atan( float x )
        {
            return ::atan( x );
        }

        static float atan2( float y, float x )
        {
            return ::atan2( y, x );
        }

        static uint16 toFloat16( float v );

    public:
        // 矩阵函数
        static void makeTransformLookDir( Matrix4& m, const Vector3& eye, const Vector3& lookDir, const Vector3& upDir );
        static void makeTransformLookAt( Matrix4& m, const Vector3& eye, const Vector3& target, const Vector3& upDir );

    public:
        // 辅助
        static Vector3 calcNormal( const Vector3& p0, const Vector3& p1, const Vector3& p2, 
            bool needNormalised = true );
        
        static void calcTangent( const Vector3& va, const Vector3& vd, const Vector3& vc, 
            const Vector2& uva, const Vector2& uvd, const Vector2& uvc,
            Vector3& tangent, Vector3& binormal );

        static void    setGramSchmidtOrthogonalizeFlip( bool flip ) { s_gramSchmidtOrthogonalizeFlip = flip; }
        static Vector4 gramSchmidtOrthogonalize( const Vector3& t, const Vector3& b, const Vector3& n );

        static float calcArea( const Vector3& p0, const Vector3& p1, const Vector3& p2 );
        static float calcSignedArea( const Vector2& p0, const Vector2& p1, const Vector2& p2 );
        static bool  calcGravityCoord( const Vector2& p0, const Vector2& p1, const Vector2& p2, 
            const Vector2& c, Vector3* g );

    public:
        static Vector3 reflectDir( const Vector3& i, const Vector3& n, bool brdfMode );

    public:
        static float gaussianDistribution1D( float x, float rho );
        static float gaussianDistribution2D( float x, float y, float rho );

    public:
        // 相交测试
        static bool intersects( const Sphere& sphere, const AxisAlignedBox& box );
        static bool intersects( const Plane& plane, const AxisAlignedBox& box );
        static bool intersects( const Sphere& sphere, const Plane& plane );

        static std::pair<bool, float> intersects( const Ray& ray, const Plane& plane );
        static std::pair<bool, float> intersects( const Ray& ray, const AxisAlignedBox& box );
        static std::pair<bool, float> intersects( const Ray& ray, const Sphere& sphere, bool discardInside = true );
        static std::pair<bool, float> intersects( const Ray& ray, const vector<Plane>::type& planeList, bool normalIsOutside );
        static std::pair<bool, float> intersects( const Ray& ray, const list<Plane>::type& planeList, bool normalIsOutside );

        static bool rayIntersectsTriangle( const Ray& ray, const Vector3& v0, const Vector3& v1, const Vector3& v2,
            float* pt = 0, Vector3* gravity = 0 );
    };

    //////////////////////////////////////////////////////////////////////////
#define USE_SSE2_FOR_MERSENNE_TWISTER 1
#if USE_SSE2_FOR_MERSENNE_TWISTER
#include <emmintrin.h>
#endif

    typedef uint32 uint32_t;
    typedef uint64 uint64_t;

#if USE_SSE2_FOR_MERSENNE_TWISTER

    /** 128-bit data structure */
    union W128_T {
        __m128i si;
        uint32_t u[4];
    };
    /** 128-bit data type */
    typedef union W128_T w128_t;

#else

    /** 128-bit data structure */
    struct W128_T {
        uint32_t u[4];
    };
    /** 128-bit data type */
    typedef struct W128_T w128_t;

#endif

#if defined(MEXP) || defined(N) || defined(N32) || defined(N64)
    #error can not define mexp n n32 n64
#endif


/** Mersenne Exponent. The period of the sequence 
 *  is a multiple of 2^MEXP-1.
 * #define MEXP 19937 */
static const uint64_t MEXP = 19937;
/** SFMT generator has an internal state array of 128-bit integers,
 * and N is its size. */
static const uint64_t N = (MEXP / 128 + 1);
/** N32 is the size of internal state array when regarded as an array
 * of 32-bit integers.*/
static const uint64_t N32 = (N * 4);
/** N64 is the size of internal state array when regarded as an array
 * of 64-bit integers.*/
static const uint64_t N64 = (N * 2);

/** Thread-safe Random Number Generator which wraps the SIMD-oriented Fast Mersenne Twister (SFMT). */
class FLMRandomStream
{
public:

	FLMRandomStream(int32 InSeed = 102341) :
		initialized(0)
	{
		psfmt32 = &sfmt[0].u[0];
		psfmt64 = (uint64_t *)&sfmt[0].u[0];
		init_gen_rand(InSeed);
	}

	/** 
	 * Generates a uniformly distributed pseudo-random float in the range [0,1).
	 * This is implemented with the Mersenne Twister and has excellent precision and distribution properties
	 */
	inline float getFraction()
	{
		float NewFraction;
		do 
		{
			NewFraction = (float)genrand_res53();
			// The comment for genrand_res53 says it returns a real number in the range [0,1), but in practice it sometimes returns 1,
			// Possibly a result of rounding during the double -> float conversion
		} while (NewFraction >= 1.0f - 1e-6f);
		return NewFraction;
	}

    inline uint32 getRand32()
    {
        return gen_rand32();
    }

    inline uint64 getRand64()
    {
        return gen_rand64();
    }

    inline int getRandRange( int low, int high )
    {
        assert( low <= high );
        int len = high - low + 1;
        return (gen_rand32() % len) + low;
    }

private:

	/** 
	 * @file SFMT.h 
	 *
	 * @brief SIMD oriented Fast Mersenne Twister(SFMT) pseudorandom
	 * number generator
	 *
	 * @author Mutsuo Saito (Hiroshima University)
	 * @author Makoto Matsumoto (Hiroshima University)
	 *
	 * Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
	 * University. All rights reserved.
	 *
	 * The new BSD License is applied to this software.
	 * see LICENSE.txt
	 *
	 * @note We assume that your system has inttypes.h.  If your system
	 * doesn't have inttypes.h, you have to typedef uint32_t and uint64_t,
	 * and you have to define PRIu64 and PRIx64 in this file as follows:
	 * @verbatim
	 typedef unsigned int uint32_t
	 typedef unsigned long long uint64_t  
	 #define PRIu64 "llu"
	 #define PRIx64 "llx"
	@endverbatim
	 * uint32_t must be exactly 32-bit unsigned integer type (no more, no
	 * less), and uint64_t must be exactly 64-bit unsigned integer type.
	 * PRIu64 and PRIx64 are used for printf function to print 64-bit
	 * unsigned int and 64-bit unsigned int in hexadecimal format.
	 */

	/** the 128-bit internal state array */
	w128_t sfmt[N];
	/** the 32bit integer pointer to the 128-bit internal state array */
	uint32_t *psfmt32;
#if !defined(BIG_ENDIAN64) || defined(ONLY64)
	/** the 64bit integer pointer to the 128-bit internal state array */
	uint64_t *psfmt64;
#endif
	/** index counter to the 32-bit internal state array */
	int idx;
	/** a flag: it is 0 if and only if the internal state is not yet
	* initialized. */
	int initialized;

	uint32_t gen_rand32(void);
	uint64_t gen_rand64(void);
	void fill_array32(uint32_t *array, int size);
	void fill_array64(uint64_t *array, int size);
	void init_gen_rand(uint32_t seed);
	void init_by_array(uint32_t *init_key, int key_length);
	const char * get_idstring(void);
	int get_min_array_size32(void);
	int get_min_array_size64(void);
	void gen_rand_all(void);
	void gen_rand_array(w128_t *array, int size);
	void period_certification(void);

	/* These real versions are due to Isaku Wada */
	/** generates a random number on [0,1]-real-interval */
	inline double to_real1(uint32_t v)
	{
		return v * (1.0/4294967295.0); 
		/* divided by 2^32-1 */ 
	}

	/** generates a random number on [0,1]-real-interval */
	inline double genrand_real1(void)
	{
		return to_real1(gen_rand32());
	}

	/** generates a random number on [0,1)-real-interval */
	inline double to_real2(uint32_t v)
	{
		return v * (1.0/4294967296.0); 
		/* divided by 2^32 */
	}

	/** generates a random number on [0,1)-real-interval */
	inline double genrand_real2(void)
	{
		return to_real2(gen_rand32());
	}

	/** generates a random number on (0,1)-real-interval */
	inline double to_real3(uint32_t v)
	{
		return (((double)v) + 0.5)*(1.0/4294967296.0); 
		/* divided by 2^32 */
	}

	/** generates a random number on (0,1)-real-interval */
	inline double genrand_real3(void)
	{
		return to_real3(gen_rand32());
	}
	/** These real versions are due to Isaku Wada */

	/** generates a random number on [0,1) with 53-bit resolution*/
	inline double to_res53(uint64_t v) 
	{ 
		return v * (1.0/18446744073709551616.0L);
	}

	/** generates a random number on [0,1) with 53-bit resolution from two
	 * 32 bit integers */
	inline double to_res53_mix(uint32_t x, uint32_t y) 
	{ 
		return to_res53(x | ((uint64_t)y << 32));
	}

	/** generates a random number on [0,1) with 53-bit resolution
	 */
	inline double genrand_res53(void) 
	{ 
		return to_res53(gen_rand64());
	} 

	/** generates a random number on [0,1) with 53-bit resolution
		using 32bit integer.
	 */
	inline double genrand_res53_mix(void) 
	{ 
		uint32_t x, y;

		x = gen_rand32();
		y = gen_rand32();
		return to_res53_mix(x, y);
	} 
};
//////////////////////////////////////////////////////////////////////////
}

