#include "KhaosPreHeaders.h"
#include "KhaosMath.h"
#include "KhaosAxisAlignedBox.h"
#include "KhaosPlane.h"
#include "KhaosSphere.h"
#include "KhaosRay.h"
#include "KhaosVector2.h"
#include "khaosRect.h"

namespace Khaos
{
    const float Math::POS_INFINITY   =  std::numeric_limits<float>::infinity();
    const float Math::NEG_INFINITY   = -std::numeric_limits<float>::infinity();
    const float Math::PI             = float( 4.0 * atan( 1.0 ) );
    const float Math::TWO_PI         = float( 2.0 * PI );
    const float Math::HALF_PI        = float( 0.5 * PI );
    const float Math::SQRT_PI        = float( ::sqrt(PI) );
    const float Math::s_deg2Rad      = PI / float(180.0);
    const float Math::s_rad2Deg      = float(180.0) / PI;

    bool Math::s_gramSchmidtOrthogonalizeFlip = false;

    //////////////////////////////////////////////////////////////////////////
    uint16 _toFloat16_Impl( float v );

    uint16 Math::toFloat16( float v )
    {
        return _toFloat16_Impl(v);
    }

    uint32 Math::reverseBase2Fast( uint32 Bits )
    {
        Bits = ( Bits << 16) | ( Bits >> 16);
        Bits = ( (Bits & 0x00ff00ff) << 8 ) | ( (Bits & 0xff00ff00) >> 8 );
        Bits = ( (Bits & 0x0f0f0f0f) << 4 ) | ( (Bits & 0xf0f0f0f0) >> 4 );
        Bits = ( (Bits & 0x33333333) << 2 ) | ( (Bits & 0xcccccccc) >> 2 );
        Bits = ( (Bits & 0x55555555) << 1 ) | ( (Bits & 0xaaaaaaaa) >> 1 );

        return Bits;
    }

    float Math::base2ToFlt( uint32 bits )
    {
        static const double invLargeNum = 1.0 / (double)0x100000000LL;
        return float(bits * invLargeNum);
    }

    float Math::reverseBase2FltFast( uint32 Bits )
    {
        return base2ToFlt(reverseBase2Fast(Bits));
    }

#if 0
    float Math::reverseBase2Flt( uint32 i )
    {
        double x = 0;
        double f = 0.5;

        while ( i )
        {
            x += f * (double)(i & 1);
            i >>= 1;
            f *= 0.5;
        }

        return (float)x;
    }
#endif

    UIntVector2 Math::scrambleTEA( const UIntVector2& v, uint IterationCount )
    {
        // Start with some random data (numbers can be arbitrary but those have been used 
        // by others and seem to work well)
        uint k[4] = { 0xA341316Cu , 0xC8013EA4u , 0xAD90777Du , 0x7E95761Eu };

        uint y = v[0];
        uint z = v[1];
        uint sum = 0;

        for ( uint i = 0; i < IterationCount; ++i )
        {
            sum += 0x9e3779b9;
            y += (z << 4u) + k[0] ^ z + sum ^ (z >> 5u) + k[1];
            z += (y << 4u) + k[2] ^ y + sum ^ (y >> 5u) + k[3];
        }

        return UIntVector2(y, z);
    }

    //////////////////////////////////////////////////////////////////////////
    void Math::makeTransformLookDir( Matrix4& m, const Vector3& eye, const Vector3& lookDir, const Vector3& upDir )
    {
        Vector3 zdir = -lookDir;
        zdir.normalise();

        Vector3 xdir = upDir.crossProduct(zdir);
        xdir.normalise();

        Vector3 ydir = zdir.crossProduct(xdir);

        m[0][0] = xdir.x; m[0][1] = ydir.x; m[0][2] = zdir.x; m[0][3] = eye.x;
        m[1][0] = xdir.y; m[1][1] = ydir.y; m[1][2] = zdir.y; m[1][3] = eye.y;
        m[2][0] = xdir.z; m[2][1] = ydir.z; m[2][2] = zdir.z; m[2][3] = eye.z;
        m[3][0] = 0.0;    m[3][1] = 0.0;    m[3][2] = 0.0;    m[3][3] = 1.0f;
    }

    void Math::makeTransformLookAt( Matrix4& m, const Vector3& eye, const Vector3& target, const Vector3& upDir )
    {
        makeTransformLookDir( m, eye, target-eye, upDir );
    }

    Vector3 Math::calcNormal( const Vector3& p0, const Vector3& p1, const Vector3& p2, bool needNormalised )
    {
        Vector3 p01 = p1 - p0;
        Vector3 p02 = p2 - p0;

        Vector3 ret = p01.crossProduct( p02 );

        if ( needNormalised )
            return ret.normalisedCopy();
        else
            return ret;
    }

    bool _inverse_mat2( float v[4] )
    {
        // 计算2x2逆矩阵
        float a = v[0];
        float b = v[1];
        float c = v[2];
        float d = v[3];

        float m_t = a * d - b * c;

        v[0] = d;
        v[1] = -b;
        v[2] = -c;
        v[3] = a;

        if ( m_t != 0.0f )
        {
            v[0] /= m_t;
            v[1] /= m_t;
            v[2] /= m_t;
            v[3] /= m_t;
            return true;
        }

        return false; // 奇异矩阵
    }

