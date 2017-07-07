#pragma once
#include "KhaosVector3.h"
#include "KhaosMatrix4.h"

namespace Khaos 
{
	class AxisAlignedBox
	{
	public:
		enum Extent
		{
			EXTENT_NULL,
			EXTENT_FINITE,
			EXTENT_INFINITE
		};

	protected:
		Vector3 m_minimum;
		Vector3 m_maximum;
		Extent m_extent;
		mutable Vector3* m_corners;

	public:
		/*
           1-----2
          /|    /|
		 / |   / |
		5-----4  |
		|  0--|--3
		| /   | /
		|/    |/
		6-----7
		*/
		typedef enum 
        {
			FAR_LEFT_BOTTOM = 0,
			FAR_LEFT_TOP = 1,
			FAR_RIGHT_TOP = 2,
			FAR_RIGHT_BOTTOM = 3,
			NEAR_RIGHT_BOTTOM = 7,
			NEAR_LEFT_BOTTOM = 6,
			NEAR_LEFT_TOP = 5,
			NEAR_RIGHT_TOP = 4
		} CornerEnum;

    public:
		AxisAlignedBox() : m_minimum(Vector3::ZERO), m_maximum(Vector3::UNIT_SCALE), m_corners(0)
		{
			// Default to a null box 
			setMinimum( -0.5, -0.5, -0.5 );
			setMaximum( 0.5, 0.5, 0.5 );
			m_extent = EXTENT_NULL;
		}

		AxisAlignedBox(Extent e) : m_minimum(Vector3::ZERO), m_maximum(Vector3::UNIT_SCALE), m_corners(0)
		{
			setMinimum( -0.5, -0.5, -0.5 );
			setMaximum( 0.5, 0.5, 0.5 );
			m_extent = e;
		}

		AxisAlignedBox(const AxisAlignedBox & rkBox) : m_minimum(Vector3::ZERO), m_maximum(Vector3::UNIT_SCALE), m_corners(0)

		{
			if (rkBox.isNull())
				setNull();
			else if (rkBox.isInfinite())
				setInfinite();
			else
				setExtents( rkBox.m_minimum, rkBox.m_maximum );
		}

		AxisAlignedBox( const Vector3& min, const Vector3& max ) : m_minimum(Vector3::ZERO), m_maximum(Vector3::UNIT_SCALE), m_corners(0)
		{
			setExtents( min, max );
		}

		AxisAlignedBox(
			float mx, float my, float mz,
			float Mx, float My, float Mz ) : m_minimum(Vector3::ZERO), m_maximum(Vector3::UNIT_SCALE), m_corners(0)
		{
			setExtents( mx, my, mz, Mx, My, Mz );
		}

    public:
		AxisAlignedBox& operator=(const AxisAlignedBox& rhs)
		{
			// Specifically override to avoid copying m_corners
			if (rhs.isNull())
				setNull();
			else if (rhs.isInfinite())
				setInfinite();
			else
				setExtents(rhs.m_minimum, rhs.m_maximum);

			return *this;
		}

		~AxisAlignedBox()
		{
			if (m_corners)
				KHAOS_FREE(m_corners);
		}

    public:
		const Vector3& getMinimum() const
		{ 
			return m_minimum; 
		}

		Vector3& getMinimum()
		{ 
			return m_minimum; 
		}

		const Vector3& getMaximum() const
		{ 
			return m_maximum;
		}

		Vector3& getMaximum()
		{ 
			return m_maximum;
		}

		void setMinimum( const Vector3& vec )
		{
			m_extent = EXTENT_FINITE;
			m_minimum = vec;
		}

		void setMinimum( float x, float y, float z )
		{
			m_extent = EXTENT_FINITE;
			m_minimum.x = x;
			m_minimum.y = y;
			m_minimum.z = z;
		}

		void setMinimumX(float x)
		{
			m_minimum.x = x;
		}

		void setMinimumY(float y)
		{
			m_minimum.y = y;
		}

		void setMinimumZ(float z)
		{
			m_minimum.z = z;
		}

