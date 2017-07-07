#include "KhaosPreHeaders.h"
#include "KhaosFrustum.h"
#include "KhaosAxisAlignedBox.h"
#include "KhaosRenderDevice.h"


namespace Khaos
{
    Frustum::Frustum() :
        m_projType(PT_PERSPECTIVE),
        m_fov(Math::PI / 4.0f), m_aspect(1.0f),
        m_left(-10.0f), m_right(10.0f), m_top(10.0f), m_bottom(-10.0f),
        m_zNear(10.0f), m_zFar(10000.0f)
    {
        m_dirty.setFlag( DIRTY_VIEW | DIRTY_PROJECTION );
    }

    Frustum::~Frustum()
    {
    }

    void Frustum::setPerspective( float fov, float aspect, float zNear, float zFar )
    {
        m_projType = PT_PERSPECTIVE;

        m_fov    = fov;
        m_aspect = aspect;
        m_zNear  = zNear;
        m_zFar   = zFar;

        _calcPerspectiveParas();
        _setProjectionDirty();
    }

    void Frustum::setPerspective2( float left, float top, float right, float bottom, float zNear, float zFar )
    {
        m_projType = PT_PERSPECTIVE;

        m_left   = left;
        m_top    = top;
        m_right  = right;
        m_bottom = bottom;
        m_zNear  = zNear;
        m_zFar   = zFar;

        _setProjectionDirty();
    }

    void Frustum::setPerspective2( float width, float height, float zNear, float zFar )
    {
        float left   = width * -0.5f;
        float right  = width * 0.5f;
        float top    = height * 0.5f;
        float bottom = height * -0.5f;

        setPerspective2( left, top, right, bottom, zNear, zFar );
    }

    void Frustum::setOrtho( float left, float top, float right, float bottom, float zNear, float zFar )
    {
        m_projType = PT_ORTHOGRAPHIC;

        m_left   = left;
        m_top    = top;
        m_right  = right;
        m_bottom = bottom;
        m_zNear  = zNear;
        m_zFar   = zFar;

        _setProjectionDirty();
    }

    void Frustum::setOrtho( float width, float height, float zNear, float zFar )
    {
        float left   = width * -0.5f;
        float right  = width * 0.5f;
        float top    = height * 0.5f;
        float bottom = height * -0.5f;

        setOrtho( left, top, right, bottom, zNear, zFar );
    }

    void Frustum::setTransform( const Vector3& eye, const Vector3& target, const Vector3& upDir )
    {
        Math::makeTransformLookAt( m_matWorld, eye, target, upDir );
        m_matView = m_matWorld.inverse();
        m_eyePos = m_matWorld.getTrans();
        _setViewDirty();
    }

    void Frustum::setTransform( const Matrix4& matWorld )
    {
        //khaosAssert( !matWorld.hasScale() );
        m_matWorld = matWorld;
        m_matView = m_matWorld.inverse();
        m_eyePos = matWorld.getTrans();
        _setViewDirty();
    }

    const Vector3& Frustum::getEyePos() const
    {
        return m_eyePos;
    }

    const Plane& Frustum::getPlane( FrustumPlane pla ) const
    {
        const_cast<Frustum*>(this)->_update();
        return m_planes[pla];
    }

    const Vector3* Frustum::getCorners() const
    {
        const_cast<Frustum*>(this)->_update();
        return m_corners;
    }

    const Vector3* Frustum::getCamVecs() const
    {
        const_cast<Frustum*>(this)->_update();
        return m_camVecs;
    }

    const Vector3* Frustum::getVolBasis() const
    {
        const_cast<Frustum*>(this)->_update();
        return m_volBasis;
    }

    const AxisAlignedBox& Frustum::getAABB() const
    {
        const_cast<Frustum*>(this)->_update();
        return m_aabb;
    }

    const Matrix4& Frustum::getWorldMatrix() const
    {
        return m_matWorld;
    }

    const Matrix4& Frustum::getViewMatrix() const
    {
        const_cast<Frustum*>(this)->_update();
        return m_matView;
    }

    const Matrix4& Frustum::getProjMatrix() const
    {
        const_cast<Frustum*>(this)->_update();
        return m_matProj;
    }

