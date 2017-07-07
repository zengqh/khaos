#pragma once
#include "KhaosMath.h"

namespace Khaos
{
    class Vector3;
    class Matrix3;

    class Quaternion
    {
    public:
		Quaternion () : w(1), x(0), y(0), z(0)
		{
		}

		Quaternion ( float fW, float fX, float fY, float fZ ) : w(fW), x(fX), y(fY), z(fZ)
		{
		}

        Quaternion( const Matrix3& rot )
        {
            this->fromRotationMatrix(rot);
        }

        Quaternion( float rfAngle, const Vector3& rkAxis )
        {
            this->fromAngleAxis(rfAngle, rkAxis);
        }

        Quaternion( const Vector3& xaxis, const Vector3& yaxis, const Vector3& zaxis )
        {
            this->fromAxes(xaxis, yaxis, zaxis);
        }

        Quaternion( const Vector3* akAxis )
        {
            this->fromAxes(akAxis);
        }

		Quaternion( float* valptr )
		{
			memcpy( &w, valptr, sizeof(float)*4 );
		}

    public:
		void swap( Quaternion& other )
		{
			std::swap(w, other.w);
			std::swap(x, other.x);
			std::swap(y, other.y);
			std::swap(z, other.z);
		}

        float* ptr()
        {
            return &w;
        }

        const float* ptr() const
        {
            return &w;
        }

    public:
		float operator[]( const size_t i ) const
		{
			khaosAssert( i < 4 );
			return *(&w+i);
		}

		float& operator[]( const size_t i )
		{
			khaosAssert( i < 4 );
			return *(&w+i);
		}

        Quaternion& operator=( const Quaternion& rkQ )
        {
            w = rkQ.w;
            x = rkQ.x;
            y = rkQ.y;
            z = rkQ.z;
            return *this;
        }

        Quaternion operator+( const Quaternion& rkQ ) const;
        Quaternion operator-( const Quaternion& rkQ ) const;
        Quaternion operator*( const Quaternion& rkQ ) const;
        
        Quaternion operator*( float fScalar ) const;
        friend Quaternion operator*( float fScalar, const Quaternion& rkQ );
        
        Quaternion operator-() const;
        
        bool operator==( const Quaternion& rhs ) const
        {
            return (rhs.x == x) && (rhs.y == y) && (rhs.z == z) && (rhs.w == w);
        }
        
        bool operator!=( const Quaternion& rhs ) const
        {
            return !operator==(rhs);
        }

        Vector3 operator*( const Vector3& rkVector ) const;

    public:
		void fromRotationMatrix( const Matrix3& kRot );
        void toRotationMatrix( Matrix3& kRot ) const;
	
        void fromAngleAxis( float rfAngle, const Vector3& rkAxis );
        void toAngleAxis( float& rfAngle, Vector3& rkAxis ) const;
 
        void fromAxes( const Vector3* akAxis );
        void fromAxes( const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis );

        void toAxes( Vector3* akAxis ) const;
        void toAxes( Vector3& xAxis, Vector3& yAxis, Vector3& zAxis ) const;

        Vector3 xAxis() const;
        Vector3 yAxis() const;
        Vector3 zAxis() const;

        float dot(const Quaternion& rkQ) const;
        float norm() const;
        float normalise(); 
        Quaternion inverse() const;
        Quaternion unitInverse() const;
        Quaternion exp() const;
        Quaternion log() const;

		float getRoll( bool reprojectAxis = true ) const;
		float getPitch( bool reprojectAxis = true ) const;
		float getYaw( bool reprojectAxis = true ) const;		

		bool equals( const Quaternion& rhs, float tolerance ) const;
		
        bool isNaN() const
        {
            return Math::isNaN(x) || Math::isNaN(y) || Math::isNaN(z) || Math::isNaN(w);
        }

    public:
        static Quaternion slerp( float fT, const Quaternion& rkP, const Quaternion& rkQ, bool shortestPath = false );
        static Quaternion slerpExtraSpins( float fT, const Quaternion& rkP, const Quaternion& rkQ, int iExtraSpins );
        static void intermediate( const Quaternion& rkQ0, const Quaternion& rkQ1, const Quaternion& rkQ2, Quaternion& rka, Quaternion& rkB );
        static Quaternion squad( float fT, const Quaternion& rkP, const Quaternion& rkA, const Quaternion& rkB, const Quaternion& rkQ, bool shortestPath = false );
        static Quaternion nlerp( float fT, const Quaternion& rkP, const Quaternion& rkQ, bool shortestPath = false );

    private:
        static const float s_epsilon;

    public:
        static const Quaternion ZERO;
        static const Quaternion IDENTITY;

    public:
		float w, x, y, z;
    };
}