		void setMaximum( const Vector3& vec )
		{
			m_extent = EXTENT_FINITE;
			m_maximum = vec;
		}

		void setMaximum( float x, float y, float z )
		{
			m_extent = EXTENT_FINITE;
			m_maximum.x = x;
			m_maximum.y = y;
			m_maximum.z = z;
		}

		void setMaximumX( float x )
		{
			m_maximum.x = x;
		}

		void setMaximumY( float y )
		{
			m_maximum.y = y;
		}

		void setMaximumZ( float z )
		{
			m_maximum.z = z;
		}

		void setExtents( const Vector3& min, const Vector3& max )
		{
            khaosAssert( (min.x <= max.x && min.y <= max.y && min.z <= max.z) );

			m_extent = EXTENT_FINITE;
			m_minimum = min;
			m_maximum = max;
		}

		void setExtents(
			float mx, float my, float mz,
			float Mx, float My, float Mz )
		{
            khaosAssert( (mx <= Mx && my <= My && mz <= Mz) );

			m_extent = EXTENT_FINITE;

			m_minimum.x = mx;
			m_minimum.y = my;
			m_minimum.z = mz;

			m_maximum.x = Mx;
			m_maximum.y = My;
			m_maximum.z = Mz;

		}

		/** Returns a pointer to an array of 8 corner points, useful for
		collision vs. non-aligned objects.
		   1-----2
		  /|    /|
		 / |   / |
		5-----4  |
		|  0--|--3
		| /   | /
		|/    |/
		6-----7
		this implementation uses a static member, make sure to use your own copy !
		*/
		const Vector3* getAllCorners() const
		{
			khaosAssert( (m_extent == EXTENT_FINITE) );

			// The order of these items is, using right-handed co-ordinates:
			// Minimum Z face, starting with Min(all), then anticlockwise
			//   around face (looking onto the face)
			// Maximum Z face, starting with Max(all), then anticlockwise
			//   around face (looking onto the face)
			// Only for optimization/compatibility.
			if (!m_corners)
				m_corners = KHAOS_MALLOC_ARRAY_T(Vector3, 8);

			m_corners[0] = m_minimum;
			m_corners[1].x = m_minimum.x; m_corners[1].y = m_maximum.y; m_corners[1].z = m_minimum.z;
			m_corners[2].x = m_maximum.x; m_corners[2].y = m_maximum.y; m_corners[2].z = m_minimum.z;
			m_corners[3].x = m_maximum.x; m_corners[3].y = m_minimum.y; m_corners[3].z = m_minimum.z;            

			m_corners[4] = m_maximum;
			m_corners[5].x = m_minimum.x; m_corners[5].y = m_maximum.y; m_corners[5].z = m_maximum.z;
			m_corners[6].x = m_minimum.x; m_corners[6].y = m_minimum.y; m_corners[6].z = m_maximum.z;
			m_corners[7].x = m_maximum.x; m_corners[7].y = m_minimum.y; m_corners[7].z = m_maximum.z;

			return m_corners;
		}

		Vector3 getCorner(CornerEnum cornerToGet) const
		{
			switch(cornerToGet)
			{
			case FAR_LEFT_BOTTOM:
				return m_minimum;
			case FAR_LEFT_TOP:
				return Vector3(m_minimum.x, m_maximum.y, m_minimum.z);
			case FAR_RIGHT_TOP:
				return Vector3(m_maximum.x, m_maximum.y, m_minimum.z);
			case FAR_RIGHT_BOTTOM:
				return Vector3(m_maximum.x, m_minimum.y, m_minimum.z);
			case NEAR_RIGHT_BOTTOM:
				return Vector3(m_maximum.x, m_minimum.y, m_maximum.z);
			case NEAR_LEFT_BOTTOM:
				return Vector3(m_minimum.x, m_minimum.y, m_maximum.z);
			case NEAR_LEFT_TOP:
				return Vector3(m_minimum.x, m_maximum.y, m_maximum.z);
			case NEAR_RIGHT_TOP:
				return m_maximum;
			default:
				return Vector3();
			}
		}

