#include "KhaosPreHeaders.h"
#include "KhaosQuaternion.h"
#include "KhaosMatrix3.h"
#include "KhaosVector3.h"

namespace Khaos
{
    const float Quaternion::s_epsilon = (float)1e-03;
    const Quaternion Quaternion::ZERO(0,0,0,0);
    const Quaternion Quaternion::IDENTITY(1,0,0,0);

    //////////////////////////////////////////////////////////////////////////
    void Quaternion::fromRotationMatrix( const Matrix3& kRot )
    {
        // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
        // article "Quaternion Calculus and Fast Animation".

        float fTrace = kRot[0][0]+kRot[1][1]+kRot[2][2];
        float fRoot;

        if ( fTrace > 0.0 )
        {
            // |w| > 1/2, may as well choose w > 1/2
            fRoot = Math::sqrt(fTrace + 1.0f);  // 2w
            w = 0.5f*fRoot;
            fRoot = 0.5f/fRoot;  // 1/(4w)
            x = (kRot[2][1]-kRot[1][2])*fRoot;
            y = (kRot[0][2]-kRot[2][0])*fRoot;
            z = (kRot[1][0]-kRot[0][1])*fRoot;
        }
        else
        {
            // |w| <= 1/2
            static size_t s_iNext[3] = { 1, 2, 0 };
            size_t i = 0;
            if ( kRot[1][1] > kRot[0][0] )
                i = 1;
            if ( kRot[2][2] > kRot[i][i] )
                i = 2;
            size_t j = s_iNext[i];
            size_t k = s_iNext[j];

            fRoot = Math::sqrt(kRot[i][i]-kRot[j][j]-kRot[k][k] + 1.0f);
            float* apkQuat[3] = { &x, &y, &z };
            *apkQuat[i] = 0.5f*fRoot;
            fRoot = 0.5f/fRoot;
            w = (kRot[k][j]-kRot[j][k])*fRoot;
            *apkQuat[j] = (kRot[j][i]+kRot[i][j])*fRoot;
            *apkQuat[k] = (kRot[k][i]+kRot[i][k])*fRoot;
        }
    }
    
    void Quaternion::toRotationMatrix( Matrix3& kRot ) const
    {
        float fTx  = x+x;
        float fTy  = y+y;
        float fTz  = z+z;
        float fTwx = fTx*w;
        float fTwy = fTy*w;
        float fTwz = fTz*w;
        float fTxx = fTx*x;
        float fTxy = fTy*x;
        float fTxz = fTz*x;
        float fTyy = fTy*y;
        float fTyz = fTz*y;
        float fTzz = fTz*z;

        kRot[0][0] = 1.0f-(fTyy+fTzz);
        kRot[0][1] = fTxy-fTwz;
        kRot[0][2] = fTxz+fTwy;
        kRot[1][0] = fTxy+fTwz;
        kRot[1][1] = 1.0f-(fTxx+fTzz);
        kRot[1][2] = fTyz-fTwx;
        kRot[2][0] = fTxz-fTwy;
        kRot[2][1] = fTyz+fTwx;
        kRot[2][2] = 1.0f-(fTxx+fTyy);
    }
    
    void Quaternion::fromAngleAxis( float rfAngle, const Vector3& rkAxis )
    {
        // assert:  axis[] is unit length
        //
        // The quaternion representing the rotation is
        //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

        float fHalfAngle( (float)0.5*rfAngle );
        float fSin = Math::sin(fHalfAngle);
        w = Math::cos(fHalfAngle);
        x = fSin*rkAxis.x;
        y = fSin*rkAxis.y;
        z = fSin*rkAxis.z;
    }
    
