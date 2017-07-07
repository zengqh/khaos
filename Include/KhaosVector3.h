#pragma once
#include "KhaosMath.h"
#include "KhaosQuaternion.h"

namespace Khaos
{
    class Vector3
    {
    public:
		float x, y, z;

    public:
        Vector3() : x(0), y(0), z(0)
        {
        }

        Vector3( const float fX, const float fY, const float fZ ) : x( fX ), y( fY ), z( fZ )
        {
        }

        explicit Vector3( const float afCoordinate[3] ) : x( afCoordinate[0] ), y( afCoordinate[1] ), z( afCoordinate[2] )
        {
        }

        explicit Vector3( const int afCoordinate[3] ) : x( (float)afCoordinate[0] ), y( (float)afCoordinate[1] ), z( (float)afCoordinate[2] )
        {
        }

        explicit Vector3( const float scaler ) : x( scaler ), y( scaler ), z( scaler )
        {
        }

    public:
        void setValue( const float* v )
        {
            this->x = *v; ++v;
            this->y = *v; ++v;
            this->z = *v;
        }

        void setValue( float x, float y, float z )
        {
            this->x = x;
            this->y = y;
            this->z = z;
        }

		void swap( Vector3& other )
		{
			std::swap(x, other.x);
			std::swap(y, other.y);
			std::swap(z, other.z);
		}

        float* ptr()
        {
            return &x;
        }

        const float* ptr() const
        {
            return &x;
        }

    public:
		float operator[]( const size_t i ) const
        {
            khaosAssert( i < 3 );
            return *(&x+i);
        }

		float& operator[]( const size_t i )
        {
            khaosAssert( i < 3 );
            return *(&x+i);
        }
		
        Vector3& operator=( const Vector3& rkVector )
        {
            x = rkVector.x;
            y = rkVector.y;
            z = rkVector.z;
            return *this;
        }

        Vector3& operator=( const float fScaler )
        {
            x = fScaler;
            y = fScaler;
            z = fScaler;
            return *this;
        }

        bool operator==( const Vector3& rkVector ) const
        {
            return ( x == rkVector.x && y == rkVector.y && z == rkVector.z );
        }

        bool operator!=( const Vector3& rkVector ) const
        {
            return ( x != rkVector.x || y != rkVector.y || z != rkVector.z );
        }

        bool operator<( const Vector3& rhs ) const
        {
            if ( x < rhs.x && y < rhs.y && z < rhs.z )
                return true;
            return false;
        }

        bool operator>( const Vector3& rhs ) const
        {
            if ( x > rhs.x && y > rhs.y && z > rhs.z )
                return true;
            return false;
        }

        Vector3 operator+( const Vector3& rkVector ) const
        {
            return Vector3(x + rkVector.x, y + rkVector.y, z + rkVector.z);
        }

        Vector3 operator-( const Vector3& rkVector ) const
        {
            return Vector3(x - rkVector.x, y - rkVector.y, z - rkVector.z);
        }

        Vector3 operator*( const float fScalar ) const
        {
            return Vector3(x * fScalar, y * fScalar, z * fScalar);
        }

        Vector3 operator*( const Vector3& rhs ) const
        {
            return Vector3(x * rhs.x, y * rhs.y, z * rhs.z);
        }

        Vector3 operator/( const float fScalar ) const
        {
            khaosAssert( fScalar != (float)0.0 );
            float fInv = (float)1.0 / fScalar;
            return Vector3(x * fInv, y * fInv, z * fInv);
        }

        Vector3 operator/( const Vector3& rhs ) const
        {
            return Vector3( x / rhs.x, y / rhs.y, z / rhs.z );
        }

        const Vector3& operator+() const
        {
            return *this;
        }

        Vector3 operator-() const
        {
            return Vector3(-x, -y, -z);
        }

        friend Vector3 operator*( const float fScalar, const Vector3& rkVector )
        {
            return Vector3(fScalar * rkVector.x, fScalar * rkVector.y, fScalar * rkVector.z);
        }

        friend Vector3 operator/( const float fScalar, const Vector3& rkVector )
        {
            return Vector3(fScalar / rkVector.x, fScalar / rkVector.y, fScalar / rkVector.z);
        }

        friend Vector3 operator+( const Vector3& lhs, const float rhs )
        {
            return Vector3(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs);
        }

        friend Vector3 operator+( const float lhs, const Vector3& rhs )
        {
            return Vector3(lhs + rhs.x, lhs + rhs.y, lhs + rhs.z);
        }

        friend Vector3 operator-( const Vector3& lhs, const float rhs )
        {
            return Vector3(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs);
        }

