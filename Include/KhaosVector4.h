#pragma once
#include "KhaosVector3.h"

namespace Khaos
{
    class Vector4
    {
    public:
        float x, y, z, w;

    public:
        Vector4() : x(0), y(0), z(0), w(0)
        {
        }

        Vector4( const float fX, const float fY, const float fZ, const float fW )
            : x( fX ), y( fY ), z( fZ ), w( fW )
        {
        }

        explicit Vector4( const float afCoordinate[4] )
            : x( afCoordinate[0] ),
              y( afCoordinate[1] ),
              z( afCoordinate[2] ),
              w( afCoordinate[3] )
        {
        }

        explicit Vector4( const int afCoordinate[4] )
            : x( (float)afCoordinate[0] ),
              y( (float)afCoordinate[1] ),
              z( (float)afCoordinate[2] ),
              w( (float)afCoordinate[3] )
        {
        }

        explicit Vector4( const float scaler )
            : x( scaler )
            , y( scaler )
            , z( scaler )
            , w( scaler )
        {
        }

        explicit Vector4( const Vector3& rhs )
            : x(rhs.x), y(rhs.y), z(rhs.z), w(1.0f)
        {
        }

        explicit Vector4( const Vector3& rhs, float fW )
            : x(rhs.x), y(rhs.y), z(rhs.z), w(fW)
        {
        }

    public:
        void setValue( const Vector3& rhs, float w )
        {
            this->x = rhs.x;
            this->y = rhs.y;
            this->z = rhs.z;
            this->w = w;
        }

        void setValue( const float* v )
        {
            this->x = *v; ++v;
            this->y = *v; ++v;
            this->z = *v; ++v;
            this->w = *v;
        }

        void setValue( float x, float y, float z, float w )
        {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
        }

		void swap(Vector4& other)
		{
			std::swap(x, other.x);
			std::swap(y, other.y);
			std::swap(z, other.z);
			std::swap(w, other.w);
		}
	
        const Vector3& asVec3() const
        {
            return *(Vector3*)&x;
        }

        Vector3& asVec3()
        {
            return *(Vector3*)&x;
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
            khaosAssert( i < 4 );
            return *(&x+i);
        }

		float& operator[]( const size_t i )
        {
            khaosAssert( i < 4 );
            return *(&x+i);
        }

        Vector4& operator=( const Vector4& rkVector )
        {
            x = rkVector.x;
            y = rkVector.y;
            z = rkVector.z;
            w = rkVector.w;

            return *this;
        }

		Vector4& operator=( const float fScalar )
		{
			x = fScalar;
			y = fScalar;
			z = fScalar;
			w = fScalar;
			return *this;
		}

        bool operator==( const Vector4& rkVector ) const
        {
            return ( x == rkVector.x &&
                y == rkVector.y &&
                z == rkVector.z &&
                w == rkVector.w );
        }

        bool operator!=( const Vector4& rkVector ) const
        {
            return ( x != rkVector.x ||
                y != rkVector.y ||
                z != rkVector.z ||
                w != rkVector.w );
        }

        Vector4& operator=(const Vector3& rhs)
        {
            x = rhs.x;
            y = rhs.y;
            z = rhs.z;
            w = 1.0f;
            return *this;
        }

        Vector4 operator+( const Vector4& rkVector ) const
        {
            return Vector4(
                x + rkVector.x,
                y + rkVector.y,
                z + rkVector.z,
                w + rkVector.w);
        }

        Vector4 operator-( const Vector4& rkVector ) const
        {
            return Vector4(
                x - rkVector.x,
                y - rkVector.y,
                z - rkVector.z,
                w - rkVector.w);
        }

        Vector4 operator*( const float fScalar ) const
        {
            return Vector4(
                x * fScalar,
                y * fScalar,
                z * fScalar,
                w * fScalar);
        }

        Vector4 operator*( const Vector4& rhs ) const
        {
            return Vector4(
                rhs.x * x,
                rhs.y * y,
                rhs.z * z,
                rhs.w * w);
        }

