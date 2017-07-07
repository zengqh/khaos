#pragma once
#include "KhaosSceneObject.h"
#include "KhaosBitSet.h"
#include "KhaosColor.h"
#include "KhaosAxisAlignedBox.h"
#include "KhaosShadow.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    enum LightType
    {
        LT_DIRECTIONAL,
        LT_POINT,
        LT_SPOT,
        LT_MAX
    };

    class Mesh;
    class SceneGraph;

    //////////////////////////////////////////////////////////////////////////
    class Light : public SceneObject, public ShadowObject
    {
    private:
        static const uint8 DIRTY_PROPERTY  = KhaosBitSetFlag8(0);

    public:
        Light();
        virtual ~Light();

    public:
        // light general property
        LightType getLightType() const { return m_lightType; }

        void setPosition( const Vector3& pos );
        void setDirection( const Vector3& direction );
        void setUp( const Vector3& up );

        const Vector3& getPosition()  const;
        const Vector3& getDirection() const;
        const Vector3& getUp()        const;
        const Vector3& getRight()     const;

        void setDiffuse( const Color& diffuse );
        const Color& getDiffuse() const;

    public:
        void setTransform( const Matrix4& matWorld );

        const Matrix4& getWorldMatrix() const;
        const AxisAlignedBox& getAABB() const; // 返回world空间aabb

        bool  intersects( const AxisAlignedBox& aabb ) const;
        virtual float squaredDistance( const Vector3& pos ) const = 0;

        virtual Mesh* getVolume() const { return 0; }
        virtual const Matrix4& getVolumeWorldMatrix() const { return getWorldMatrix(); }
        virtual const AxisAlignedBox& getVolumeAABB() const { return getAABB(); }

    public:
        uint8 _getTempId() const { return m_tmpId; }
        void  _setTempId( uint8 id ) { m_tmpId = id; }

        virtual bool _canArrivePos( SceneGraph* sg, const Vector3& pos ) const { return false; }
        virtual void _calcPointIrradiance( const Vector3& pos, const Vector3& norm, Color& clrIrr ) const { clrIrr = Color::BLACK; }

        static float _glossyToPower( float glossy );

    protected:
        void _update();
        void _setPropertyDirty();

        virtual void _checkLightProp();
        virtual void _calcAABB() = 0;

    protected:
        virtual Light* _getHost() { return this; }

    protected:
        LightType       m_lightType;
        
        Vector3         m_position;
        Vector3         m_direction;
        Vector3         m_up;
        Vector3         m_right;

        Color           m_diffuse;

        Matrix4         m_matWorld;
        AxisAlignedBox  m_aabb;

        BitSet8         m_dirty;

        uint8           m_tmpId; // temporary id for render
    };

    //////////////////////////////////////////////////////////////////////////
    class DirectLight : public Light
    {
    public:
        DirectLight();
        virtual ~DirectLight();

    public:
        virtual float squaredDistance( const Vector3& pos ) const;

        virtual bool _canArrivePos( SceneGraph* sg, const Vector3& pos ) const;
        virtual void _calcPointIrradiance( const Vector3& pos, const Vector3& norm, Color& clrIrr ) const;

    protected:
        virtual void _calcAABB();
        virtual ShadowTechBase* _createShadowTech( ShadowType type );
    };

    class PointLight : public Light
    {
    public:
        PointLight();
        virtual ~PointLight();

        void setRange( float range );
        void setFadePower( float fade );

        float getRange()        const;
        float getFadePower()    const;
        float getDistAtt( const Vector3& vL ) const;

    public:
        virtual float squaredDistance( const Vector3& pos ) const;
        virtual Mesh* getVolume() const;
        virtual const Matrix4& getVolumeWorldMatrix() const;
        virtual const AxisAlignedBox& getVolumeAABB() const;

        virtual bool _canArrivePos( SceneGraph* sg, const Vector3& pos ) const;
        virtual void _calcPointIrradiance( const Vector3& pos, const Vector3& norm, Color& clrIrr ) const;

    protected:
        virtual void _checkLightProp();
        virtual void _calcAABB();

    protected:
        float m_range;    // point/spot range 
        float m_fadePower; // point/spot fade power
        Matrix4 m_volMatWorld;
        AxisAlignedBox m_volAABB;
    };

    class SpotLight : public PointLight
    {
    public:
        SpotLight();
        virtual ~SpotLight();

        void setInnerCone( float angle );
        void setOuterCone( float angle );

        float          getInnerCone()   const;
        float          getOuterCone()   const;
        const Vector2& _getSpotAngles() const;

    public:
        virtual float squaredDistance( const Vector3& pos ) const;
        virtual Mesh* getVolume() const;

        virtual bool _canArrivePos( SceneGraph* sg, const Vector3& pos ) const;
        virtual void _calcPointIrradiance( const Vector3& pos, const Vector3& norm, Color& clrIrr ) const;

    protected:
        virtual void _checkLightProp();
        virtual void _calcAABB();

        void _makeSpotAABB( const Vector3& pos, const Vector3& xdir, const Vector3& ydir, const Vector3& zdir,
            float radius, float range, Vector3& centerSpot, AxisAlignedBox& aabb );

    protected:
        float    m_innerCone;    // spot inner cone(half)
        float    m_outerCone;    // spot outer cone(half)
        float    m_outerRadius;  // spot outer radius

        Vector3  m_centerSpot;
        Vector2  m_spotAngles;   // x = cos(outerCone) y = 1 / (cos(innerCone) - cos(outerCone))
    };

    class LightFactory
    {
    public:
        static Light* createLight( LightType type );
    };

    //////////////////////////////////////////////////////////////////////////
    class LightsInfo : public AllocatedObject
    {
    public:
        struct LightItem
        {
            LightItem() : lit(0), dist(0), inSm(false) {}
            LightItem( Light* n, float d, bool sm ) : lit(n), dist(d), inSm(sm) {}

            Light* lit;  // 灯光
            float  dist; // 到灯光的距离
            bool   inSm; // 在当前视角下，是否在该灯光可视阴影距离内
        };

        typedef vector<LightItem>::type LightList;

        struct LightListInfo : public AllocatedObject
        {
            LightListInfo() : litList(0), listID(0), maxN(0) {}

            LightList  litList;
            uint32     listID;
            int        maxN;

            void clear()
            {
                litList.clear();
                listID = 0;
                maxN = 0;
            }

            bool isGreaterEqual( const LightListInfo& rhs ) const;
        };

    private:
        static bool _sortByDist( const LightItem& lhs, const LightItem& rhs )
        {
            return lhs.dist < rhs.dist;
        }

    public:
        LightsInfo();
        ~LightsInfo();

    public:
        void addLight( Light* lit, float dist, bool inSm );
        void clear();
        void sortN( LightType ltype, int maxN );

        int            getLightsCount( LightType ltype ) const;
        LightListInfo* getLightListInfo( LightType ltype ) const;
        LightList*     getLightList( LightType ltype ) const;

    private:
        LightListInfo* m_litList[LT_MAX];
    };
}

