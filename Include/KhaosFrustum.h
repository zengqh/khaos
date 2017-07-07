#pragma once
#include "KhaosSceneObject.h"
#include "KhaosPlane.h"
#include "KhaosMatrix4.h"
#include "KhaosAxisAlignedBox.h"
#include "KhaosBitSet.h"

namespace Khaos
{
    class Frustum : public SceneObject
    {
    public:
        enum ProjectionType
        {
            PT_ORTHOGRAPHIC,
            PT_PERSPECTIVE
        };

        enum FrustumPlane
        {
            PLANE_NEAR   = 0,
            PLANE_FAR    = 1,
            PLANE_LEFT   = 2,
            PLANE_RIGHT  = 3,
            PLANE_TOP    = 4,
            PLANE_BOTTOM = 5
        };

        enum FrustumCorner
        {
            CORNER_NEAR = 0,
            CORNER_FAR  = 4
        };

        enum Visibility
        {
            NONE,
            PARTIAL,
            FULL
        };

    protected:
        static const uint8 DIRTY_VIEW       = KhaosBitSetFlag8(0);
        static const uint8 DIRTY_PROJECTION = KhaosBitSetFlag8(1);

    public:
        Frustum();
        virtual ~Frustum();
    
    public:
        void setPerspective( float fov, float aspect, float zNear, float zFar );
        void setPerspective2( float left, float top, float right, float bottom, float zNear, float zFar );
        void setPerspective2( float width, float height, float zNear, float zFar );

        void setOrtho( float left, float top, float right, float bottom, float zNear, float zFar );
        void setOrtho( float width, float height, float zNear, float zFar );

        void setTransform( const Vector3& eye, const Vector3& target, const Vector3& upDir );
        void setTransform( const Matrix4& matWorld );

        void applyJitter( const Vector2& jitter );

    public:
        ProjectionType getProjectionType() const { return m_projType; }

        float          getFov()            const { return m_fov; }
        float          getAspect()         const { return m_aspect; }

        float          getLeft()           const { return m_left; }
        float          getTop()            const { return m_top; }
        float          getRight()          const { return m_right; }
        float          getBottom()         const { return m_bottom; }

        float          getZNear()          const { return m_zNear; }
        float          getZFar()           const { return m_zFar; }

        const Matrix4& getWorldMatrix() const;
        const Matrix4& getViewMatrix() const;
        const Matrix4& getProjMatrix() const;
        const Matrix4& getViewProjMatrix() const;
        const Matrix4& getViewMatrixRD() const;
        const Matrix4& getProjMatrixRD() const;
        const Matrix4& getViewProjMatrixRD() const;

        const Vector3&          getEyePos() const;
        const Plane&            getPlane( FrustumPlane pla ) const;
        const Vector3*          getCorners() const;
        const AxisAlignedBox&   getAABB() const;
        const Vector3*          getCamVecs() const;
        const Vector3*          getVolBasis() const;

    public:
        bool testVisibility( const AxisAlignedBox& bound ) const;
        Visibility testVisibilityEx( const AxisAlignedBox& bound ) const;

    protected:
        void _setViewDirty();
        void _setProjectionDirty();
        void _update();
        void _calcPerspectiveParas();
        void _makePerspectiveProj();
        void _makeOrthoProj();
        void _makePlanes();
        void _makeCorners();

    protected:
        // 投影类型
        ProjectionType m_projType;

        // 投影参数
        float m_fov;
        float m_aspect;

        float m_left;
        float m_top;
        float m_right;
        float m_bottom;

        float m_zNear;
        float m_zFar;

        // 矩阵
        Matrix4 m_matWorld;
        Matrix4 m_matView;
        Matrix4 m_matProj;
        Matrix4 m_matViewProj;

        Matrix4 m_matViewRD; // 实际设备的view/proj，根据dx/gl会不同
        Matrix4 m_matProjRD;
        Matrix4 m_matViewProjRD;

        // 眼睛位置(world空间)
        Vector3 m_eyePos;

        // 8个点(world空间)
        Vector3 m_corners[8];

        // 6个平面(world空间)
        Plane m_planes[6];

        // 包围盒(world空间)
        AxisAlignedBox m_aabb;

        // 角向量，位置重构用(world空间)
        Vector3 m_camVecs[4];

        // volume重构世界用的基向量
        Vector3 m_volBasis[3];

        // 是否脏
        BitSet8 m_dirty;
    };
}

