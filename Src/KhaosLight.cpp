#include "KhaosPreHeaders.h"
#include "KhaosLight.h"
#include "KhaosCamera.h"
#include "KhaosTextureObj.h"
#include "KhaosRenderDevice.h"
#include "KhaosObjectAlignedBox.h"
#include "KhaosRenderTarget.h"
#include "KhaosSysResManager.h"
#include "KhaosRayQuery.h"

namespace Khaos
{
    const float LIT_TEST_LEN_ERROR = 0.01f;
    const float LIT_TEST_ADD_LEN   = 0.1f;   

    //////////////////////////////////////////////////////////////////////////
    Light::Light() : 
        m_lightType(LT_DIRECTIONAL),
        m_position(Vector3::ZERO), 
        m_direction(Vector3::NEGATIVE_UNIT_Y), m_up(Vector3::NEGATIVE_UNIT_Z), m_right(Vector3::UNIT_X),
        m_diffuse(Color::WHITE),
        m_tmpId(0)
    {
        m_dirty.setFlag( DIRTY_PROPERTY );
    }

    Light::~Light()
    {
    }

    void Light::setPosition( const Vector3& pos )
    {
        m_position = pos;
        _setPropertyDirty();
    }

    void Light::setDirection( const Vector3& direction )
    {
        m_direction = direction;
        _setPropertyDirty();
    }

    void Light::setUp( const Vector3& up )
    {
        m_up = up;
        _setPropertyDirty();
    }

    void Light::setDiffuse( const Color& diffuse )
    {
        m_diffuse = diffuse;
    }

    const Vector3& Light::getPosition()  const
    {
        const_cast<Light*>(this)->_update();
        return m_position;
    }

    const Vector3& Light::getDirection() const
    {
        const_cast<Light*>(this)->_update();
        return m_direction;
    }

    const Vector3& Light::getUp() const
    {
        const_cast<Light*>(this)->_update();
        return m_up;
    }

    const Vector3& Light::getRight() const
    {
        const_cast<Light*>(this)->_update();
        return m_right;
    }

    const Color& Light::getDiffuse() const
    {
        return m_diffuse;
    }

    void Light::setTransform( const Matrix4& matWorld )
    {
        setPosition( matWorld.getTrans() );
        setDirection( -matWorld.getAxisZ() );
        setUp( matWorld.getAxisY() );
    }

    const Matrix4& Light::getWorldMatrix() const
    {
        const_cast<Light*>(this)->_update();
        return m_matWorld;
    }

    const AxisAlignedBox& Light::getAABB() const
    {
        // 返回当前的aabb
        const_cast<Light*>(this)->_update();
        return m_aabb;
    }

    bool Light::intersects( const AxisAlignedBox& aabb ) const
    {
        return getAABB().intersects( aabb );
    }

    void Light::_checkLightProp()
    {
        // 变换更新
        m_direction.normalise();
        m_right = m_direction.crossProduct( m_up );
        m_right.normalise();
        m_up = m_right.crossProduct( m_direction );
        m_matWorld.makeTransform( m_position, Vector3::UNIT_SCALE, Quaternion(m_right, m_up, -m_direction) );
    }

    void Light::_update()
    {
        // 检查属性
        if ( m_dirty.testFlag( DIRTY_PROPERTY ) )
        {
            _checkLightProp();
            _calcAABB();
            m_dirty.unsetFlag( DIRTY_PROPERTY );
        }
    }

    void Light::_setPropertyDirty()
    {
        m_dirty.setFlag( DIRTY_PROPERTY );
        _fireAABBDirty();
    }

    float Light::_glossyToPower( float glossy )
    {
        const float g_base   = 8192;
        const float g_scale  = 1;
        const float g_offset = 0;

        float power = Math::pow( g_base, g_scale * glossy + g_offset );
        return power;
    }

    //////////////////////////////////////////////////////////////////////////
    DirectLight::DirectLight()
    {
        m_lightType = LT_DIRECTIONAL;
    }

    DirectLight::~DirectLight()
    {
    }

    float DirectLight::squaredDistance( const Vector3& pos ) const
    {
        return 0;
    }

    void DirectLight::_calcAABB()
    {
        // 平行光无限大
        m_aabb.setInfinite();
    }