        Vector4 operator/( const float fScalar ) const
        {
            khaosAssert( fScalar != (float)0.0 );
            float fInv = 1.0f / fScalar;
            return Vector4(
                x * fInv,
                y * fInv,
                z * fInv,
                w * fInv);
        }

        Vector4 operator/( const Vector4& rhs) const
        {
            return Vector4(
                x / rhs.x,
                y / rhs.y,
                z / rhs.z,
                w / rhs.w);
        }

        const Vector4& operator+() const
        {
            return *this;
        }

        Vector4 operator-() const
        {
            return Vector4(-x, -y, -z, -w);
        }

        friend Vector4 operator*( const float fScalar, const Vector4& rkVector )
        {
            return Vector4(
                fScalar * rkVector.x,
                fScalar * rkVector.y,
                fScalar * rkVector.z,
                fScalar * rkVector.w);
        }

        friend Vector4 operator/( const float fScalar, const Vector4& rkVector )
        {
            return Vector4(
                fScalar / rkVector.x,
                fScalar / rkVector.y,
                fScalar / rkVector.z,
                fScalar / rkVector.w);
        }

        friend Vector4 operator+(const Vector4& lhs, const float rhs)
        {
            return Vector4(
                lhs.x + rhs,
                lhs.y + rhs,
                lhs.z + rhs,
                lhs.w + rhs);
        }

        friend Vector4 operator+(const float lhs, const Vector4& rhs)
        {
            return Vector4(
                lhs + rhs.x,
                lhs + rhs.y,
                lhs + rhs.z,
                lhs + rhs.w);
        }

        friend Vector4 operator-(const Vector4& lhs, float rhs)
        {
            return Vector4(
                lhs.x - rhs,
                lhs.y - rhs,
                lhs.z - rhs,
                lhs.w - rhs);
        }

        friend Vector4 operator-(const float lhs, const Vector4& rhs)
        {
            return Vector4(
                lhs - rhs.x,
                lhs - rhs.y,
                lhs - rhs.z,
                lhs - rhs.w);
        }

        Vector4& operator+=( const Vector4& rkVector )
        {
            x += rkVector.x;
            y += rkVector.y;
            z += rkVector.z;
            w += rkVector.w;

            return *this;
        }

        Vector4& operator-=( const Vector4& rkVector )
        {
            x -= rkVector.x;
            y -= rkVector.y;
            z -= rkVector.z;
            w -= rkVector.w;

            return *this;
        }

        Vector4& operator*=( const float fScalar )
        {
            x *= fScalar;
            y *= fScalar;
            z *= fScalar;
            w *= fScalar;
            return *this;
        }

        Vector4& operator+=( const float fScalar )
        {
            x += fScalar;
            y += fScalar;
            z += fScalar;
            w += fScalar;
            return *this;
        }

        Vector4& operator-=( const float fScalar )
        {
            x -= fScalar;
            y -= fScalar;
            z -= fScalar;
            w -= fScalar;
            return *this;
        }

        Vector4& operator*=( const Vector4& rkVector )
        {
            x *= rkVector.x;
            y *= rkVector.y;
            z *= rkVector.z;
            w *= rkVector.w;

            return *this;
        }

        Vector4& operator/=( const float fScalar )
        {
            khaosAssert( fScalar != (float)0.0 );

            float fInv = 1.0f / fScalar;

            x *= fInv;
            y *= fInv;
            z *= fInv;
            w *= fInv;

            return *this;
        }

        Vector4& operator/=( const Vector4& rkVector )
        {
            x /= rkVector.x;
            y /= rkVector.y;
            z /= rkVector.z;
            w /= rkVector.w;

            return *this;
        }

        float dotProduct( const Vector4& vec ) const
        {
            return x * vec.x + y * vec.y + z * vec.z + w * vec.w;
        }
		
		bool isNaN() const
		{
			return Math::isNaN(x) || Math::isNaN(y) || Math::isNaN(z) || Math::isNaN(w);
		}
     
    public:
        static const Vector4 ZERO;
    };
}

