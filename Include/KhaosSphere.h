#pragma once
#include "KhaosVector3.h"

namespace Khaos
{
    class Sphere
    {
    protected:
        float m_radius;
        Vector3 m_center;

    public:
        Sphere() : m_radius(1.0), m_center(Vector3::ZERO) {}
        Sphere(const Vector3& center, float radius)
            : m_radius(radius), m_center(center) {}

    public:
        float getRadius() const { return m_radius; }
        void setRadius(float radius) { m_radius = radius; }

        const Vector3& getCenter(void) const { return m_center; }
        void setCenter(const Vector3& center) { m_center = center; }

    public:
		bool intersects(const Sphere& s) const
		{
            return (s.m_center - m_center).squaredLength() <=
                Math::sqr(s.m_radius + m_radius);
		}
		
		bool intersects(const AxisAlignedBox& box) const
		{
			return Math::intersects(*this, box);
		}
		
		bool intersects(const Plane& plane) const
		{
			return Math::intersects(*this, plane);
		}
		
		bool intersects(const Vector3& v) const
		{
            return ((v - m_center).squaredLength() <= Math::sqr(m_radius));
		}
		
    public:
		void merge(const Sphere& oth)
		{
			Vector3 diff =  oth.getCenter() - m_center;
			float lengthSq = diff.squaredLength();
			float radiusDiff = oth.getRadius() - m_radius;
			
			// Early-out
			if (Math::sqr(radiusDiff) >= lengthSq) 
			{
				// One fully contains the other
				if (radiusDiff <= 0.0f) 
					return; // no change
				else 
				{
					m_center = oth.getCenter();
					m_radius = oth.getRadius();
					return;
				}
			}
			
			float length = Math::sqrt(lengthSq);
			
			Vector3 newCenter;
			float newRadius;
			if ((length + oth.getRadius()) > m_radius) 
			{
				float t = (length + radiusDiff) / (2.0f * length);
				newCenter = m_center + diff * t;
			} 
			// otherwise, we keep our existing center
			
			newRadius = 0.5f * (length + m_radius + oth.getRadius());
			
			m_center = newCenter;
			m_radius = newRadius;
		}
    };
}

