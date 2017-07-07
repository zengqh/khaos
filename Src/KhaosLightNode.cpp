#include "KhaosPreHeaders.h"
#include "KhaosLightNode.h"
#include "KhaosMesh.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    const Matrix4& VolumeRenderable::getImmMatWorld()
    {
        return m_node->getLight()->getVolumeWorldMatrix();
    }

    const AxisAlignedBox& VolumeRenderable::getImmAABBWorld()
    {
        return m_node->getLight()->getVolumeAABB();
    }

    void VolumeRenderable::render()
    {
        m_node->getLight()->getVolume()->drawSub(0);
    }

    //////////////////////////////////////////////////////////////////////////
    LightNode::LightNode() : m_light(0)
    {
        m_type = NT_LIGHT;
        m_featureFlag.unsetFlag( TS_ENABLE_SCALE ); // 不允许缩放
        m_volRenderable._setNode( this );
    }

    LightNode::~LightNode()
    {
        KHAOS_DELETE m_light;
    }

    void LightNode::setLightType( LightType type )
    {
        if ( m_light && m_light->getLightType() != type )
        {
            KHAOS_SAFE_DELETE( m_light );
        }

        if ( !m_light )
        {
            m_light = LightFactory::createLight(type);
            m_light->setObjectListener( this );
            _setSelfTransformDirty();
        }
    }

    bool LightNode::_checkUpdateTransform()
    {
        if ( SceneNode::_checkUpdateTransform() )
        {
            // 将节点变换数据设置给Light
            m_light->setTransform( m_derivedMatrix );
            return true;
        }

        return false;
    }

    void LightNode::_makeWorldAABB()
    {
        m_worldAABB = m_light->getAABB();
    }

    bool LightNode::intersects( const AxisAlignedBox& aabb ) const
    {
        const_cast<LightNode*>(this)->_checkUpdateTransform();
        return m_light->intersects( aabb );
    }

    float LightNode::squaredDistance( const Vector3& pos ) const
    {
        const_cast<LightNode*>(this)->_checkUpdateTransform();
        return m_light->squaredDistance( pos );
    }
}

