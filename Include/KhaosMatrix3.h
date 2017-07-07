#pragma once
#include "KhaosMath.h"
#include "KhaosVector3.h"

namespace Khaos
{
    class Matrix3
    {
    public:
		Matrix3() {}

        explicit Matrix3( const float arr[3][3] )
		{
			memcpy(m, arr, 9*sizeof(float));
		}

        Matrix3( const Matrix3& rkMatrix )
		{
			memcpy(m, rkMatrix.m, 9*sizeof(float));
		}

        Matrix3( float fEntry00, float fEntry01, float fEntry02,
                 float fEntry10, float fEntry11, float fEntry12,
                 float fEntry20, float fEntry21, float fEntry22 )
		{
			m[0][0] = fEntry00;
			m[0][1] = fEntry01;
			m[0][2] = fEntry02;
			m[1][0] = fEntry10;
			m[1][1] = fEntry11;
			m[1][2] = fEntry12;
			m[2][0] = fEntry20;
			m[2][1] = fEntry21;
			m[2][2] = fEntry22;
		}

    public:
		void swap( Matrix3& other )
		{
			std::swap(m[0][0], other.m[0][0]);
			std::swap(m[0][1], other.m[0][1]);
			std::swap(m[0][2], other.m[0][2]);
			std::swap(m[1][0], other.m[1][0]);
			std::swap(m[1][1], other.m[1][1]);
			std::swap(m[1][2], other.m[1][2]);
			std::swap(m[2][0], other.m[2][0]);
			std::swap(m[2][1], other.m[2][1]);
			std::swap(m[2][2], other.m[2][2]);
		}

    public:
        float* operator[]( size_t iRow )
        {
            khaosAssert( iRow < 3 );
            return m[iRow];
        }

        const float* operator[]( size_t iRow ) const
		{
            khaosAssert( iRow < 3 );
			return m[iRow];
		}

        Matrix3& operator=(const Matrix3& rkMatrix)
		{
			memcpy(m, rkMatrix.m, 9*sizeof(float));
			return *this;
		}

        bool operator==(const Matrix3& rkMatrix) const;
        bool operator!=(const Matrix3& rkMatrix) const
		{
			return !operator==(rkMatrix);
		}

        Matrix3 operator+(const Matrix3& rkMatrix) const;
        Matrix3 operator-(const Matrix3& rkMatrix) const;
        Matrix3 operator*(const Matrix3& rkMatrix) const;
        Matrix3 operator-() const;

        Vector3 operator*(const Vector3& rkVector) const;
        friend Vector3 operator*(const Vector3& rkVector, const Matrix3& rkMatrix);

        Matrix3 operator*(float fScalar) const;
        friend Matrix3 operator*(float fScalar, const Matrix3& rkMatrix);

    public:
        void makeIdentity()
        {
            *this = IDENTITY;
        }

        Vector3 getColumn(size_t iCol) const;
        void setColumn(size_t iCol, const Vector3& vec);
        void fromAxes(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis);

        Matrix3 transpose() const;
        bool inverse(Matrix3& rkInverse, float fTolerance = 1e-06) const;
        Matrix3 inverse(float fTolerance = 1e-06) const;
        float determinant() const;

        void singularValueDecomposition(Matrix3& rkL, Vector3& rkS, Matrix3& rkR) const;
        void singularValueComposition(const Matrix3& rkL, const Vector3& rkS, const Matrix3& rkR);

        void orthonormalize();
        void qDUDecomposition(Matrix3& rkQ, Vector3& rkD,Vector3& rkU) const;
        float spectralNorm() const;

        void toAngleAxis(Vector3& rkAxis, float& rfAngle) const;
        void fromAngleAxis(const Vector3& rkAxis, float fRadians);

        bool toEulerAnglesXYZ(float& rfYAngle, float& rfPAngle, float& rfRAngle) const;
        bool toEulerAnglesXZY(float& rfYAngle, float& rfPAngle, float& rfRAngle) const;
        bool toEulerAnglesYXZ(float& rfYAngle, float& rfPAngle, float& rfRAngle) const;
        bool toEulerAnglesYZX(float& rfYAngle, float& rfPAngle, float& rfRAngle) const;
        bool toEulerAnglesZXY(float& rfYAngle, float& rfPAngle, float& rfRAngle) const;
        bool toEulerAnglesZYX(float& rfYAngle, float& rfPAngle, float& rfRAngle) const;

        void fromEulerAnglesXYZ(float fYAngle, float fPAngle, float fRAngle);
        void fromEulerAnglesXZY(float fYAngle, float fPAngle, float fRAngle);
        void fromEulerAnglesYXZ(float fYAngle, float fPAngle, float fRAngle);
        void fromEulerAnglesYZX(float fYAngle, float fPAngle, float fRAngle);
        void fromEulerAnglesZXY(float fYAngle, float fPAngle, float fRAngle);
        void fromEulerAnglesZYX(float fYAngle, float fPAngle, float fRAngle);

        void eigenSolveSymmetric(float afEigenvalue[3], Vector3 akEigenvector[3]) const;

        bool hasScale() const
        {
            // check magnitude of column vectors (==local axes)
            float t = m[0][0] * m[0][0] + m[1][0] * m[1][0] + m[2][0] * m[2][0];
            if (!Math::realEqual(t, 1.0, (float)1e-04))
                return true;
            t = m[0][1] * m[0][1] + m[1][1] * m[1][1] + m[2][1] * m[2][1];
            if (!Math::realEqual(t, 1.0, (float)1e-04))
                return true;
            t = m[0][2] * m[0][2] + m[1][2] * m[1][2] + m[2][2] * m[2][2];
            if (!Math::realEqual(t, 1.0, (float)1e-04))
                return true;

            return false;
        }

    public:
        static void tensorProduct(const Vector3& rkU, const Vector3& rkV, Matrix3& rkProduct);

    public:
        static const float EPSILON;
        static const Matrix3 ZERO;
        static const Matrix3 IDENTITY;

    protected:
        // support for eigensolver
        void _tridiagonal(float afDiag[3], float afSubDiag[3]);
        bool _qLAlgorithm(float afDiag[3], float afSubDiag[3]);

        // support for singular value decomposition
        static const float s_svdEpsilon;
        static const unsigned int s_svdMaxIterations;

        static void _bidiagonalize(Matrix3& kA, Matrix3& kL, Matrix3& kR);
        static void _golubKahanStep(Matrix3& kA, Matrix3& kL, Matrix3& kR);

        // support for spectral norm
        static float _maxCubicRoot(float afCoeff[3]);

    protected:
        float m[3][3];

        // for faster access
        friend class Matrix4;
    };
}

