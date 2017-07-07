#include "KhaosPreHeaders.h"
#include "KhaosPlane.h"
#include "KhaosMatrix3.h"
#include "KhaosAxisAlignedBox.h" 

namespace Khaos 
{
	Plane::Plane()
	{
		normal = Vector3::ZERO;
		d = 0.0;
	}
	
	Plane::Plane(const Plane& rhs)
	{
		normal = rhs.normal;
		d = rhs.d;
	}
	
	Plane::Plane(const Vector3& rkNormal, float fConstant)
	{
		normal = rkNormal;
		d = -fConstant;
	}
	//---------------------------------------------------------------------
	Plane::Plane(float a, float b, float c, float _d)
		: normal(a, b, c), d(_d)
	{
	}
	
	Plane::Plane(const Vector3& rkNormal, const Vector3& rkPoint)
	{
		redefine(rkNormal, rkPoint);
	}
	
	Plane::Plane(const Vector3& rkPoint0, const Vector3& rkPoint1,
		const Vector3& rkPoint2)
	{
		redefine(rkPoint0, rkPoint1, rkPoint2);
	}
	
	float Plane::getDistance(const Vector3& rkPoint) const
	{
		return normal.dotProduct(rkPoint) + d;
	}
	
	Plane::Side Plane::getSide(const Vector3& rkPoint) const
	{
		float fDistance = getDistance(rkPoint);

		if ( fDistance < 0.0 )
			return Plane::NEGATIVE_SIDE;

		if ( fDistance > 0.0 )
			return Plane::POSITIVE_SIDE;

		return Plane::NO_SIDE;
	}

    Plane::Side Plane::getSide( const Vector3& v0, const Vector3& v1, const Vector3& v2 ) const
    {
        int ret = 0;

        // test v0
        Plane::Side s = getSide(v0);
        if ( s == NO_SIDE )
            return BOTH_SIDE;

        ret |= s;

        // test v1
        s = getSide(v1);
        if ( s == NO_SIDE )
            return BOTH_SIDE;

        ret |= s;
        if ( ret == BOTH_SIDE )
            return BOTH_SIDE;

        // test v2
        s = getSide(v2);
        if ( s == NO_SIDE )
            return BOTH_SIDE;

        ret |= s;
        return (Side)ret;
    }

	
	Plane::Side Plane::getSide(const AxisAlignedBox& box) const
	{
		if (box.isNull()) 
			return NO_SIDE;
		if (box.isInfinite())
			return BOTH_SIDE;

        return getSide(box.getCenter(), box.getHalfSize());
	}
    
    Plane::Side Plane::getSide(const Vector3& centre, const Vector3& halfSize) const
    {
        // Calculate the distance between box centre and the plane
        float dist = getDistance(centre);

        // Calculate the maximise allows absolute distance for
        // the distance between box centre and plane
        float maxAbsDist = normal.absDotProduct(halfSize);

        if (dist < -maxAbsDist)
            return Plane::NEGATIVE_SIDE;

        if (dist > +maxAbsDist)
            return Plane::POSITIVE_SIDE;

        return Plane::BOTH_SIDE;
    }
	
	void Plane::redefine(const Vector3& rkPoint0, const Vector3& rkPoint1,
		const Vector3& rkPoint2)
	{
		Vector3 kEdge1 = rkPoint1 - rkPoint0;
		Vector3 kEdge2 = rkPoint2 - rkPoint0;
		normal = kEdge1.crossProduct(kEdge2);
		normal.normalise();
		d = -normal.dotProduct(rkPoint0);
	}
	
	void Plane::redefine(const Vector3& rkNormal, const Vector3& rkPoint)
	{
		normal = rkNormal;
		d = -rkNormal.dotProduct(rkPoint);
	}
	
	Vector3 Plane::projectVector(const Vector3& p) const
	{
		// We know plane normal is unit length, so use simple method
		Matrix3 xform;
		xform[0][0] = 1.0f - normal.x * normal.x;
		xform[0][1] = -normal.x * normal.y;
		xform[0][2] = -normal.x * normal.z;
		xform[1][0] = -normal.y * normal.x;
		xform[1][1] = 1.0f - normal.y * normal.y;
		xform[1][2] = -normal.y * normal.z;
		xform[2][0] = -normal.z * normal.x;
		xform[2][1] = -normal.z * normal.y;
		xform[2][2] = 1.0f - normal.z * normal.z;
		return xform * p;
	}
	
    float Plane::normalise()
    {
        float fLength = normal.length();

        // Will also work for zero-sized vectors, but will change nothing
		// We're not using epsilons because we don't need to.
        if ( fLength > float(0.0f) )
        {
            float fInvLength = 1.0f / fLength;
            normal *= fInvLength;
            d *= fInvLength;
        }

        return fLength;
    }
}