		void merge( const AxisAlignedBox& rhs )
		{
			// Do nothing if rhs null, or this is infinite
			if ((rhs.m_extent == EXTENT_NULL) || (m_extent == EXTENT_INFINITE))
			{
				return;
			}
			// Otherwise if rhs is infinite, make this infinite, too
			else if (rhs.m_extent == EXTENT_INFINITE)
			{
				m_extent = EXTENT_INFINITE;
			}
			// Otherwise if current null, just take rhs
			else if (m_extent == EXTENT_NULL)
			{
				setExtents(rhs.m_minimum, rhs.m_maximum);
			}
			// Otherwise merge
			else
			{
				Vector3 min = m_minimum;
				Vector3 max = m_maximum;
				max.makeCeil(rhs.m_maximum);
				min.makeFloor(rhs.m_minimum);

				setExtents(min, max);
			}

		}

		void merge( const Vector3& point )
		{
			switch (m_extent)
			{
			case EXTENT_NULL: // if null, use this point
				setExtents(point, point);
				return;

			case EXTENT_FINITE:
				m_maximum.makeCeil(point);
				m_minimum.makeFloor(point);
				return;

			case EXTENT_INFINITE: // if infinite, makes no difference
				return;
			}

			khaosAssert( false );
		}

        void merge( const Vector3* pts, int count, int stride )
        {
            for ( int i = 0; i < count; ++i )
            {
                merge( *pts );
                pts = (const Vector3*)((const char*)pts + stride);
            }
        }

		void transform( const Matrix4& matrix )
		{
			// Do nothing if current null or infinite
			if( m_extent != EXTENT_FINITE )
				return;

			Vector3 oldMin, oldMax, currentCorner;

			// Getting the old values so that we can use the existing merge method.
			oldMin = m_minimum;
			oldMax = m_maximum;

			// reset
			setNull();

			// We sequentially compute the corners in the following order :
			// 0, 6, 5, 1, 2, 4 ,7 , 3
			// This sequence allows us to only change one member at a time to get at all corners.

			// For each one, we transform it using the matrix
			// Which gives the resulting point and merge the resulting point.

			// First corner 
			// min min min
			currentCorner = oldMin;
			merge( matrix * currentCorner );

			// min,min,max
			currentCorner.z = oldMax.z;
			merge( matrix * currentCorner );

			// min max max
			currentCorner.y = oldMax.y;
			merge( matrix * currentCorner );

			// min max min
			currentCorner.z = oldMin.z;
			merge( matrix * currentCorner );

			// max max min
			currentCorner.x = oldMax.x;
			merge( matrix * currentCorner );

			// max max max
			currentCorner.z = oldMax.z;
			merge( matrix * currentCorner );

			// max min max
			currentCorner.y = oldMin.y;
			merge( matrix * currentCorner );

			// max min min
			currentCorner.z = oldMin.z;
			merge( matrix * currentCorner ); 
		}

		void transformAffine(const Matrix4& m)
		{
			khaosAssert(m.isAffine());

			// Do nothing if current null or infinite
			if ( m_extent != EXTENT_FINITE )
				return;

			Vector3 centre = getCenter();
			Vector3 halfSize = getHalfSize();

			Vector3 newCentre = m.transformAffine(centre);
			Vector3 newHalfSize(
				Math::fabs(m[0][0]) * halfSize.x + Math::fabs(m[0][1]) * halfSize.y + Math::fabs(m[0][2]) * halfSize.z, 
				Math::fabs(m[1][0]) * halfSize.x + Math::fabs(m[1][1]) * halfSize.y + Math::fabs(m[1][2]) * halfSize.z,
				Math::fabs(m[2][0]) * halfSize.x + Math::fabs(m[2][1]) * halfSize.y + Math::fabs(m[2][2]) * halfSize.z);

			setExtents(newCentre - newHalfSize, newCentre + newHalfSize);
		}

		void setNull()
		{
			m_extent = EXTENT_NULL;
		}

		bool isNull() const
		{
			return (m_extent == EXTENT_NULL);
		}

		bool isFinite() const
		{
			return (m_extent == EXTENT_FINITE);
		}