    ShadowTechBase* DirectLight::_createShadowTech( ShadowType type )
    {
        switch ( type )
        {
        case ST_SM:
        case ST_PSSM:
            return KHAOS_NEW ShadowTechDirect;
        }

        return 0;
    }

    bool DirectLight::_canArrivePos( SceneGraph* sg, const Vector3& pos ) const
    {
        const float maxDirLitLen = 8000.0f;
        Vector3 ori = pos - getDirection() * maxDirLitLen;
        LimitRay ray( ori, getDirection(), maxDirLitLen + LIT_TEST_ADD_LEN ); // 给点冗余
        
        float t = 0;
        if ( rayIntersectSGDetail( sg, ray, 0, 0, &t ) ) // 有交点
        {
            return Math::fabs(maxDirLitLen - t) < LIT_TEST_LEN_ERROR; // 和测试点误差在允许内
        }

        return false; 
    }

    void DirectLight::_calcPointIrradiance( const Vector3& pos, const Vector3& norm, Color& clrIrr ) const
    {
        Vector3 vL = -getDirection();
        clrIrr = (getDiffuse() * Math::maxVal( norm.dotProduct(vL), 0.0f )).getRGB();
    }

    //////////////////////////////////////////////////////////////////////////
    PointLight::PointLight() : m_range(0.0f), m_fadePower(1.0f)
    {
        m_lightType = LT_POINT;
    }
    
    PointLight::~PointLight()
    {
    }

    void PointLight::setRange( float range )
    {
        m_range = range;
        _setPropertyDirty();
    }

    void PointLight::setFadePower( float fade )
    {
        m_fadePower = fade;
        _setPropertyDirty();
    }

    float PointLight::getRange() const
    {
        const_cast<PointLight*>(this)->_update();
        return m_range;
    }

    float PointLight::getFadePower() const
    {
        const_cast<PointLight*>(this)->_update();
        return m_fadePower;
    }

    float PointLight::getDistAtt( const Vector3& vL ) const
    {
        Vector3 vDist = vL / getRange();
        float fFallOff = Math::saturate(1.0f - vDist.dotProduct(vDist));
        return Math::pow( fFallOff, getFadePower() );
    }

    float PointLight::squaredDistance( const Vector3& pos ) const
    {
        return m_position.squaredDistance( pos );
    }

    Mesh* PointLight::getVolume() const
    {
        return g_sysResManager->getPointLitMesh();
    }

    const Matrix4& PointLight::getVolumeWorldMatrix() const 
    {
        const_cast<PointLight*>(this)->_update();
        return m_volMatWorld; 
    }

    const AxisAlignedBox& PointLight::getVolumeAABB() const 
    {
        const_cast<PointLight*>(this)->_update();
        return m_volAABB; 
    }

    void PointLight::_checkLightProp()
    {
        Light::_checkLightProp();

        if ( m_range < 0 )
            m_range = 0;

        if ( m_fadePower < 0.001f )
            m_fadePower = 0.001f;

        // 模型是半径略微大于1的球
        m_volMatWorld.makeTransform( m_position, Vector3(m_range), Quaternion::IDENTITY );
    }

    void PointLight::_calcAABB()
    {
        // 点光源
        Vector3 size(m_range, m_range, m_range);
        m_aabb.setExtents( m_position-size, m_position+size );

        // 这里准确计算volaabb，模型是半径略微大于1的球
        Vector3 volRadius = getVolume()->getAABB().getHalfSize() * m_range;
        m_volAABB.setExtents( m_position-volRadius, m_position+volRadius );
    }

    bool PointLight::_canArrivePos( SceneGraph* sg, const Vector3& pos ) const
    {
        Vector3 vL = pos - getPosition();
        float  vL_len = vL.normalise();
        
        if ( vL_len > getRange() ) // 光照范围外了
            return false;

        LimitRay ray( getPosition(), vL, vL_len );

        float t = 0;
        if ( rayIntersectSGDetail( sg, ray, 0, 0, &t ) ) // 有交点
        {
            return vL_len - t < LIT_TEST_LEN_ERROR;
        }

        return false; 
    }

    void PointLight::_calcPointIrradiance( const Vector3& pos, const Vector3& norm, Color& clrIrr ) const
    {
        Vector3 vL = getPosition() - pos;

        float att = getDistAtt(vL);
        vL.normalise();

        clrIrr = (getDiffuse() * (Math::maxVal(norm.dotProduct(vL), 0.0f) * att)).getRGB();
    }

