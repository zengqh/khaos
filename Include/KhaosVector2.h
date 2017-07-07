#pragma once
#include "KhaosMath.h"

namespace Khaos
{
    class Vector2
    {
    public:
        float x, y;

    public:
        Vector2() : x(0), y(0)
        {
        }

        Vector2( const float fX, const float fY ) : x(fX), y(fY)
        {
        }

        explicit Vector2( const float scaler ) : x(scaler), y(scaler)
        {
        }

        explicit Vector2( const float afCoordinate[2] ) : x(afCoordinate[0]), y(afCoordinate[1])
        {
        }

        explicit Vector2( const int afCoordinate[2] ) : x((float)afCoordinate[0]), y((float)afCoordinate[1])
        {
        }

    public:
		void swap( Vector2& other )
		{
			std::swap( x, other.x );
			std::swap( y, other.y );
		}

        void setValue( float x, float y )
        {
            this->x = x;
            this->y = y;
        }

        void setValue( float t )
        {
            x = t;
            y = t;
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
            khaosAssert( i < 2 );
            return *(&x+i);
        }

		float& operator[]( const size_t i )
        {
            khaosAssert( i < 2 );
            return *(&x+i);
        }

        Vector2& operator=( const Vector2& rkVector )
        {
            x = rkVector.x;
            y = rkVector.y;
            return *this;
        }

		Vector2& operator=( const float fScalar )
		{
			x = fScalar;
			y = fScalar;
			return *this;
		}

        bool operator==( const Vector2& rkVector ) const
        {
            return (x == rkVector.x && y == rkVector.y);
        }

        bool operator!=( const Vector2& rkVector ) const
        {
            return (x != rkVector.x || y != rkVector.y);
        }

        bool operator<( const Vector2& rhs ) const
        {
            if ( x < rhs.x && y < rhs.y )
                return true;
            return false;
        }

        bool operator>( const Vector2& rhs ) const
        {
            if( x > rhs.x && y > rhs.y )
                return true;
            return false;
        }

        Vector2 operator+( const Vector2& rkVector ) const
        {
            return Vector2(x + rkVector.x, y + rkVector.y);
        }

        Vector2 operator-( const Vector2& rkVector ) const
        {
            return Vector2(x - rkVector.x, y - rkVector.y);
        }

        Vector2 operator*( const float fScalar ) const
        {
            return Vector2(x * fScalar, y * fScalar);
        }

        Vector2 operator*( const Vector2& rhs ) const
        {
            return Vector2( x * rhs.x, y * rhs.y );
        }

        Vector2 operator/( const float fScalar ) const
        {
            khaosAssert( fScalar != (float)0.0 );
            float fInv = 1.0f / fScalar;
            return Vector2(x * fInv, y * fInv);
        }

        Vector2 operator/( const Vector2& rhs ) const
        {
            return Vector2(x / rhs.x, y / rhs.y);
        }

        const Vector2& operator+() const
        {
            return *this;
        }

        Vector2 operator-() const
        {
            return Vector2(-x, -y);
        }

        friend Vector2 operator*( const float fScalar, const Vector2& rkVector )
        {
            return Vector2(fScalar * rkVector.x, fScalar * rkVector.y);
        }

        friend Vector2 operator/( const float fScalar, const Vector2& rkVector )
        {
            return Vector2(fScalar / rkVector.x, fScalar / rkVector.y);
        }

        friend Vector2 operator+( const Vector2& lhs, const float rhs )
        {
            return Vector2(lhs.x + rhs, lhs.y + rhs);
        }

        friend Vector2 operator+( const float lhs, const Vector2& rhs )
        {
            return Vector2(lhs + rhs.x, lhs + rhs.y);
        }

        friend Vector2 operator-( const Vector2& lhs, const float rhs )
        {
            return Vector2(lhs.x - rhs, lhs.y - rhs);
        }

        friend Vector2 operator-( const float lhs, const Vector2& rhs )
        {
            return Vector2(lhs - rhs.x, lhs - rhs.y);
        }

        Vector2& operator+=( const Vector2& rkVector )
        {
            x += rkVector.x;
            y += rkVector.y;
            return *this;
        }

        Vector2& operator+=( const float fScaler )
        {
            x += fScaler;
            y += fScaler;
            return *this;
        }