		void setInfinite()
		{
			m_extent = EXTENT_INFINITE;
		}

		bool isInfinite() const
		{
			return (m_extent == EXTENT_INFINITE);
		}

		/** Returns whether or not this box intersects another. */
		bool intersects(const AxisAlignedBox& b2) const
		{
			// Early-fail for nulls
			if (this->isNull() || b2.isNull())
				return false;

			// Early-success for infinites
			if (this->isInfinite() || b2.isInfinite())
				return true;

			// Use up to 6 separating planes
			if (m_maximum.x < b2.m_minimum.x)
				return false;
			if (m_maximum.y < b2.m_minimum.y)
				return false;
			if (m_maximum.z < b2.m_minimum.z)
				return false;

			if (m_minimum.x > b2.m_maximum.x)
				return false;
			if (m_minimum.y > b2.m_maximum.y)
				return false;
			if (m_minimum.z > b2.m_maximum.z)
				return false;

			// otherwise, must be intersecting
			return true;

		}

		/// Calculate the area of intersection of this box and another
		AxisAlignedBox intersection(const AxisAlignedBox& b2) const
		{
            if (this->isNull() || b2.isNull())
			{
				return AxisAlignedBox();
			}
			else if (this->isInfinite())
			{
				return b2;
			}
			else if (b2.isInfinite())
			{
				return *this;
			}

			Vector3 intMin = m_minimum;
            Vector3 intMax = m_maximum;

            intMin.makeCeil(b2.getMinimum());
            intMax.makeFloor(b2.getMaximum());

            // Check intersection isn't null
            if (intMin.x < intMax.x &&
                intMin.y < intMax.y &&
                intMin.z < intMax.z)
            {
                return AxisAlignedBox(intMin, intMax);
            }

            return AxisAlignedBox();
		}

		/// Calculate the volume of this box
		float volume() const
		{
			switch (m_extent)
			{
			case EXTENT_NULL:
				return 0.0f;

			case EXTENT_FINITE:
				{
					Vector3 diff = m_maximum - m_minimum;
					return diff.x * diff.y * diff.z;
				}

			case EXTENT_INFINITE:
				return Math::POS_INFINITY;

			default: // shut up compiler
				khaosAssert( false );
				return 0.0f;
			}
		}

		/** Scales the AABB by the vector given. */
		void scale(const Vector3& s)
		{
			// Do nothing if current null or infinite
			if (m_extent != EXTENT_FINITE)
				return;

			// NB assumes centered on origin
			Vector3 min = m_minimum * s;
			Vector3 max = m_maximum * s;
			setExtents(min, max);
		}

		/** Tests whether this box intersects a sphere. */
		bool intersects(const Sphere& s) const
		{
			return Math::intersects(s, *this); 
		}

		/** Tests whether this box intersects a plane. */
		bool intersects(const Plane& p) const
		{
			return Math::intersects(p, *this);
		}

		/** Tests whether the vector point is within this box. */
		bool intersects(const Vector3& v) const
		{
			switch (m_extent)
			{
			case EXTENT_NULL:
				return false;

			case EXTENT_FINITE:
				return(v.x >= m_minimum.x  &&  v.x <= m_maximum.x  && 
					v.y >= m_minimum.y  &&  v.y <= m_maximum.y  && 
					v.z >= m_minimum.z  &&  v.z <= m_maximum.z);

			case EXTENT_INFINITE:
				return true;

			default: // shut up compiler
				khaosAssert( false );
				return false;
			}
		}

		/// Gets the centre of the box
		Vector3 getCenter() const
		{
			khaosAssert( (m_extent == EXTENT_FINITE) );

			return Vector3(
				(m_maximum.x + m_minimum.x) * 0.5f,
				(m_maximum.y + m_minimum.y) * 0.5f,
				(m_maximum.z + m_minimum.z) * 0.5f);
		}

        float getCenterX() const
        {
            khaosAssert( (m_extent == EXTENT_FINITE) );
            return (m_maximum.x + m_minimum.x) * 0.5f;
        }