    //////////////////////////////////////////////////////////////////////////
    SpotLight::SpotLight() : m_innerCone(0), m_outerCone(0), m_outerRadius(0)
    {
        m_lightType = LT_SPOT;
    }

    SpotLight::~SpotLight()
    {
    }

    void SpotLight::setInnerCone( float angle )
    {
        m_innerCone = angle;
        _setPropertyDirty();
    }

    void SpotLight::setOuterCone( float angle )
    {
        m_outerCone = angle;
        _setPropertyDirty();
    }

    float SpotLight::getInnerCone() const
    {
        const_cast<SpotLight*>(this)->_update();
        return m_innerCone;
    }

    float SpotLight::getOuterCone() const
    {
        const_cast<SpotLight*>(this)->_update();
        return m_outerCone;
    }

    const Vector2& SpotLight::_getSpotAngles() const
    {
        return m_spotAngles;
    }

    float SpotLight::squaredDistance( const Vector3& pos ) const
    {
        return m_centerSpot.squaredDistance( pos );
    }

    Mesh* SpotLight::getVolume() const
    {
        return g_sysResManager->getSpotLitMesh();
    }

    void SpotLight::_checkLightProp()
    {
        PointLight::_checkLightProp();

        m_outerCone = Math::clamp( m_outerCone, (float)0, Math::HALF_PI ); // 由于是半角，最大90
        m_innerCone = Math::clamp( m_innerCone, (float)0, m_outerCone ); // 内角不能超过外角
        m_outerRadius = m_range * Math::tan( m_outerCone ); // 外圆的半径

        m_spotAngles.x = Math::cos( m_outerCone );
        m_spotAngles.y = 1.0f / (Math::cos( m_innerCone ) - m_spotAngles.x);

        // 模型是外圆半径略大于1，高1的锥
        m_volMatWorld.makeTransform( m_position, Vector3(m_outerRadius, m_outerRadius, m_range), 
            Quaternion(m_right, m_up, -m_direction) );
    }

    void SpotLight::_makeSpotAABB( const Vector3& pos, const Vector3& xdir, const Vector3& ydir, const Vector3& zdir,
        float radius, float range, Vector3& centerSpot, AxisAlignedBox& aabb )
    {
        float halfRange = range * 0.5f;
        centerSpot = pos - zdir * halfRange;
        Vector3 halfSize(radius, radius, halfRange);

        Vector3 corners[8];
        ObjectAlignedBox::getCorners( centerSpot, xdir, ydir, zdir, halfSize, corners );

        aabb.setNull();
        aabb.merge( corners, 8, sizeof(Vector3) ); 
    }

    void SpotLight::_calcAABB()
    {
        // 聚光灯
        Vector3  zdir = -m_direction;
        Vector3& ydir = m_up;
        Vector3& xdir = m_right;

        _makeSpotAABB( m_position, xdir, ydir, zdir, m_outerRadius, m_range, m_centerSpot, m_aabb );

        // 这里准确计算volaabb
        const AxisAlignedBox& volMeshAABB = getVolume()->getAABB();
        Vector3 volHalfSize = volMeshAABB.getHalfSize();
        float volRadius = Math::maxVal(volHalfSize.x, volHalfSize.y) * m_outerRadius;
        float volRange  = volMeshAABB.getSize().z * m_range;
        Vector3 volCenterSpot;
        _makeSpotAABB( m_position, xdir, ydir, zdir, volRadius, volRange, volCenterSpot, m_volAABB );
    }

    bool SpotLight::_canArrivePos( SceneGraph* sg, const Vector3& pos ) const
    {
        Vector3 vL = pos - getPosition();
        float  vL_len = vL.normalise();

        if ( vL_len > getRange() ) // 光照范围外了
            return false;

        LimitRay ray( getPosition(), vL, vL_len );

        float t = 0;
        if ( rayIntersectSGDetail( sg, ray, 0, 0, &t ) ) // 有交点
        {
            return vL_len - t < LIT_TEST_LEN_ERROR;
        }

        return false; 
    }