    void Math::calcTangent( const Vector3& va, const Vector3& vd, const Vector3& vc, 
        const Vector2& uva, const Vector2& uvd, const Vector2& uvc,
        Vector3& tangent, Vector3& binormal )
    {
        // tri ADC
        // | Uad  Vad | * | T | = | AD |  
        // | Uac  Vac |   | B |   | AC |

        float matUV[4] = 
        {
            uvd.x-uva.x,  uvd.y-uva.y,
            uvc.x-uva.x,  uvc.y-uva.y
        };

        if ( !_inverse_mat2( matUV ) )
        {
            tangent = Vector3::UNIT_X;
            binormal = Vector3::UNIT_Y;
            return;
        }

        Vector3 ad = vd - va;
        Vector3 ac = vc - va;

        tangent  = ad * matUV[0] + ac * matUV[1];
        binormal = ad * matUV[2] + ac * matUV[3];
    }

    Vector4 Math::gramSchmidtOrthogonalize( const Vector3& t1, const Vector3& b, const Vector3& n )
    {
        // Gram-Schmidt orthogonalize
        Vector3 t = (t1 - n * n.dotProduct(t1)).normalisedCopy();

        // Calculate handedness
        bool hand = ( n.crossProduct(t1).dotProduct(b) >= 0.0f ); // we use dx's uv, so V coordinate left hand, not < 0

        if ( s_gramSchmidtOrthogonalizeFlip ) // 强制矫正反转
            hand = !hand;

        if ( hand )
            return Vector4(t, -1);
        else
            return Vector4(t, 1);
    }

    float Math::calcArea( const Vector3& p0, const Vector3& p1, const Vector3& p2 )
    {
        Vector3 p01 = p1 - p0;
        Vector3 p02 = p2 - p0;

        Vector3 q = p01.crossProduct(p02);
        return q.length() * 0.5f;
    }

    float Math::calcSignedArea( const Vector2& p0, const Vector2& p1, const Vector2& p2 )
    {
        float x1 = p0.x;
        float x2 = p1.x;
        float x3 = p2.x;

        float y1 = p0.y;
        float y2 = p1.y;
        float y3 = p2.y;

        return ((y1 - y3) * (x2 - x3) + (y2 - y3) * (x3 - x1)) * 0.5f;
    }

