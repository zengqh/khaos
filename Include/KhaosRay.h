#pragma once
#include "KhaosVector3.h"
#include "KhaosPlaneBoundedVolume.h"

namespace Khaos
{
    class Ray
    {
    protected:
        Vector3 m_origin;
        float   m_origin_w; // not used, only for vec4 type

        Vector3 m_direction;
        float   m_direction_w; // not used, only for vec4 type

        Vector4 m_oneOverDir;

    public:
        Ray() : m_origin(Vector3::ZERO), m_direction(Vector3::UNIT_Z), m_origin_w(0), m_direction_w(0) {}

        Ray( const Vector3& origin, const Vector3& direction ) :
            m_origin(origin), m_direction(direction), m_origin_w(1), m_direction_w(0)
        { m_oneOverDir.asVec3() = Vector3::UNIT_SCALE / m_direction; }

    public:
        /** Sets the origin of the ray. */
        void setOrigin(const Vector3& origin) { m_origin = origin; } 

        /** Gets the origin of the ray. */
        const Vector3& getOrigin(void) const { return m_origin; } 
        const Vector4& getOriginV4() const { return *(Vector4*)m_origin.ptr(); }

        /** Sets the direction of the ray. */
        void setDirection(const Vector3& dir) 
        { m_direction = dir; m_oneOverDir.asVec3() = Vector3::UNIT_SCALE / m_direction; } 
        
        /** Gets the direction of the ray. */
        const Vector3& getDirection(void) const { return m_direction; } 
        const Vector4& getDirectionV4() const { return *(Vector4*)m_direction.ptr(); }

        const Vector4& getOneOverDir() const { return m_oneOverDir; }

		/** Gets the position of a point t units along the ray. */
		Vector3 getPoint(float t) const { 
			return Vector3(m_origin + (m_direction * t));
		}
		
		/** Gets the position of a point t units along the ray. */
		Vector3 operator*(float t) const { 
			return getPoint(t);
		}

    public:
		/** Tests whether this ray intersects the given plane. 
		    return A pair structure where the first element indicates whether
			an intersection occurs, and if true, the second element will
			indicate the distance along the ray at which it intersects. 
			This can be converted to a point in space by calling getPoint().
		*/
		std::pair<bool, float> intersects(const Plane& p) const
		{
			return Math::intersects(*this, p);
		}

        /** Tests whether this ray intersects the given plane bounded volume. 
        return A pair structure where the first element indicates whether
        an intersection occurs, and if true, the second element will
        indicate the distance along the ray at which it intersects. 
        This can be converted to a point in space by calling getPoint().
        */
        std::pair<bool, float> intersects(const PlaneBoundedVolume& p) const
        {
            return Math::intersects(*this, p.planes, p.outside == Plane::POSITIVE_SIDE);
        }

		/** Tests whether this ray intersects the given sphere. 
		    return A pair structure where the first element indicates whether
			an intersection occurs, and if true, the second element will
			indicate the distance along the ray at which it intersects. 
			This can be converted to a point in space by calling getPoint().
		*/
		std::pair<bool, float> intersects(const Sphere& s) const
		{
			return Math::intersects(*this, s);
		}

		/** Tests whether this ray intersects the given box. 
		    return A pair structure where the first element indicates whether
			an intersection occurs, and if true, the second element will
			indicate the distance along the ray at which it intersects. 
			This can be converted to a point in space by calling getPoint().
		*/
		std::pair<bool, float> intersects(const AxisAlignedBox& box) const
		{
			return Math::intersects(*this, box);
		}

        bool intersects( const Vector3& v0, const Vector3& v1, const Vector3& v2,
            float* pt = 0, Vector3* gravity = 0 ) const
        {
            return Math::rayIntersectsTriangle( *this, v0, v1, v2, pt, gravity );
        }
    };

    class LimitRay : public Ray
    {
    protected:
        float m_maxLen;

    public:
        LimitRay() : m_maxLen(Math::POS_INFINITY) {}

        LimitRay( const Vector3& origin, const Vector3& direction, float maxLen /*= Math::POS_INFINITY*/ ) : 
            Ray(origin, direction), m_maxLen(maxLen) {}

        LimitRay( const Ray& ray, float maxLen /*= Math::POS_INFINITY*/ ) :
            Ray(ray), m_maxLen(maxLen) {}

        LimitRay( const Vector3& origin, const Vector3& dest ) :
            Ray(origin, (dest - origin).normalisedCopy()), m_maxLen(origin.distance(dest)) {}

    public:
        void setMaxLen( float maxLen ) { m_maxLen = maxLen; }

        float getMaxLen() const { return m_maxLen; }

        Vector3 getEndPoint() const
        {
            return this->getPoint( m_maxLen );
        }

    public:
        template<class T>
        std::pair<bool, float> intersects(const T& p) const
        {
            std::pair<bool, float> ret = Ray::intersects(p);
            if ( ret.second > m_maxLen )
                ret.first = false;
            return ret;
        }

        bool intersects( const Vector3& v0, const Vector3& v1, const Vector3& v2,
            float* pt = 0, Vector3* gravity = 0 ) const
        {
            float t = 0;
            bool ret = Ray::intersects( v0, v1, v2, &t, gravity );
            
            if ( t > m_maxLen )
                ret = false;

            if ( pt )
                *pt = t;

            return ret;
        }
    };
}