    void Quaternion::toAngleAxis( float& rfAngle, Vector3& rkAxis ) const
    {
        // The quaternion representing the rotation is
        //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

        float fSqrLength = x*x+y*y+z*z;
        if ( fSqrLength > 0.0 )
        {
            rfAngle = (float)2.0*Math::acos(w);
            float fInvLength = Math::invSqrt(fSqrLength);
            rkAxis.x = x*fInvLength;
            rkAxis.y = y*fInvLength;
            rkAxis.z = z*fInvLength;
        }
        else
        {
            // angle is 0 (mod 2*pi), so any axis will do
            rfAngle = 0.0;
            rkAxis.x = 1.0;
            rkAxis.y = 0.0;
            rkAxis.z = 0.0;
        }
    }
    
    void Quaternion::fromAxes( const Vector3* akAxis )
    {
        Matrix3 kRot;

        for (size_t iCol = 0; iCol < 3; iCol++)
        {
            kRot[0][iCol] = akAxis[iCol].x;
            kRot[1][iCol] = akAxis[iCol].y;
            kRot[2][iCol] = akAxis[iCol].z;
        }

        fromRotationMatrix(kRot);
    }
    
    void Quaternion::fromAxes( const Vector3& xaxis, const Vector3& yaxis, const Vector3& zaxis )
    {
        Matrix3 kRot;

        kRot[0][0] = xaxis.x;
        kRot[1][0] = xaxis.y;
        kRot[2][0] = xaxis.z;

        kRot[0][1] = yaxis.x;
        kRot[1][1] = yaxis.y;
        kRot[2][1] = yaxis.z;

        kRot[0][2] = zaxis.x;
        kRot[1][2] = zaxis.y;
        kRot[2][2] = zaxis.z;

        fromRotationMatrix(kRot);
    }
    
    void Quaternion::toAxes( Vector3* akAxis ) const
    {
        Matrix3 kRot;
        toRotationMatrix(kRot);

        for (size_t iCol = 0; iCol < 3; iCol++)
        {
            akAxis[iCol].x = kRot[0][iCol];
            akAxis[iCol].y = kRot[1][iCol];
            akAxis[iCol].z = kRot[2][iCol];
        }
    }
    
    Vector3 Quaternion::xAxis() const
    {
        //float fTx  = 2.0*x;
        float fTy  = 2.0f*y;
        float fTz  = 2.0f*z;
        float fTwy = fTy*w;
        float fTwz = fTz*w;
        float fTxy = fTy*x;
        float fTxz = fTz*x;
        float fTyy = fTy*y;
        float fTzz = fTz*z;

        return Vector3(1.0f-(fTyy+fTzz), fTxy+fTwz, fTxz-fTwy);
    }
    
    Vector3 Quaternion::yAxis() const
    {
        float fTx  = 2.0f*x;
        float fTy  = 2.0f*y;
        float fTz  = 2.0f*z;
        float fTwx = fTx*w;
        float fTwz = fTz*w;
        float fTxx = fTx*x;
        float fTxy = fTy*x;
        float fTyz = fTz*y;
        float fTzz = fTz*z;

        return Vector3(fTxy-fTwz, 1.0f-(fTxx+fTzz), fTyz+fTwx);
    }
    
    Vector3 Quaternion::zAxis() const
    {
        float fTx  = 2.0f*x;
        float fTy  = 2.0f*y;
        float fTz  = 2.0f*z;
        float fTwx = fTx*w;
        float fTwy = fTy*w;
        float fTxx = fTx*x;
        float fTxz = fTz*x;
        float fTyy = fTy*y;
        float fTyz = fTz*y;

        return Vector3(fTxz+fTwy, fTyz-fTwx, 1.0f-(fTxx+fTyy));
    }
    
    void Quaternion::toAxes( Vector3& xaxis, Vector3& yaxis, Vector3& zaxis ) const
    {
        Matrix3 kRot;
        toRotationMatrix(kRot);

        xaxis.x = kRot[0][0];
        xaxis.y = kRot[1][0];
        xaxis.z = kRot[2][0];

        yaxis.x = kRot[0][1];
        yaxis.y = kRot[1][1];
        yaxis.z = kRot[2][1];

        zaxis.x = kRot[0][2];
        zaxis.y = kRot[1][2];
        zaxis.z = kRot[2][2];
    }