    bool Math::calcGravityCoord( const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& c, Vector3* g )
    {
        float st = calcSignedArea( p0, p1, p2 );
        
        if ( fabs(st) < 1e-6f ) // 不考虑0面积
            return false;

        float s0 = calcSignedArea( c, p1, p2 );
        float s1 = calcSignedArea( c, p2, p0 );

        s0 /= st;
        s1 /= st;

        const float minS = 0.0f;
        const float maxS = 1.0f;

        if ( s0 < minS || s0 > maxS )
            return false;

        if ( s1 < minS || s1 > maxS )
            return false;

        float s2 = 1 - s0 - s1;
        if ( s2 < minS || s2 > maxS )
            return false;

        g->x = s0;
        g->y = s1;
        g->z = s2;
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    Vector3 Math::reflectDir( const Vector3& i, const Vector3& n, bool brdfMode )
    {
        // brdf  mode: v = -i + 2 * n * dot(i.n)
        // shade mode: v =  i - 2 * n * dot(i.n)

        Vector3 v = -i + n * (2.0f * i.dotProduct(n));
        return brdfMode ? v : -v;
    }

    //////////////////////////////////////////////////////////////////////////
    float Math::gaussianDistribution1D( float x, float rho )
    {
        static const float two_pi_sqrt = sqrt(TWO_PI);
        float g = 1.0f / (rho * two_pi_sqrt); 
        g *= exp( -(x * x) / (2.0f * rho * rho) );
        return g;
    }

    float Math::gaussianDistribution2D( float x, float y, float rho )
    {
        float g = 1.0f / sqrt(TWO_PI * rho * rho);
        g *= exp( -(x * x + y * y) / (2.0f * rho * rho) );
        return g;
    }

    //////////////////////////////////////////////////////////////////////////
    bool Math::intersects(const Sphere& sphere, const AxisAlignedBox& box)
    {
        if (box.isNull()) return false;
        if (box.isInfinite()) return true;

        // Use splitting planes
        const Vector3& center = sphere.getCenter();
        float radius = sphere.getRadius();
        const Vector3& min = box.getMinimum();
        const Vector3& max = box.getMaximum();

        // Arvo's algorithm
        float s, d = 0;
        for (int i = 0; i < 3; ++i)
        {
            if (center.ptr()[i] < min.ptr()[i])
            {
                s = center.ptr()[i] - min.ptr()[i];
                d += s * s; 
            }
            else if(center.ptr()[i] > max.ptr()[i])
            {
                s = center.ptr()[i] - max.ptr()[i];
                d += s * s; 
            }
        }
        return d <= radius * radius;

    }
    
    bool Math::intersects(const Plane& plane, const AxisAlignedBox& box)
    {
        return (plane.getSide(box) == Plane::BOTH_SIDE);
    }

    bool Math::intersects(const Sphere& sphere, const Plane& plane)
    {
        return (
            Math::fabs(plane.getDistance(sphere.getCenter()))
            <= sphere.getRadius() );
    }

    std::pair<bool, float> Math::intersects(const Ray& ray, const Plane& plane)
    {
        float denom = plane.normal.dotProduct(ray.getDirection());
        if (Math::fabs(denom) < std::numeric_limits<float>::epsilon())
        {
            // Parallel
            return std::pair<bool, float>(false, (float)0);
        }
        else
        {
            float nom = plane.normal.dotProduct(ray.getOrigin()) + plane.d;
            float t = -(nom/denom);
            return std::pair<bool, float>(t >= 0, t);
        }
    }

    std::pair<bool, float> Math::intersects(const Ray& ray, const AxisAlignedBox& box)
    {
        if (box.isNull()) return std::pair<bool, float>(false, (float)0);
        if (box.isInfinite()) return std::pair<bool, float>(true, (float)0);

        float lowt = 0.0f;
        float t;
        bool hit = false;
        Vector3 hitpoint;
        const Vector3& min = box.getMinimum();
        const Vector3& max = box.getMaximum();
        const Vector3& rayorig = ray.getOrigin();
        const Vector3& raydir = ray.getDirection();

        // Check origin inside first
        if ( rayorig > min && rayorig < max )
        {
            return std::pair<bool, float>(true, (float)0);
        }

        // Check each face in turn, only check closest 3
        // Min x
        if (rayorig.x <= min.x && raydir.x > 0)
        {
            t = (min.x - rayorig.x) / raydir.x;
            if (t >= 0)
            {
                // Substitute t back into ray and check bounds and dist
                hitpoint = rayorig + raydir * t;
                if (hitpoint.y >= min.y && hitpoint.y <= max.y &&
                    hitpoint.z >= min.z && hitpoint.z <= max.z &&
                    (!hit || t < lowt))
                {
                    hit = true;
                    lowt = t;
                }
            }
        }
        // Max x
        if (rayorig.x >= max.x && raydir.x < 0)
        {
            t = (max.x - rayorig.x) / raydir.x;
            if (t >= 0)
            {
                // Substitute t back into ray and check bounds and dist
                hitpoint = rayorig + raydir * t;
                if (hitpoint.y >= min.y && hitpoint.y <= max.y &&
                    hitpoint.z >= min.z && hitpoint.z <= max.z &&
                    (!hit || t < lowt))
                {
                    hit = true;
                    lowt = t;
                }
            }
        }
        // Min y
        if (rayorig.y <= min.y && raydir.y > 0)
        {
            t = (min.y - rayorig.y) / raydir.y;
            if (t >= 0)
            {
                // Substitute t back into ray and check bounds and dist
                hitpoint = rayorig + raydir * t;
                if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
                    hitpoint.z >= min.z && hitpoint.z <= max.z &&
                    (!hit || t < lowt))
                {
                    hit = true;
                    lowt = t;
                }
            }
        }
        // Max y
        if (rayorig.y >= max.y && raydir.y < 0)
        {
            t = (max.y - rayorig.y) / raydir.y;
            if (t >= 0)
            {
                // Substitute t back into ray and check bounds and dist
                hitpoint = rayorig + raydir * t;
                if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
                    hitpoint.z >= min.z && hitpoint.z <= max.z &&
                    (!hit || t < lowt))
                {
                    hit = true;
                    lowt = t;
                }
            }
        }
        // Min z
        if (rayorig.z <= min.z && raydir.z > 0)
        {
            t = (min.z - rayorig.z) / raydir.z;
            if (t >= 0)
            {
                // Substitute t back into ray and check bounds and dist
                hitpoint = rayorig + raydir * t;
                if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
                    hitpoint.y >= min.y && hitpoint.y <= max.y &&
                    (!hit || t < lowt))
                {
                    hit = true;
                    lowt = t;
                }
            }
        }
        // Max z
        if (rayorig.z >= max.z && raydir.z < 0)
        {
            t = (max.z - rayorig.z) / raydir.z;
            if (t >= 0)
            {
                // Substitute t back into ray and check bounds and dist
                hitpoint = rayorig + raydir * t;
                if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
                    hitpoint.y >= min.y && hitpoint.y <= max.y &&
                    (!hit || t < lowt))
                {
                    hit = true;
                    lowt = t;
                }
            }
        }

        return std::pair<bool, float>(hit, lowt);
    } 

    std::pair<bool, float> Math::intersects(const Ray& ray, const Sphere& sphere, 
        bool discardInside)
    {
        const Vector3& raydir = ray.getDirection();
        // Adjust ray origin relative to sphere center
        const Vector3& rayorig = ray.getOrigin() - sphere.getCenter();
        float radius = sphere.getRadius();

        // Check origin inside first
        if (rayorig.squaredLength() <= radius*radius && discardInside)
        {
            return std::pair<bool, float>(true, (float)0);
        }

        // Mmm, quadratics
        // Build coeffs which can be used with std quadratic solver
        // ie t = (-b +/- sqrt(b*b + 4ac)) / 2a
        float a = raydir.dotProduct(raydir);
        float b = 2 * rayorig.dotProduct(raydir);
        float c = rayorig.dotProduct(rayorig) - radius*radius;

        // Calc determinant
        float d = (b*b) - (4 * a * c);
        if (d < 0)
        {
            // No intersection
            return std::pair<bool, float>(false, (float)0);
        }
        else
        {
            // BTW, if d=0 there is one intersection, if d > 0 there are 2
            // But we only want the closest one, so that's ok, just use the 
            // '-' version of the solver
            float t = ( -b - Math::sqrt(d) ) / (2 * a);
            if (t < 0)
                t = ( -b + Math::sqrt(d) ) / (2 * a);
            return std::pair<bool, float>(true, t);
        }
    }

    template<class T>
    std::pair<bool, float> _intersects(const Ray& ray, const T& planes, bool normalIsOutside)
    {
        typename T::const_iterator planeit, planeitend;
        planeitend = planes.end();
        bool allInside = true;
        std::pair<bool, float> ret;
        std::pair<bool, float> end;
        ret.first = false;
        ret.second = 0.0f;
        end.first = false;
        end.second = 0;


        // derive side
        // NB we don't pass directly since that would require Plane::Side in 
        // interface, which results in recursive includes since Math is so fundamental
        Plane::Side outside = normalIsOutside ? Plane::POSITIVE_SIDE : Plane::NEGATIVE_SIDE;

        for (planeit = planes.begin(); planeit != planeitend; ++planeit)
        {
            const Plane& plane = *planeit;
            // is origin outside?
            if (plane.getSide(ray.getOrigin()) == outside)
            {
                allInside = false;
                // Test single plane
                std::pair<bool, float> planeRes = 
                    ray.intersects(plane);
                if (planeRes.first)
                {
                    // Ok, we intersected
                    ret.first = true;
                    // Use the most distant result since convex volume
                    ret.second = std::max(ret.second, planeRes.second);
                }
                else
                {
                    ret.first =false;
                    ret.second=0.0f;
                    return ret;
                }
            }
            else
            {
                std::pair<bool, float> planeRes = 
                    ray.intersects(plane);
                if (planeRes.first)
                {
                    if( !end.first )
                    {
                        end.first = true;
                        end.second = planeRes.second;
                    }
                    else
                    {
                        end.second = std::min( planeRes.second, end.second );
                    }

                }

            }
        }

        if (allInside)
        {
            // Intersecting at 0 distance since inside the volume!
            ret.first = true;
            ret.second = 0.0f;
            return ret;
        }

        if( end.first )
        {
            if( end.second < ret.second )
            {
                ret.first = false;
                return ret;
            }
        }
        return ret;
    }

    std::pair<bool, float> Math::intersects( const Ray& ray, const vector<Plane>::type& planeList, bool normalIsOutside )
    {
        return _intersects( ray, planeList, normalIsOutside );
    }

    std::pair<bool, float> Math::intersects( const Ray& ray, const list<Plane>::type& planeList, bool normalIsOutside )
    {
        return _intersects( ray, planeList, normalIsOutside );
    }

    bool Math::rayIntersectsTriangle( const Ray& ray, const Vector3& v0, const Vector3& v1, const Vector3& v2,
        float* pt, Vector3* gravity )
    {
        // 通过射线(orig, dir)，三角形(v0,v1,v2)求交点的三角形重心坐标(u,v)，和射线原点orig到交点的距离t
        // p = (1 - u - v) * v0 + u * v1 + v * v2
        // 且 p = orig + t * dir （射线方程）
        // 所以 (1 - u - v) * v0 + u * v1 + v * v2 = orig + t * dir
        // 整理得： -dir * t + (v1 -v0) * u + (v2 - v0) * v = (orig - v0)
        // 设 e1 = v1 - v0, e2 = v2 - v0, q = orig - v0, nd = -dir
        // 即 nd * t + e1 * u + e2 * v = q
        // 根据克莱姆法则解方程，
        // 先求行列式 D = | nd e1 e2 |，由于一个行列式的值和它的转置行列式值相同，所以
        //          | nd |
        // D = D' = | e1 | = (e1 × e2)．nd （其实就是先把nd看成(i,j, k)，叉乘求得的向量与nd再点乘）
        //          | e2 |
        // 如果D != 0 那么方程有解：
        // t = Dt / D = | q   e1  e2  | / D = (e1 × e2)．q   / D
        // u = Du / D = | nd  q   e2  | / D = (q  × e2)．nd  / D
        // v = Dv / D = | nd  e1  q   | / D = (e1 ×  q)．nd  / D
        // 当 u,v在[0, 1]，t >= 0则合法

        Vector3 e1 = v1 - v0;
        Vector3 e2 = v2 - v0;
        Vector3 nd = -ray.getDirection();

        // 求D
        Vector3 e1_e2 = e1.crossProduct( e2 );
        float   D     = e1_e2.dotProduct( nd );

        if ( Math::fabs(D) < 1e-4f )
            return false;

        // 求Dt, t
        Vector3 q  = ray.getOrigin() - v0;
        float   Dt = e1_e2.dotProduct( q );
        float   t  = Dt / D;

        if ( t < 0 )
            return false;

        // 求Du, u
        Vector3 q_e2 = q.crossProduct( e2 );
        float   Du   = q_e2.dotProduct( nd );
        float   u    = Du / D;

        if ( u < 0 || u > 1 )
            return false;

        // 求Dv, v
        Vector3 e1_q = e1.crossProduct( q );
        float   Dv   = e1_q.dotProduct( nd );
        float   v    = Dv / D;

        if ( v < 0 || v > 1 )
            return false;

        // 检查w
        float w = 1 - u - v;
        if ( w < 0 || w > 1 )
            return false;

        if ( pt ) *pt = t;
        
        if ( gravity )
        {
            gravity->x = w; // 注意：这里假定的顺序
            gravity->y = u;
            gravity->z = v;
        }

        return true;
    }


    //////////////////////////////////////////////////////////////////////////
    
