#include "KhaosPreHeaders.h"
#include "KhaosEnvProbeNode.h"
#include "KhaosRenderTarget.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    EnvProbe::EnvProbe() : m_resolution(128), m_zNear(0.1f), m_zFar(100.0f), m_rtt(KHAOS_NEW RenderTargetCube)
    {
        m_rtt->createInnerCamera();
        setResolution( m_resolution );
        setNear( m_zNear );
        setFar( m_zFar );
    }

    EnvProbe::~EnvProbe()
    {
        KHAOS_DELETE m_rtt;
    }

    void EnvProbe::setResolution( int s )  
    {
        m_resolution = s;
        m_rtt->setSize( s );
    }

    void EnvProbe::setNear( float n ) 
    {
        m_zNear = n;
        m_rtt->setNearFar( m_zNear, m_zFar );
    }

    void EnvProbe::setFar( float f ) 
    {
        m_zFar = f;
        m_rtt->setNearFar( m_zNear, m_zFar );
        _updateAABB();
    }

    void EnvProbe::setPos( const Vector3& pos )
    {
        m_pos = pos;
        m_rtt->setCamPos( pos );
        _updateAABB();
    }

    void EnvProbe::_updateAABB()
    {
        Vector3 halfSize(m_zFar, m_zFar, m_zFar);
        m_aabb.setExtents( m_pos - halfSize, m_pos + halfSize );
        _fireAABBDirty();
    }

    //////////////////////////////////////////////////////////////////////////
    EnvProbeNode::EnvProbeNode() : m_probe(KHAOS_NEW EnvProbe)
    {
        m_type = NT_ENVPROBE;
        m_featureFlag.unsetFlag( TS_ENABLE_SCALE | TS_ENABLE_ROT ); // 不允许缩放旋转
        m_probe->setObjectListener( this );
    }

    EnvProbeNode::~EnvProbeNode()
    {
        KHAOS_DELETE m_probe;
    }

    bool EnvProbeNode::_checkUpdateTransform()
    {
        if ( SceneNode::_checkUpdateTransform() )
        {
            // 将节点变换数据设置给probe
            m_probe->setPos( m_derivedMatrix.getTrans() );
            return true;
        }

        return false;
    }

    void EnvProbeNode::_makeWorldAABB()
    {
        m_worldAABB = m_probe->getAABB();
    }
}