    void SpotLight::_calcPointIrradiance( const Vector3& pos, const Vector3& norm, Color& clrIrr ) const
    {
        Vector3 vL = getPosition() - pos;

        float att = getDistAtt(vL);
        vL.normalise();

        Vector3 spotDir = -getDirection();
        float spotAtt = Math::sqr( Math::saturate( (spotDir.dotProduct(vL) - _getSpotAngles().x) * _getSpotAngles().y ) );
        att *= spotAtt;

        clrIrr = (getDiffuse() * (Math::maxVal(norm.dotProduct(vL), 0.0f) * att)).getRGB();
    }

    //////////////////////////////////////////////////////////////////////////
    Light* LightFactory::createLight( LightType type )
    {
        switch (type)
        {
        case LT_DIRECTIONAL:
            return KHAOS_NEW DirectLight;

        case LT_POINT:
            return KHAOS_NEW PointLight;

        case LT_SPOT:
            return KHAOS_NEW SpotLight;

        default:
            khaosAssert(0);
            return 0;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    LightsInfo::LightsInfo()
    {
        for ( int i = 0; i < LT_MAX; ++i )
        {
            m_litList[i] = 0;
        }
    }

    LightsInfo::~LightsInfo()
    {
        for ( int i = 0; i < LT_MAX; ++i )
        {
            if ( LightListInfo* ll = m_litList[i] )
                KHAOS_DELETE ll;
        }
    }

    void LightsInfo::addLight( Light* lit, float dist, bool inSm )
    {
        LightListInfo*& ll = m_litList[lit->getLightType()];
        if ( !ll )
            ll = KHAOS_NEW LightListInfo;
        ll->litList.push_back( LightItem(lit, dist, inSm) );
    }

    void LightsInfo::clear()
    {
        for ( int i = 0; i < LT_MAX; ++i )
        {
            if ( LightListInfo* ll = m_litList[i] )
                ll->clear();
        }
    }

    void LightsInfo::sortN( LightType ltype, int maxN )
    {
        khaosAssert( 0 <= ltype && ltype < LT_MAX );
        khaosAssert( 0 <= maxN && maxN <= 4 );
        
        LightListInfo* ll = m_litList[ltype];
        if ( !ll )
            return;

        // 按照距离排序前maxN个
        int ll_size = (int)ll->litList.size();
        if ( ll_size > maxN )
        {
            std::nth_element( ll->litList.begin(), ll->litList.begin() + maxN, ll->litList.end(), _sortByDist );
            ll_size = maxN;
        }

        ll->maxN = ll_size;

        // 统计列表id
        ll->listID = -1;
        uint8* tmpids = (uint8*)&ll->listID;

        for ( int i = 0; i < ll_size; ++i )
        {
            tmpids[i] = ll->litList[i].lit->_getTempId();
        }

        std::sort( tmpids, tmpids+ll_size );
    }

    LightsInfo::LightListInfo* LightsInfo::getLightListInfo( LightType ltype ) const
    {
        khaosAssert( 0 <= ltype && ltype < LT_MAX );
        return m_litList[ltype];
    }

    LightsInfo::LightList* LightsInfo::getLightList( LightType ltype ) const
    {
        khaosAssert( 0 <= ltype && ltype < LT_MAX );
        if ( LightListInfo* ll = m_litList[ltype] )
            return &ll->litList;
        return 0;
    }

    int LightsInfo::getLightsCount( LightType ltype ) const
    {
        khaosAssert( 0 <= ltype && ltype < LT_MAX );
        if ( LightListInfo* ll = m_litList[ltype] )
            return (int)ll->litList.size();
        return 0;
    }

    bool LightsInfo::LightListInfo::isGreaterEqual( const LightListInfo& rhs ) const
    {
        // 右边为空集则总是满足
        int rhsCnt = rhs.maxN;
        if ( rhsCnt <= 0 )
            return true;

        // 个数至少大于等于右边
        int lhsCnt = this->maxN;
        if ( lhsCnt < rhsCnt )
            return false;

        // 比较自己的列表（已排序）是否包容右边
        static const uint32 mask_tab[4] =
        {
            0xff,
            0xffff,
            0xffffff,
            0xffffffff
        };

        khaosAssert( 1 <= rhsCnt && rhsCnt <= 4 );
        uint32 mask_cur = mask_tab[rhsCnt-1];
        
        uint32 lhs_id = listID & mask_cur;
        uint32 rhs_id = rhs.listID & mask_cur;
        return lhs_id == rhs_id;
    }
}