#define POS1	122
#define SL1	18
#define SL2	1
#define SR1	11
#define SR2	1
#define MSK1	0xdfffffefU
#define MSK2	0xddfecb7fU
#define MSK3	0xbffaffffU
#define MSK4	0xbffffff6U
#define PARITY1	0x00000001U
#define PARITY2	0x00000000U
#define PARITY3	0x00000000U
#define PARITY4	0x13c9e684U
#define IDSTR	"SFMT-19937:122-18-1-11-1:dfffffef-ddfecb7f-bffaffff-bffffff6"

/** a parity check vector which certificate the period of 2^{MEXP} */
static const uint32_t parity[4] = {PARITY1, PARITY2, PARITY3, PARITY4};

/*----------------
  STATIC FUNCTIONS
  ----------------*/
inline static int idxof(int i);
inline static void rshift128(w128_t *out,  w128_t const *in, int shift);
inline static void lshift128(w128_t *out,  w128_t const *in, int shift);
inline static uint32_t func1(uint32_t x);
inline static uint32_t func2(uint32_t x);
#if defined(BIG_ENDIAN64) && !defined(ONLY64)
inline static void swap(w128_t *array, int size);
#endif

#if USE_SSE2_FOR_MERSENNE_TWISTER
/** 
 * @file  SFMT-sse2.h
 * @brief SIMD oriented Fast Mersenne Twister(SFMT) for Intel SSE2
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (Hiroshima University)
 *
 * @note We assume LITTLE ENDIAN in this file
 *
 * Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University. All rights reserved.
 *
 * The new BSD License is applied to this software, see LICENSE.txt
 */