    Quaternion Quaternion::operator+(const Quaternion& rkQ) const
    {
        return Quaternion(w+rkQ.w,x+rkQ.x,y+rkQ.y,z+rkQ.z);
    }
    
    Quaternion Quaternion::operator-(const Quaternion& rkQ) const
    {
        return Quaternion(w-rkQ.w,x-rkQ.x,y-rkQ.y,z-rkQ.z);
    }
    
    Quaternion Quaternion::operator*(const Quaternion& rkQ) const
    {
        // NOTE:  Multiplication is not generally commutative, so in most
        // cases p*q != q*p.

        return Quaternion
        (
            w * rkQ.w - x * rkQ.x - y * rkQ.y - z * rkQ.z,
            w * rkQ.x + x * rkQ.w + y * rkQ.z - z * rkQ.y,
            w * rkQ.y + y * rkQ.w + z * rkQ.x - x * rkQ.z,
            w * rkQ.z + z * rkQ.w + x * rkQ.y - y * rkQ.x
        );
    }
    
    Quaternion Quaternion::operator*(float fScalar) const
    {
        return Quaternion(fScalar*w,fScalar*x,fScalar*y,fScalar*z);
    }
    
    Quaternion operator*(float fScalar, const Quaternion& rkQ)
    {
        return Quaternion(fScalar*rkQ.w,fScalar*rkQ.x,fScalar*rkQ.y,
            fScalar*rkQ.z);
    }
    
    Quaternion Quaternion::operator-() const
    {
        return Quaternion(-w,-x,-y,-z);
    }
    
    float Quaternion::dot(const Quaternion& rkQ) const
    {
        return w*rkQ.w+x*rkQ.x+y*rkQ.y+z*rkQ.z;
    }
    
    float Quaternion::norm() const
    {
        return w*w+x*x+y*y+z*z;
    }
    
    Quaternion Quaternion::inverse() const
    {
        float fNorm = w*w+x*x+y*y+z*z;
        if ( fNorm > 0.0 )
        {
            float fInvNorm = 1.0f/fNorm;
            return Quaternion(w*fInvNorm,-x*fInvNorm,-y*fInvNorm,-z*fInvNorm);
        }
        else
        {
            // return an invalid result to flag the error
            return ZERO;
        }
    }
    
    Quaternion Quaternion::unitInverse() const
    {
        // assert:  'this' is unit length
        return Quaternion(w,-x,-y,-z);
    }
    
    Quaternion Quaternion::exp() const
    {
        // If q = A*(x*i+y*j+z*k) where (x,y,z) is unit length, then
        // exp(q) = cos(A)+sin(A)*(x*i+y*j+z*k).  If sin(A) is near zero,
        // use exp(q) = cos(A)+A*(x*i+y*j+z*k) since A/sin(A) has limit 1.

        float fAngle( Math::sqrt(x*x+y*y+z*z) );
        float fSin = Math::sin(fAngle);

        Quaternion kResult;
        kResult.w = Math::cos(fAngle);

        if ( Math::fabs(fSin) >= s_epsilon )
        {
            float fCoeff = fSin/(fAngle);
            kResult.x = fCoeff*x;
            kResult.y = fCoeff*y;
            kResult.z = fCoeff*z;
        }
        else
        {
            kResult.x = x;
            kResult.y = y;
            kResult.z = z;
        }

        return kResult;
    }
    