    const Matrix4& Frustum::getViewProjMatrix() const
    {
        const_cast<Frustum*>(this)->_update();
        return m_matViewProj;
    }

    const Matrix4& Frustum::getViewMatrixRD() const
    {
        const_cast<Frustum*>(this)->_update();
        return m_matViewRD;
    }

    const Matrix4& Frustum::getProjMatrixRD() const
    {
        const_cast<Frustum*>(this)->_update();
        return m_matProjRD;
    }

    const Matrix4& Frustum::getViewProjMatrixRD() const
    {
        const_cast<Frustum*>(this)->_update();
        return m_matViewProjRD;
    }

    bool Frustum::testVisibility( const AxisAlignedBox& bound ) const
    {
        // Null boxes always invisible
        if ( bound.isNull() ) return false;

        // Infinite boxes always visible
        if ( bound.isInfinite() ) return true;

        // Make any pending updates to the calculated frustum planes
        const_cast<Frustum*>(this)->_update();

        // Get centre of the box
        Vector3 centre = bound.getCenter();
        // Get the half-size of the box
        Vector3 halfSize = bound.getHalfSize();

        // For each plane, see if all points are on the negative side
        // If so, object is not visible
        for (int plane = 0; plane < 6; ++plane)
        {
            Plane::Side side = m_planes[plane].getSide(centre, halfSize);
            if (side == Plane::NEGATIVE_SIDE)
            {
                // ALL corners on negative side therefore out of view
                return false;
            }
        }

        return true;
    }

    Frustum::Visibility Frustum::testVisibilityEx( const AxisAlignedBox& bound ) const
    {
        // Null boxes always invisible
        if ( bound.isNull() ) return NONE;

        // Infinite boxes always visible
        if ( bound.isInfinite() ) return PARTIAL;

        // Make any pending updates to the calculated frustum planes
        const_cast<Frustum*>(this)->_update();

        // Get centre of the box
        Vector3 centre = bound.getCenter();
        // Get the half-size of the box
        Vector3 halfSize = bound.getHalfSize();

        bool all_inside = true;

        for ( int plane = 0; plane < 6; ++plane )
        {
            // This updates frustum planes and deals with cull frustum
            Plane::Side side = m_planes[plane].getSide(centre, halfSize);
            if(side == Plane::NEGATIVE_SIDE) return NONE;
            // We can't return now as the box could be later on the negative side of a plane.
            if(side == Plane::BOTH_SIDE) 
                all_inside = false;
        }

        if ( all_inside )
            return FULL;
        else
            return PARTIAL;
    }

    void Frustum::applyJitter( const Vector2& jitter )
    {
        _update();

        Matrix4 matTransJitter;
        matTransJitter.makeTrans( jitter.x, jitter.y, 0 );

        // 注意，我们只改变渲染用project
        m_matProjRD = matTransJitter * m_matProj; // project之后在平移jitter
        g_renderDevice->toDeviceProjMatrix( m_matProjRD ); // 渲染设备用Proj
        m_matViewProjRD = m_matProjRD * m_matViewRD;
    }

    void Frustum::_update()
    {
        bool viewOrProjDirty = false;

        // view更新
        if ( m_dirty.testFlag(DIRTY_VIEW) )
        {
            viewOrProjDirty = true;

            m_matViewRD = m_matView;
            g_renderDevice->toDeviceViewMatrix( m_matViewRD ); // 渲染设备用View

            m_dirty.unsetFlag(DIRTY_VIEW);
        }

        // proj更新
        if ( m_dirty.testFlag(DIRTY_PROJECTION) )
        {
            viewOrProjDirty = true;

            if ( m_projType == PT_PERSPECTIVE )
                _makePerspectiveProj();
            else if ( m_projType == PT_ORTHOGRAPHIC )
                _makeOrthoProj();
            else
                khaosAssert(0);

            m_matProjRD = m_matProj;
            g_renderDevice->toDeviceProjMatrix( m_matProjRD ); // 渲染设备用Proj

            m_dirty.unsetFlag(DIRTY_PROJECTION);
        }

        if ( viewOrProjDirty )
        {
            m_matViewProj = m_matProj * m_matView;
            m_matViewProjRD = m_matProjRD * m_matViewRD;

            _makeCorners();
            _makePlanes();
        }
    }