#ifndef SFMT_SSE2_H
#define SFMT_SSE2_H

inline static __m128i mm_recursion(__m128i *a, __m128i *b, __m128i c,
				   __m128i d, __m128i mask);

/**
 * This function represents the recursion formula.
 * @param a a 128-bit part of the interal state array
 * @param b a 128-bit part of the interal state array
 * @param c a 128-bit part of the interal state array
 * @param d a 128-bit part of the interal state array
 * @param mask 128-bit mask
 * @return output
 */
inline static __m128i mm_recursion(__m128i *a, __m128i *b, 
				   __m128i c, __m128i d, __m128i mask) {
    __m128i v, x, y, z;
    
    x = _mm_load_si128(a);
    y = _mm_srli_epi32(*b, SR1);
    z = _mm_srli_si128(c, SR2);
    v = _mm_slli_epi32(d, SL1);
    z = _mm_xor_si128(z, x);
    z = _mm_xor_si128(z, v);
    x = _mm_slli_si128(x, SL2);
    y = _mm_and_si128(y, mask);
    z = _mm_xor_si128(z, x);
    z = _mm_xor_si128(z, y);
    return z;
}

/**
 * This function fills the internal state array with pseudorandom
 * integers.
 */
inline void FLMRandomStream::gen_rand_all(void) {
    int i;
    __m128i r, r1, r2, mask;
    mask = _mm_set_epi32(MSK4, MSK3, MSK2, MSK1);

    r1 = _mm_load_si128(&sfmt[N - 2].si);
    r2 = _mm_load_si128(&sfmt[N - 1].si);
    for (i = 0; i < N - POS1; i++) {
	r = mm_recursion(&sfmt[i].si, &sfmt[i + POS1].si, r1, r2, mask);
	_mm_store_si128(&sfmt[i].si, r);
	r1 = r2;
	r2 = r;
    }
    for (; i < N; i++) {
	r = mm_recursion(&sfmt[i].si, &sfmt[i + POS1 - N].si, r1, r2, mask);
	_mm_store_si128(&sfmt[i].si, r);
	r1 = r2;
	r2 = r;
    }
}

/**
 * This function fills the user-specified array with pseudorandom
 * integers.
 *
 * @param array an 128-bit array to be filled by pseudorandom numbers.  
 * @param size number of 128-bit pesudorandom numbers to be generated.
 */
inline void FLMRandomStream::gen_rand_array(w128_t *array, int size) {
    int i, j;
    __m128i r, r1, r2, mask;
    mask = _mm_set_epi32(MSK4, MSK3, MSK2, MSK1);

    r1 = _mm_load_si128(&sfmt[N - 2].si);
    r2 = _mm_load_si128(&sfmt[N - 1].si);
    for (i = 0; i < N - POS1; i++) {
	r = mm_recursion(&sfmt[i].si, &sfmt[i + POS1].si, r1, r2, mask);
	_mm_store_si128(&array[i].si, r);
	r1 = r2;
	r2 = r;
    }
    for (; i < N; i++) {
	r = mm_recursion(&sfmt[i].si, &array[i + POS1 - N].si, r1, r2, mask);
	_mm_store_si128(&array[i].si, r);
	r1 = r2;
	r2 = r;
    }
    /* main loop */
    for (; i < size - N; i++) {
	r = mm_recursion(&array[i - N].si, &array[i + POS1 - N].si, r1, r2,
			 mask);
	_mm_store_si128(&array[i].si, r);
	r1 = r2;
	r2 = r;
    }
    for (j = 0; j < 2 * N - size; j++) {
	r = _mm_load_si128(&array[j + size - N].si);
	_mm_store_si128(&sfmt[j].si, r);
    }
    for (; i < size; i++) {
	r = mm_recursion(&array[i - N].si, &array[i + POS1 - N].si, r1, r2,
			 mask);
	_mm_store_si128(&array[i].si, r);
	_mm_store_si128(&sfmt[j++].si, r);
	r1 = r2;
	r2 = r;
    }
}

#endif

#endif

/**
 * This function simulate a 64-bit index of LITTLE ENDIAN 
 * in BIG ENDIAN machine.
 */
#ifdef ONLY64
inline static int idxof(int i) {
    return i ^ 1;
}
#else
inline static int idxof(int i) {
    return i;
}
#endif
/**
 * This function simulates SIMD 128-bit right shift by the standard C.
 * The 128-bit integer given in in is shifted by (shift * 8) bits.
 * This function simulates the LITTLE ENDIAN SIMD.
 * @param out the output of this function
 * @param in the 128-bit data to be shifted
 * @param shift the shift value
 */