        friend Vector3 operator-( const float lhs, const Vector3& rhs )
        {
            return Vector3(lhs - rhs.x, lhs - rhs.y, lhs - rhs.z);
        }

        Vector3& operator+=( const Vector3& rkVector )
        {
            x += rkVector.x;
            y += rkVector.y;
            z += rkVector.z;
            return *this;
        }

        Vector3& operator+=( const float fScalar )
        {
            x += fScalar;
            y += fScalar;
            z += fScalar;
            return *this;
        }

        Vector3& operator-=( const Vector3& rkVector )
        {
            x -= rkVector.x;
            y -= rkVector.y;
            z -= rkVector.z;
            return *this;
        }

        Vector3& operator-=( const float fScalar )
        {
            x -= fScalar;
            y -= fScalar;
            z -= fScalar;
            return *this;
        }

        Vector3& operator*=( const float fScalar )
        {
            x *= fScalar;
            y *= fScalar;
            z *= fScalar;
            return *this;
        }

        Vector3& operator*=( const Vector3& rkVector )
        {
            x *= rkVector.x;
            y *= rkVector.y;
            z *= rkVector.z;
            return *this;
        }

        Vector3& operator/=( const float fScalar )
        {
            khaosAssert( fScalar != (float)0.0 );
            float fInv = (float)1.0 / fScalar;
            x *= fInv;
            y *= fInv;
            z *= fInv;
            return *this;
        }

        Vector3& operator/=( const Vector3& rkVector )
        {
            x /= rkVector.x;
            y /= rkVector.y;
            z /= rkVector.z;
            return *this;
        }

    public:
        float length () const
        {
            return Math::sqrt( x * x + y * y + z * z );
        }

        float squaredLength () const
        {
            return x * x + y * y + z * z;
        }

        float distance( const Vector3& rhs ) const
        {
            return (*this - rhs).length();
        }

        float squaredDistance( const Vector3& rhs ) const
        {
            return (*this - rhs).squaredLength();
        }

        float dotProduct( const Vector3& vec ) const
        {
            return x * vec.x + y * vec.y + z * vec.z;
        }

        float absDotProduct( const Vector3& vec ) const
        {
            return Math::fabs(x * vec.x) + Math::fabs(y * vec.y) + Math::fabs(z * vec.z);
        }

        float maxValue() const
        {
            return Math::maxVal(Math::maxVal(x, y), z);
        }

        float minValue() const
        {
            return Math::minVal(Math::minVal(x, y), z);
        }

        float normalise()
        {
            float fLength = Math::sqrt( x * x + y * y + z * z );

            if ( fLength > float(0.0) )
            {
                float fInvLength = (float)1.0 / fLength;
                x *= fInvLength;
                y *= fInvLength;
                z *= fInvLength;
            }

            return fLength;
        }

        Vector3 crossProduct( const Vector3& rkVector ) const
        {
            return Vector3( y * rkVector.z - z * rkVector.y, z * rkVector.x - x * rkVector.z, x * rkVector.y - y * rkVector.x );
        }

        Vector3 midPoint( const Vector3& vec ) const
        {
            return Vector3( ( x + vec.x ) * 0.5f, ( y + vec.y ) * 0.5f, ( z + vec.z ) * 0.5f );
        }

        void makeFloor( const Vector3& cmp )
        {
            if ( cmp.x < x ) x = cmp.x;
            if ( cmp.y < y ) y = cmp.y;
            if ( cmp.z < z ) z = cmp.z;
        }

        void makeCeil( const Vector3& cmp )
        {
            if ( cmp.x > x ) x = cmp.x;
            if ( cmp.y > y ) y = cmp.y;
            if ( cmp.z > z ) z = cmp.z;
        }

        Vector3 perpendicular() const
        {
            static const float fSquareZero = (float)(1e-06 * 1e-06);

            Vector3 perp = this->crossProduct( Vector3::UNIT_X );
            if ( perp.squaredLength() < fSquareZero )
            {
                perp = this->crossProduct( Vector3::UNIT_Y );
            }

			perp.normalise();
            return perp;
        }

        Vector3 perpendicular( const Vector3& secondVec ) const
        {
            static const float fSquareZero = (float)(1e-06 * 1e-06);

            Vector3 perp = this->crossProduct( secondVec );
            
            if ( perp.squaredLength() < fSquareZero )
            {
                perp = this->crossProduct( Vector3::UNIT_X );

                if ( perp.squaredLength() < fSquareZero )
                {
                    perp = this->crossProduct( Vector3::UNIT_Y );
                }
            }

            perp.normalise();
            return perp;
        }