        float getCenterY() const
        {
            khaosAssert( (m_extent == EXTENT_FINITE) );
            return (m_maximum.y + m_minimum.y) * 0.5f;
        }

        float getCenterZ() const
        {
            khaosAssert( (m_extent == EXTENT_FINITE) );
            return (m_maximum.z + m_minimum.z) * 0.5f;
        }

		/// Gets the size of the box
		Vector3 getSize() const
		{
			switch (m_extent)
			{
			case EXTENT_NULL:
				return Vector3::ZERO;

			case EXTENT_FINITE:
				return m_maximum - m_minimum;

			case EXTENT_INFINITE:
				return Vector3(
					Math::POS_INFINITY,
					Math::POS_INFINITY,
					Math::POS_INFINITY);

			default: // shut up compiler
				khaosAssert( false );
				return Vector3::ZERO;
			}
		}

		/// Gets the half-size of the box
		Vector3 getHalfSize() const
		{
			switch (m_extent)
			{
			case EXTENT_NULL:
				return Vector3::ZERO;

			case EXTENT_FINITE:
				return (m_maximum - m_minimum) * 0.5;

			case EXTENT_INFINITE:
				return Vector3(
					Math::POS_INFINITY,
					Math::POS_INFINITY,
					Math::POS_INFINITY);

			default: // shut up compiler
				khaosAssert( false );
				return Vector3::ZERO;
			}
		}

        /** Tests whether the given point contained by this box.
        */
        bool contains(const Vector3& v) const
        {
            if (isNull())
                return false;
            if (isInfinite())
                return true;

            return m_minimum.x <= v.x && v.x <= m_maximum.x &&
                   m_minimum.y <= v.y && v.y <= m_maximum.y &&
                   m_minimum.z <= v.z && v.z <= m_maximum.z;
        }
		
		/** Returns the minimum distance between a given point and any part of the box. */
		float distance(const Vector3& v) const
		{
			
			if (this->contains(v))
				return 0;
			else
			{
				float maxDist = std::numeric_limits<float>::min();

				if (v.x < m_minimum.x)
					maxDist = std::max(maxDist, m_minimum.x - v.x);
				if (v.y < m_minimum.y)
					maxDist = std::max(maxDist, m_minimum.y - v.y);
				if (v.z < m_minimum.z)
					maxDist = std::max(maxDist, m_minimum.z - v.z);
				
				if (v.x > m_maximum.x)
					maxDist = std::max(maxDist, v.x - m_maximum.x);
				if (v.y > m_maximum.y)
					maxDist = std::max(maxDist, v.y - m_maximum.y);
				if (v.z > m_maximum.z)
					maxDist = std::max(maxDist, v.z - m_maximum.z);
				
				return maxDist;
			}
		}

        /** Tests whether another box contained by this box.
        */
        bool contains(const AxisAlignedBox& other) const
        {
            if (other.isNull() || this->isInfinite())
                return true;

            if (this->isNull() || other.isInfinite())
                return false;

            return this->m_minimum.x <= other.m_minimum.x &&
                   this->m_minimum.y <= other.m_minimum.y &&
                   this->m_minimum.z <= other.m_minimum.z &&
                   other.m_maximum.x <= this->m_maximum.x &&
                   other.m_maximum.y <= this->m_maximum.y &&
                   other.m_maximum.z <= this->m_maximum.z;
        }

        /** Tests 2 boxes for equality.
        */
        bool operator==(const AxisAlignedBox& rhs) const
        {
            if (this->m_extent != rhs.m_extent)
                return false;

            if (!this->isFinite())
                return true;

            return this->m_minimum == rhs.m_minimum &&
                   this->m_maximum == rhs.m_maximum;
        }

        /** Tests 2 boxes for inequality.
        */
        bool operator!=(const AxisAlignedBox& rhs) const
        {
            return !(*this == rhs);
        }

    public:
		// special values
		static const AxisAlignedBox BOX_NULL;
        static const AxisAlignedBox BOX_UNIT;
		static const AxisAlignedBox BOX_INFINITE;
	};
} 