#ifdef ONLY64
inline static void rshift128(w128_t *out, w128_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u[2] << 32) | ((uint64_t)in->u[3]);
    tl = ((uint64_t)in->u[0] << 32) | ((uint64_t)in->u[1]);

    oh = th >> (shift * 8);
    ol = tl >> (shift * 8);
    ol |= th << (64 - shift * 8);
    out->u[0] = (uint32_t)(ol >> 32);
    out->u[1] = (uint32_t)ol;
    out->u[2] = (uint32_t)(oh >> 32);
    out->u[3] = (uint32_t)oh;
}
#else
inline static void rshift128(w128_t *out, w128_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u[3] << 32) | ((uint64_t)in->u[2]);
    tl = ((uint64_t)in->u[1] << 32) | ((uint64_t)in->u[0]);

    oh = th >> (shift * 8);
    ol = tl >> (shift * 8);
    ol |= th << (64 - shift * 8);
    out->u[1] = (uint32_t)(ol >> 32);
    out->u[0] = (uint32_t)ol;
    out->u[3] = (uint32_t)(oh >> 32);
    out->u[2] = (uint32_t)oh;
}
#endif
/**
 * This function simulates SIMD 128-bit left shift by the standard C.
 * The 128-bit integer given in in is shifted by (shift * 8) bits.
 * This function simulates the LITTLE ENDIAN SIMD.
 * @param out the output of this function
 * @param in the 128-bit data to be shifted
 * @param shift the shift value
 */
#ifdef ONLY64
inline static void lshift128(w128_t *out, w128_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u[2] << 32) | ((uint64_t)in->u[3]);
    tl = ((uint64_t)in->u[0] << 32) | ((uint64_t)in->u[1]);

    oh = th << (shift * 8);
    ol = tl << (shift * 8);
    oh |= tl >> (64 - shift * 8);
    out->u[0] = (uint32_t)(ol >> 32);
    out->u[1] = (uint32_t)ol;
    out->u[2] = (uint32_t)(oh >> 32);
    out->u[3] = (uint32_t)oh;
}
#else
inline static void lshift128(w128_t *out, w128_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u[3] << 32) | ((uint64_t)in->u[2]);
    tl = ((uint64_t)in->u[1] << 32) | ((uint64_t)in->u[0]);

    oh = th << (shift * 8);
    ol = tl << (shift * 8);
    oh |= tl >> (64 - shift * 8);
    out->u[1] = (uint32_t)(ol >> 32);
    out->u[0] = (uint32_t)ol;
    out->u[3] = (uint32_t)(oh >> 32);
    out->u[2] = (uint32_t)oh;
}
#endif

/**
 * This function represents the recursion formula.
 * @param r output
 * @param a a 128-bit part of the internal state array
 * @param b a 128-bit part of the internal state array
 * @param c a 128-bit part of the internal state array
 * @param d a 128-bit part of the internal state array
 */
#if !USE_SSE2_FOR_MERSENNE_TWISTER
#ifdef ONLY64
inline static void do_recursion(w128_t *r, w128_t *a, w128_t *b, w128_t *c,
				w128_t *d) {
    w128_t x;
    w128_t y;

    lshift128(&x, a, SL2);
    rshift128(&y, c, SR2);
    r->u[0] = a->u[0] ^ x.u[0] ^ ((b->u[0] >> SR1) & MSK2) ^ y.u[0] 
	^ (d->u[0] << SL1);
    r->u[1] = a->u[1] ^ x.u[1] ^ ((b->u[1] >> SR1) & MSK1) ^ y.u[1] 
	^ (d->u[1] << SL1);
    r->u[2] = a->u[2] ^ x.u[2] ^ ((b->u[2] >> SR1) & MSK4) ^ y.u[2] 
	^ (d->u[2] << SL1);
    r->u[3] = a->u[3] ^ x.u[3] ^ ((b->u[3] >> SR1) & MSK3) ^ y.u[3] 
	^ (d->u[3] << SL1);
}
#else
inline static void do_recursion(w128_t *r, w128_t *a, w128_t *b, w128_t *c,
				w128_t *d) {
    w128_t x;
    w128_t y;

    lshift128(&x, a, SL2);
    rshift128(&y, c, SR2);
    r->u[0] = a->u[0] ^ x.u[0] ^ ((b->u[0] >> SR1) & MSK1) ^ y.u[0] 
	^ (d->u[0] << SL1);
    r->u[1] = a->u[1] ^ x.u[1] ^ ((b->u[1] >> SR1) & MSK2) ^ y.u[1] 
	^ (d->u[1] << SL1);
    r->u[2] = a->u[2] ^ x.u[2] ^ ((b->u[2] >> SR1) & MSK3) ^ y.u[2] 
	^ (d->u[2] << SL1);
    r->u[3] = a->u[3] ^ x.u[3] ^ ((b->u[3] >> SR1) & MSK4) ^ y.u[3] 
	^ (d->u[3] << SL1);
}
#endif
#endif

#if !USE_SSE2_FOR_MERSENNE_TWISTER
/**
 * This function fills the internal state array with pseudorandom
 * integers.
 */
void FLMRandomStream::gen_rand_all(void) {
    int i;
    w128_t *r1, *r2;

    r1 = &sfmt[N - 2];
    r2 = &sfmt[N - 1];
    for (i = 0; i < N - POS1; i++) {
	do_recursion(&sfmt[i], &sfmt[i], &sfmt[i + POS1], r1, r2);
	r1 = r2;
	r2 = &sfmt[i];
    }
    for (; i < N; i++) {
	do_recursion(&sfmt[i], &sfmt[i], &sfmt[i + POS1 - N], r1, r2);
	r1 = r2;
	r2 = &sfmt[i];
    }
}

/**
 * This function fills the user-specified array with pseudorandom
 * integers.
 *
 * @param array an 128-bit array to be filled by pseudorandom numbers.  
 * @param size number of 128-bit pseudorandom numbers to be generated.
 */