        Vector3 randomDeviant( float angle, const Vector3& up = Vector3::ZERO ) const
        {
            Vector3 newUp;

            if ( up == Vector3::ZERO )
            {
                newUp = this->perpendicular();
            }
            else
            {
                newUp = up;
            }

            Quaternion q;
            q.fromAngleAxis( Math::unitRandom() * Math::TWO_PI, *this );
            newUp = q * newUp;
            q.fromAngleAxis( angle, newUp );
            return q * (*this);
        }

		float angleBetween( const Vector3& dest ) const
		{
			float lenProduct = length() * dest.length();
			if ( lenProduct < (float)1e-6 )
				lenProduct = (float)1e-6;

			float f = dotProduct(dest) / lenProduct;
			f = Math::clamp(f, (float)-1.0, (float)1.0);
			return Math::acos(f);
		}

        Quaternion getRotationTo( const Vector3& dest, const Vector3& fallbackAxis = Vector3::ZERO ) const
        {
            // Based on Stan Melax's article in Game Programming Gems
            Quaternion q;
            // Copy, since cannot modify local
            Vector3 v0 = *this;
            Vector3 v1 = dest;
            v0.normalise();
            v1.normalise();

            float d = v0.dotProduct(v1);
            // If dot == 1, vectors are the same
            if (d >= 1.0f)
            {
                return Quaternion::IDENTITY;
            }
			if (d < (1e-6f - 1.0f))
			{
				if (fallbackAxis != Vector3::ZERO)
				{
					// rotate 180 degrees about the fallback axis
					q.fromAngleAxis((Math::PI), fallbackAxis);
				}
				else
				{
					// Generate an axis
					Vector3 axis = Vector3::UNIT_X.crossProduct(*this);
					if (axis.isZeroLength()) // pick another if colinear
						axis = Vector3::UNIT_Y.crossProduct(*this);
					axis.normalise();
					q.fromAngleAxis((Math::PI), axis);
				}
			}
			else
			{
                float s = Math::sqrt( (1+d)*2 );
	            float invs = 1 / s;

				Vector3 c = v0.crossProduct(v1);

    	        q.x = c.x * invs;
        	    q.y = c.y * invs;
            	q.z = c.z * invs;
            	q.w = s * 0.5f;
				q.normalise();
			}
            return q;
        }

        bool isZeroLength() const
        {
            float sqlen = (x * x) + (y * y) + (z * z);
            return (sqlen < (1e-06 * 1e-06));

        }

        Vector3 normalisedCopy(void) const
        {
            Vector3 ret = *this;
            ret.normalise();
            return ret;
        }

        Vector3 reflect( const Vector3& normal ) const
        {
            return Vector3( *this - ( 2 * this->dotProduct(normal) * normal ) );
        }

		bool positionEquals( const Vector3& rhs, float tolerance = (float)1e-03 ) const
		{
			return Math::realEqual(x, rhs.x, tolerance) &&
				Math::realEqual(y, rhs.y, tolerance) &&
				Math::realEqual(z, rhs.z, tolerance);
		}

		bool positionCloses( const Vector3& rhs, float tolerance = (float)1e-03f ) const
		{
			return squaredDistance(rhs) <=
                (squaredLength() + rhs.squaredLength()) * tolerance;
		}

		bool directionEquals( const Vector3& rhs, float tolerance ) const
		{
			float dot = dotProduct(rhs);
			float angle = Math::acos(dot);
			return Math::fabs(angle) <= tolerance;
		}

		bool isNaN() const
		{
			return Math::isNaN(x) || Math::isNaN(y) || Math::isNaN(z);
		}

		Vector3 primaryAxis() const
		{
			float absx = Math::fabs(x);
			float absy = Math::fabs(y);
			float absz = Math::fabs(z);
			if (absx > absy)
				if (absx > absz)
					return x > 0 ? Vector3::UNIT_X : Vector3::NEGATIVE_UNIT_X;
				else
					return z > 0 ? Vector3::UNIT_Z : Vector3::NEGATIVE_UNIT_Z;
			else // absx <= absy
				if (absy > absz)
					return y > 0 ? Vector3::UNIT_Y : Vector3::NEGATIVE_UNIT_Y;
				else
					return z > 0 ? Vector3::UNIT_Z : Vector3::NEGATIVE_UNIT_Z;
		}

    public:
        static const Vector3 ZERO;
        static const Vector3 UNIT_X;
        static const Vector3 UNIT_Y;
        static const Vector3 UNIT_Z;
        static const Vector3 NEGATIVE_UNIT_X;
        static const Vector3 NEGATIVE_UNIT_Y;
        static const Vector3 NEGATIVE_UNIT_Z;
        static const Vector3 UNIT_SCALE;
    };
}