    Quaternion Quaternion::log() const
    {
        // If q = cos(A)+sin(A)*(x*i+y*j+z*k) where (x,y,z) is unit length, then
        // log(q) = A*(x*i+y*j+z*k).  If sin(A) is near zero, use log(q) =
        // sin(A)*(x*i+y*j+z*k) since sin(A)/A has limit 1.

        Quaternion kResult;
        kResult.w = 0.0;

        if ( Math::fabs(w) < 1.0 )
        {
            float fAngle( Math::acos(w) );
            float fSin = Math::sin(fAngle);
            if ( Math::fabs(fSin) >= s_epsilon )
            {
                float fCoeff = fAngle/fSin;
                kResult.x = fCoeff*x;
                kResult.y = fCoeff*y;
                kResult.z = fCoeff*z;
                return kResult;
            }
        }

        kResult.x = x;
        kResult.y = y;
        kResult.z = z;

        return kResult;
    }
    
    Vector3 Quaternion::operator*( const Vector3& v ) const
    {
		// nVidia SDK implementation
		Vector3 uv, uuv;
		Vector3 qvec(x, y, z);
		uv = qvec.crossProduct(v);
		uuv = qvec.crossProduct(uv);
		uv *= (2.0f * w);
		uuv *= 2.0f;

		return v + uv + uuv;
    }
    
	bool Quaternion::equals(const Quaternion& rhs, float tolerance) const
	{
        float fCos = dot(rhs);
        float angle = Math::acos(fCos);

		return (Math::fabs(angle) <= tolerance)
            || Math::realEqual(angle, Math::PI, tolerance);
	}
    
    Quaternion Quaternion::slerp(float fT, const Quaternion& rkP,
        const Quaternion& rkQ, bool shortestPath)
    {
        float fCos = rkP.dot(rkQ);
        Quaternion rkT;

        // Do we need to invert rotation?
        if (fCos < 0.0f && shortestPath)
        {
            fCos = -fCos;
            rkT = -rkQ;
        }
        else
        {
            rkT = rkQ;
        }

        if (Math::fabs(fCos) < 1 - s_epsilon)
        {
            // Standard case (slerp)
            float fSin = Math::sqrt(1 - Math::sqr(fCos));
            float fAngle = Math::atan2(fSin, fCos);
            float fInvSin = 1.0f / fSin;
            float fCoeff0 = Math::sin((1.0f - fT) * fAngle) * fInvSin;
            float fCoeff1 = Math::sin(fT * fAngle) * fInvSin;
            return fCoeff0 * rkP + fCoeff1 * rkT;
        }
        else
        {
            // There are two situations:
            // 1. "rkP" and "rkQ" are very close (fCos ~= +1), so we can do a linear
            //    interpolation safely.
            // 2. "rkP" and "rkQ" are almost inverse of each other (fCos ~= -1), there
            //    are an infinite number of possibilities interpolation. but we haven't
            //    have method to fix this case, so just use linear interpolation here.
            Quaternion t = (1.0f - fT) * rkP + fT * rkT;
            // taking the complement requires renormalisation
            t.normalise();
            return t;
        }
    }
    
    Quaternion Quaternion::slerpExtraSpins(float fT,
        const Quaternion& rkP, const Quaternion& rkQ, int iExtraSpins)
    {
        float fCos = rkP.dot(rkQ);
        float fAngle ( Math::acos(fCos) );

        if ( Math::fabs(fAngle) < s_epsilon )
            return rkP;

        float fSin = Math::sin(fAngle);
        float fPhase ( Math::PI*iExtraSpins*fT );
        float fInvSin = 1.0f/fSin;
        float fCoeff0 = Math::sin((1.0f-fT)*fAngle - fPhase)*fInvSin;
        float fCoeff1 = Math::sin(fT*fAngle + fPhase)*fInvSin;
        return fCoeff0*rkP + fCoeff1*rkQ;
    }
    
    void Quaternion::intermediate(const Quaternion& rkQ0,
        const Quaternion& rkQ1, const Quaternion& rkQ2,
        Quaternion& rkA, Quaternion& rkB)
    {
        // assert:  q0, q1, q2 are unit quaternions

        Quaternion kQ0inv = rkQ0.unitInverse();
        Quaternion kQ1inv = rkQ1.unitInverse();
        Quaternion rkP0 = kQ0inv*rkQ1;
        Quaternion rkP1 = kQ1inv*rkQ2;
        Quaternion kArg = 0.25*(rkP0.log()-rkP1.log());
        Quaternion kMinusArg = -kArg;

        rkA = rkQ1*kArg.exp();
        rkB = rkQ1*kMinusArg.exp();
    }
    