void FLMRandomStream::gen_rand_array(w128_t *array, int size) {
    int i, j;
    w128_t *r1, *r2;

    r1 = &sfmt[N - 2];
    r2 = &sfmt[N - 1];
    for (i = 0; i < N - POS1; i++) {
	do_recursion(&array[i], &sfmt[i], &sfmt[i + POS1], r1, r2);
	r1 = r2;
	r2 = &array[i];
    }
    for (; i < N; i++) {
	do_recursion(&array[i], &sfmt[i], &array[i + POS1 - N], r1, r2);
	r1 = r2;
	r2 = &array[i];
    }
    for (; i < size - N; i++) {
	do_recursion(&array[i], &array[i - N], &array[i + POS1 - N], r1, r2);
	r1 = r2;
	r2 = &array[i];
    }
    for (j = 0; j < 2 * N - size; j++) {
	sfmt[j] = array[j + size - N];
    }
    for (; i < size; i++, j++) {
	do_recursion(&array[i], &array[i - N], &array[i + POS1 - N], r1, r2);
	r1 = r2;
	r2 = &array[i];
	sfmt[j] = array[i];
    }
}
#endif

#if defined(BIG_ENDIAN64) && !defined(ONLY64) && !defined(HAVE_ALTIVEC)
inline static void swap(w128_t *array, int size) {
    int i;
    uint32_t x, y;

    for (i = 0; i < size; i++) {
	x = array[i].u[0];
	y = array[i].u[2];
	array[i].u[0] = array[i].u[1];
	array[i].u[2] = array[i].u[3];
	array[i].u[1] = x;
	array[i].u[3] = y;
    }
}
#endif
/**
 * This function represents a function used in the initialization
 * by init_by_array
 * @param x 32-bit integer
 * @return 32-bit integer
 */
static uint32_t func1(uint32_t x) {
    return (x ^ (x >> 27)) * (uint32_t)1664525UL;
}

/**
 * This function represents a function used in the initialization
 * by init_by_array
 * @param x 32-bit integer
 * @return 32-bit integer
 */
static uint32_t func2(uint32_t x) {
    return (x ^ (x >> 27)) * (uint32_t)1566083941UL;
}

/**
 * This function certificate the period of 2^{MEXP}
 */
void FLMRandomStream::period_certification(void) {
    int inner = 0;
    int i, j;
    uint32_t work;

    for (i = 0; i < 4; i++)
	inner ^= psfmt32[idxof(i)] & parity[i];
    for (i = 16; i > 0; i >>= 1)
	inner ^= inner >> i;
    inner &= 1;
    /* check OK */
    if (inner == 1) {
	return;
    }
    /* check NG, and modification */
    for (i = 0; i < 4; i++) {
	work = 1;
	for (j = 0; j < 32; j++) {
	    if ((work & parity[i]) != 0) {
		psfmt32[idxof(i)] ^= work;
		return;
	    }
	    work = work << 1;
	}
    }
}

/*----------------
  PUBLIC FUNCTIONS
  ----------------*/
/**
 * This function returns the identification string.
 * The string shows the word size, the Mersenne exponent,
 * and all parameters of this generator.
 */
const char * FLMRandomStream::get_idstring(void) {
    return IDSTR;
}

/**
 * This function returns the minimum size of array used for \b
 * fill_array32() function.
 * @return minimum size of array used for fill_array32() function.
 */
int FLMRandomStream::get_min_array_size32(void) {
    return N32;
}

/**
 * This function returns the minimum size of array used for \b
 * fill_array64() function.
 * @return minimum size of array used for fill_array64() function.
 */
int FLMRandomStream::get_min_array_size64(void) {
    return N64;
}

#ifndef ONLY64
/**
 * This function generates and returns 32-bit pseudorandom number.
 * init_gen_rand or init_by_array must be called before this function.
 * @return 32-bit pseudorandom number
 */
uint32_t FLMRandomStream::gen_rand32(void) {
    uint32_t r;

    assert(initialized);
    if (idx >= N32) {
	gen_rand_all();
	idx = 0;
    }
    r = psfmt32[idx++];
    return r;
}
#endif
/**
 * This function generates and returns 64-bit pseudorandom number.
 * init_gen_rand or init_by_array must be called before this function.
 * The function gen_rand64 should not be called after gen_rand32,
 * unless an initialization is again executed. 
 * @return 64-bit pseudorandom number
 */
uint64_t FLMRandomStream::gen_rand64(void) {
#if defined(BIG_ENDIAN64) && !defined(ONLY64)
    uint32_t r1, r2;
#else
    uint64_t r;
#endif

    assert(initialized);
    assert(idx % 2 == 0);

    if (idx >= N32) {
	gen_rand_all();
	idx = 0;
    }
#if defined(BIG_ENDIAN64) && !defined(ONLY64)
    r1 = psfmt32[idx];
    r2 = psfmt32[idx + 1];
    idx += 2;
    return ((uint64_t)r2 << 32) | r1;
#else
    r = psfmt64[idx / 2];
    idx += 2;
    return r;
#endif
}