        Vector2& operator-=( const Vector2& rkVector )
        {
            x -= rkVector.x;
            y -= rkVector.y;
            return *this;
        }

        Vector2& operator-=( const float fScaler )
        {
            x -= fScaler;
            y -= fScaler;
            return *this;
        }

        Vector2& operator*=( const float fScalar )
        {
            x *= fScalar;
            y *= fScalar;
            return *this;
        }

        Vector2& operator*=( const Vector2& rkVector )
        {
            x *= rkVector.x;
            y *= rkVector.y;
            return *this;
        }

        Vector2& operator/=( const float fScalar )
        {
            khaosAssert( fScalar != 0.0 );
            float fInv = 1.0f / fScalar;
            x *= fInv;
            y *= fInv;
            return *this;
        }

        Vector2& operator/=( const Vector2& rkVector )
        {
            x /= rkVector.x;
            y /= rkVector.y;
            return *this;
        }

    public:
        float length () const
        {
            return Math::sqrt( x * x + y * y );
        }

        float squaredLength () const
        {
            return x * x + y * y;
        }

        float distance( const Vector2& rhs ) const
        {
            return (*this - rhs).length();
        }

        float squaredDistance( const Vector2& rhs ) const
        {
            return (*this - rhs).squaredLength();
        }

        float dotProduct( const Vector2& vec ) const
        {
            return x * vec.x + y * vec.y;
        }

        float normalise()
        {
            float fLength = Math::sqrt( x * x + y * y );

            if ( fLength > float(0.0) )
            {
                float fInvLength = (float)1.0 / fLength;
                x *= fInvLength;
                y *= fInvLength;
            }

            return fLength;
        }

        Vector2 midPoint( const Vector2& vec ) const
        {
            return Vector2( (x + vec.x) * 0.5f, (y + vec.y) * 0.5f );
        }

        void makeFloor( const Vector2& cmp )
        {
            if ( cmp.x < x ) x = cmp.x;
            if ( cmp.y < y ) y = cmp.y;
        }

        void makeCeil( const Vector2& cmp )
        {
            if ( cmp.x > x ) x = cmp.x;
            if ( cmp.y > y ) y = cmp.y;
        }

        void makeRange( float t )
        {
            if ( t < x )
                x = t;
            else if ( t > y )
                y = t;
        }

        float getRange() const
        {
            return y - x;
        }

        float getHalfRange() const
        {
            return (y - x) * 0.5f;
        }

        float getMidValue() const
        {
            return (y + x) * 0.5f;
        }

        Vector2 perpendicular() const
        {
            return Vector2(-y, x);
        }

        float crossProduct( const Vector2& rkVector ) const
        {
            return x * rkVector.y - y * rkVector.x;
        }

        Vector2 randomDeviant( float angle ) const
        {
            angle *= Math::unitRandom() * Math::TWO_PI;
            float cosa = Math::cos(angle);
            float sina = Math::sin(angle);
            return  Vector2(cosa * x - sina * y, sina * x + cosa * y);
        }

        bool isZeroLength() const
        {
            float sqlen = (x * x) + (y * y);
            return (sqlen < (1e-06 * 1e-06));

        }

        Vector2 normalisedCopy() const
        {
            Vector2 ret = *this;
            ret.normalise();
            return ret;
        }

        Vector2 reflect( const Vector2& normal ) const
        {
            return Vector2( *this - (2 * this->dotProduct(normal) * normal) );
        }

		bool isNaN() const
		{
			return Math::isNaN(x) || Math::isNaN(y);
		}

		float angleBetween( const Vector2& other ) const
		{		
			float lenProduct = length() * other.length();
			if ( lenProduct < (float)1e-6 )
				lenProduct = (float)1e-6;
		
			float f = dotProduct(other) / lenProduct;
			f = Math::clamp(f, (float)-1.0, (float)1.0);
			return Math::acos(f);
		}

		float angleTo( const Vector2& other ) const
		{
			float angle = angleBetween(other);
			if ( crossProduct(other) < (float)0 )			
				angle = Math::TWO_PI - angle;
			return angle;
		}

    public:
        static const Vector2 ZERO;
        static const Vector2 UNIT_X;
        static const Vector2 UNIT_Y;
        static const Vector2 NEGATIVE_UNIT_X;
        static const Vector2 NEGATIVE_UNIT_Y;
        static const Vector2 UNIT_SCALE;
    };
}