    void Frustum::_calcPerspectiveParas()
    {
        // Calculate general projection parameters
        float thetaY = m_fov * 0.5f;
        float tanThetaY = Math::tan(thetaY);
        float tanThetaX = tanThetaY * m_aspect;

        float half_w = tanThetaX * m_zNear;
        float half_h = tanThetaY * m_zNear;

        m_left   = - half_w;
        m_right  = + half_w;
        m_bottom = - half_h;
        m_top    = + half_h;
    }

    void Frustum::_makePerspectiveProj()
    {
        float inv_w = 1 / (m_right - m_left);
        float inv_h = 1 / (m_top - m_bottom);
        float inv_d = 1 / (m_zFar - m_zNear);

        // Calc matrix elements
        float A = 2 * m_zNear * inv_w;
        float B = 2 * m_zNear * inv_h;
        float C = (m_right + m_left) * inv_w;
        float D = (m_top + m_bottom) * inv_h;
       
        float q = - (m_zFar + m_zNear) * inv_d;
        float qn = -2 * (m_zFar * m_zNear) * inv_d;

        // NB: This creates 'uniform' perspective projection matrix,
        // which depth range [-1,1], right-handed rules
        //
        // [ A   0   C   0  ]
        // [ 0   B   D   0  ]
        // [ 0   0   q   qn ]
        // [ 0   0   -1  0  ]
        //
        // A = 2 * near / (right - left)
        // B = 2 * near / (top - bottom)
        // C = (right + left) / (right - left)
        // D = (top + bottom) / (top - bottom)
        // q = - (far + near) / (far - near)
        // qn = - 2 * (far * near) / (far - near)

        m_matProj = Matrix4::ZERO;
        m_matProj[0][0] = A;
        m_matProj[0][2] = C;
        m_matProj[1][1] = B;
        m_matProj[1][2] = D;
        m_matProj[2][2] = q;
        m_matProj[2][3] = qn;
        m_matProj[3][2] = -1;
    }

    void Frustum::_makeOrthoProj()
    {
        float inv_w = 1 / (m_right - m_left);
        float inv_h = 1 / (m_top - m_bottom);
        float inv_d = 1 / (m_zFar - m_zNear);

        float A = 2 * inv_w;
        float B = 2 * inv_h;
        float C = - (m_right + m_left) * inv_w;
        float D = - (m_top + m_bottom) * inv_h;
      
        float q = - 2 * inv_d;
        float qn = - (m_zFar + m_zNear)  * inv_d;

        // NB: This creates 'uniform' orthographic projection matrix,
        // which depth range [-1,1], right-handed rules
        //
        // [ A   0   0   C  ]
        // [ 0   B   0   D  ]
        // [ 0   0   q   qn ]
        // [ 0   0   0   1  ]
        //
        // A = 2 * / (right - left)
        // B = 2 * / (top - bottom)
        // C = - (right + left) / (right - left)
        // D = - (top + bottom) / (top - bottom)
        // q = - 2 / (far - near)
        // qn = - (far + near) / (far - near)

        m_matProj = Matrix4::ZERO;
        m_matProj[0][0] = A;
        m_matProj[0][3] = C;
        m_matProj[1][1] = B;
        m_matProj[1][3] = D;
        m_matProj[2][2] = q;
        m_matProj[2][3] = qn;
        m_matProj[3][3] = 1;
    }