    Quaternion Quaternion::squad(float fT,
        const Quaternion& rkP, const Quaternion& rkA,
        const Quaternion& rkB, const Quaternion& rkQ, bool shortestPath)
    {
        float fSlerpT = 2.0f*fT*(1.0f-fT);
        Quaternion kSlerpP = slerp(fT, rkP, rkQ, shortestPath);
        Quaternion kSlerpQ = slerp(fT, rkA, rkB);
        return slerp(fSlerpT, kSlerpP ,kSlerpQ);
    }
    
    float Quaternion::normalise()
    {
        float len = norm();
        float factor = 1.0f / Math::sqrt(len);
        *this = *this * factor;
        return len;
    }
    
	float Quaternion::getRoll(bool reprojectAxis) const
	{
		if (reprojectAxis)
		{
			// roll = atan2(localx.y, localx.x)
			// pick parts of xAxis() implementation that we need
//			float fTx  = 2.0*x;
			float fTy  = 2.0f*y;
			float fTz  = 2.0f*z;
			float fTwz = fTz*w;
			float fTxy = fTy*x;
			float fTyy = fTy*y;
			float fTzz = fTz*z;

			// Vector3(1.0-(fTyy+fTzz), fTxy+fTwz, fTxz-fTwy);

			return (Math::atan2(fTxy+fTwz, 1.0f-(fTyy+fTzz)));

		}
		else
		{
			return (Math::atan2(2*(x*y + w*z), w*w + x*x - y*y - z*z));
		}
	}
    
	float Quaternion::getPitch(bool reprojectAxis) const
	{
		if (reprojectAxis)
		{
			// pitch = atan2(localy.z, localy.y)
			// pick parts of yAxis() implementation that we need
			float fTx  = 2.0f*x;
//			float fTy  = 2.0f*y;
			float fTz  = 2.0f*z;
			float fTwx = fTx*w;
			float fTxx = fTx*x;
			float fTyz = fTz*y;
			float fTzz = fTz*z;

			// Vector3(fTxy-fTwz, 1.0-(fTxx+fTzz), fTyz+fTwx);
			return (Math::atan2(fTyz+fTwx, 1.0f-(fTxx+fTzz)));
		}
		else
		{
			// internal version
			return (Math::atan2(2*(y*z + w*x), w*w - x*x - y*y + z*z));
		}
	}
    
	float Quaternion::getYaw(bool reprojectAxis) const
	{
		if (reprojectAxis)
		{
			// yaw = atan2(localz.x, localz.z)
			// pick parts of zAxis() implementation that we need
			float fTx  = 2.0f*x;
			float fTy  = 2.0f*y;
			float fTz  = 2.0f*z;
			float fTwy = fTy*w;
			float fTxx = fTx*x;
			float fTxz = fTz*x;
			float fTyy = fTy*y;

			// Vector3(fTxz+fTwy, fTyz-fTwx, 1.0-(fTxx+fTyy));

			return (Math::atan2(fTxz+fTwy, 1.0f-(fTxx+fTyy)));

		}
		else
		{
			// internal version
			return (Math::asin(-2*(x*z - w*y)));
		}
	}
    
    Quaternion Quaternion::nlerp(float fT, const Quaternion& rkP,
        const Quaternion& rkQ, bool shortestPath)
    {
		Quaternion result;
        float fCos = rkP.dot(rkQ);
		if (fCos < 0.0f && shortestPath)
		{
			result = rkP + fT * ((-rkQ) - rkP);
		}
		else
		{
			result = rkP + fT * (rkQ - rkP);
		}
        result.normalise();
        return result;
    }
}