#ifndef ONLY64
/**
 * This function generates pseudorandom 32-bit integers in the
 * specified array[] by one call. The number of pseudorandom integers
 * is specified by the argument size, which must be at least 624 and a
 * multiple of four.  The generation by this function is much faster
 * than the following gen_rand function.
 *
 * For initialization, init_gen_rand or init_by_array must be called
 * before the first call of this function. This function can not be
 * used after calling gen_rand function, without initialization.
 *
 * @param array an array where pseudorandom 32-bit integers are filled
 * by this function.  The pointer to the array must be \b "aligned"
 * (namely, must be a multiple of 16) in the SIMD version, since it
 * refers to the address of a 128-bit integer.  In the standard C
 * version, the pointer is arbitrary.
 *
 * @param size the number of 32-bit pseudorandom integers to be
 * generated.  size must be a multiple of 4, and greater than or equal
 * to (MEXP / 128 + 1) * 4.
 *
 * @note \b memalign or \b posix_memalign is available to get aligned
 * memory. Mac OSX doesn't have these functions, but \b malloc of OSX
 * returns the pointer to the aligned memory block.
 */
void FLMRandomStream::fill_array32(uint32_t *array, int size) {
    assert(initialized);
    assert(idx == N32);
    assert(size % 4 == 0);
    assert(size >= N32);

    gen_rand_array((w128_t *)array, size / 4);
    idx = N32;
}
#endif

/**
 * This function generates pseudorandom 64-bit integers in the
 * specified array[] by one call. The number of pseudorandom integers
 * is specified by the argument size, which must be at least 312 and a
 * multiple of two.  The generation by this function is much faster
 * than the following gen_rand function.
 *
 * For initialization, init_gen_rand or init_by_array must be called
 * before the first call of this function. This function can not be
 * used after calling gen_rand function, without initialization.
 *
 * @param array an array where pseudorandom 64-bit integers are filled
 * by this function.  The pointer to the array must be "aligned"
 * (namely, must be a multiple of 16) in the SIMD version, since it
 * refers to the address of a 128-bit integer.  In the standard C
 * version, the pointer is arbitrary.
 *
 * @param size the number of 64-bit pseudorandom integers to be
 * generated.  size must be a multiple of 2, and greater than or equal
 * to (MEXP / 128 + 1) * 2
 *
 * @note \b memalign or \b posix_memalign is available to get aligned
 * memory. Mac OSX doesn't have these functions, but \b malloc of OSX
 * returns the pointer to the aligned memory block.
 */
void FLMRandomStream::fill_array64(uint64_t *array, int size) {
    assert(initialized);
    assert(idx == N32);
    assert(size % 2 == 0);
    assert(size >= N64);

    gen_rand_array((w128_t *)array, size / 2);
    idx = N32;

#if defined(BIG_ENDIAN64) && !defined(ONLY64)
    swap((w128_t *)array, size /2);
#endif
}

/**
 * This function initializes the internal state array with a 32-bit
 * integer seed.
 *
 * @param seed a 32-bit integer used as the seed.
 */
void FLMRandomStream::init_gen_rand(uint32_t seed) {
    int i;

    psfmt32[idxof(0)] = seed;
    for (i = 1; i < N32; i++) {
	psfmt32[idxof(i)] = 1812433253UL * (psfmt32[idxof(i - 1)] 
					    ^ (psfmt32[idxof(i - 1)] >> 30))
	    + i;
    }
    idx = N32;
    period_certification();
    initialized = 1;
}

/**
 * This function initializes the internal state array,
 * with an array of 32-bit integers used as the seeds
 * @param init_key the array of 32-bit integers, used as a seed.
 * @param key_length the length of init_key.
 */
void FLMRandomStream::init_by_array(uint32_t *init_key, int key_length) {
    int i, j, count;
    uint32_t r;
    int lag;
    int mid;
    int size = N * 4;

    if (size >= 623) {
	lag = 11;
    } else if (size >= 68) {
	lag = 7;
    } else if (size >= 39) {
	lag = 5;
    } else {
	lag = 3;
    }
    mid = (size - lag) / 2;

    memset(sfmt, 0x8b, sizeof(sfmt));
    if (key_length + 1 > N32) {
	count = key_length + 1;
    } else {
	count = N32;
    }
    r = func1(psfmt32[idxof(0)] ^ psfmt32[idxof(mid)] 
	      ^ psfmt32[idxof(N32 - 1)]);
    psfmt32[idxof(mid)] += r;
    r += key_length;
    psfmt32[idxof(mid + lag)] += r;
    psfmt32[idxof(0)] = r;

    count--;
    for (i = 1, j = 0; (j < count) && (j < key_length); j++) {
	r = func1(psfmt32[idxof(i)] ^ psfmt32[idxof((i + mid) % N32)] 
		  ^ psfmt32[idxof((i + N32 - 1) % N32)]);
	psfmt32[idxof((i + mid) % N32)] += r;
	r += init_key[j] + i;
	psfmt32[idxof((i + mid + lag) % N32)] += r;
	psfmt32[idxof(i)] = r;
	i = (i + 1) % N32;
    }
    for (; j < count; j++) {
	r = func1(psfmt32[idxof(i)] ^ psfmt32[idxof((i + mid) % N32)] 
		  ^ psfmt32[idxof((i + N32 - 1) % N32)]);
	psfmt32[idxof((i + mid) % N32)] += r;
	r += i;
	psfmt32[idxof((i + mid + lag) % N32)] += r;
	psfmt32[idxof(i)] = r;
	i = (i + 1) % N32;
    }
    for (j = 0; j < N32; j++) {
	r = func2(psfmt32[idxof(i)] + psfmt32[idxof((i + mid) % N32)] 
		  + psfmt32[idxof((i + N32 - 1) % N32)]);
	psfmt32[idxof((i + mid) % N32)] ^= r;
	r -= i;
	psfmt32[idxof((i + mid + lag) % N32)] ^= r;
	psfmt32[idxof(i)] = r;
	i = (i + 1) % N32;
    }

    idx = N32;
    period_certification();
    initialized = 1;
}
    //////////////////////////////////////////////////////////////////////////
}