    void Frustum::_makePlanes()
    {
        // -------------------------
        // Update the frustum planes
        // -------------------------
        const Matrix4& combo = m_matViewProj;

        m_planes[PLANE_LEFT].normal.x = combo[3][0] + combo[0][0];
        m_planes[PLANE_LEFT].normal.y = combo[3][1] + combo[0][1];
        m_planes[PLANE_LEFT].normal.z = combo[3][2] + combo[0][2];
        m_planes[PLANE_LEFT].d = combo[3][3] + combo[0][3];

        m_planes[PLANE_RIGHT].normal.x = combo[3][0] - combo[0][0];
        m_planes[PLANE_RIGHT].normal.y = combo[3][1] - combo[0][1];
        m_planes[PLANE_RIGHT].normal.z = combo[3][2] - combo[0][2];
        m_planes[PLANE_RIGHT].d = combo[3][3] - combo[0][3];

        m_planes[PLANE_TOP].normal.x = combo[3][0] - combo[1][0];
        m_planes[PLANE_TOP].normal.y = combo[3][1] - combo[1][1];
        m_planes[PLANE_TOP].normal.z = combo[3][2] - combo[1][2];
        m_planes[PLANE_TOP].d = combo[3][3] - combo[1][3];

        m_planes[PLANE_BOTTOM].normal.x = combo[3][0] + combo[1][0];
        m_planes[PLANE_BOTTOM].normal.y = combo[3][1] + combo[1][1];
        m_planes[PLANE_BOTTOM].normal.z = combo[3][2] + combo[1][2];
        m_planes[PLANE_BOTTOM].d = combo[3][3] + combo[1][3];

        m_planes[PLANE_NEAR].normal.x = combo[3][0] + combo[2][0];
        m_planes[PLANE_NEAR].normal.y = combo[3][1] + combo[2][1];
        m_planes[PLANE_NEAR].normal.z = combo[3][2] + combo[2][2];
        m_planes[PLANE_NEAR].d = combo[3][3] + combo[2][3];

        m_planes[PLANE_FAR].normal.x = combo[3][0] - combo[2][0];
        m_planes[PLANE_FAR].normal.y = combo[3][1] - combo[2][1];
        m_planes[PLANE_FAR].normal.z = combo[3][2] - combo[2][2];
        m_planes[PLANE_FAR].d = combo[3][3] - combo[2][3];

        // Renormalise any normals which were not unit length
        for(int i=0; i<6; i++ ) 
        {
            float length = m_planes[i].normal.normalise();
            m_planes[i].d /= length;
        }
    }

    void Frustum::_makeCorners()
    {
        float radio = m_projType == PT_PERSPECTIVE ? (m_zFar / m_zNear) : 1;
        float farLeft = m_left * radio;
        float farRight = m_right * radio;
        float farBottom = m_bottom * radio;
        float farTop = m_top * radio;

        // near
        m_corners[0] = m_matWorld.transformAffine(Vector3(m_right, m_top,    -m_zNear));
        m_corners[1] = m_matWorld.transformAffine(Vector3(m_left,  m_top,    -m_zNear));
        m_corners[2] = m_matWorld.transformAffine(Vector3(m_left,  m_bottom, -m_zNear));
        m_corners[3] = m_matWorld.transformAffine(Vector3(m_right, m_bottom, -m_zNear));

        // far
        m_corners[4] = m_matWorld.transformAffine(Vector3(farRight,  farTop,     -m_zFar));
        m_corners[5] = m_matWorld.transformAffine(Vector3(farLeft,   farTop,     -m_zFar));
        m_corners[6] = m_matWorld.transformAffine(Vector3(farLeft,   farBottom,  -m_zFar));
        m_corners[7] = m_matWorld.transformAffine(Vector3(farRight,  farBottom,  -m_zFar));

        // cam vec
        m_camVecs[0] = m_corners[4] - m_eyePos;//m_corners[0];
        m_camVecs[1] = m_corners[5] - m_eyePos;//m_corners[1];
        m_camVecs[2] = m_corners[6] - m_eyePos;//m_corners[2];
        m_camVecs[3] = m_corners[7] - m_eyePos;//m_corners[3];

        // vol basis
        m_volBasis[0] = m_corners[7] - m_corners[6]; // +x
        m_volBasis[1] = m_corners[7] - m_corners[4]; // -y
        m_volBasis[2] = m_matWorld.getAxisZ() * -m_zFar; // -z

        m_volBasis[2] -= m_volBasis[0] * 0.5f; // 得到指向左上角的起始向量
        m_volBasis[2] -= m_volBasis[1] * 0.5f;

        // aabb
        m_aabb.setNull();
        m_aabb.merge( m_corners, 8, sizeof(Vector3) );
    }

    void Frustum::_setViewDirty()
    {
        m_dirty.setFlag(DIRTY_VIEW);
        _fireAABBDirty();
    }

    void Frustum::_setProjectionDirty()
    {
        m_dirty.setFlag(DIRTY_PROJECTION);
        _fireAABBDirty();
    }
}

