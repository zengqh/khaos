#include "KhaosPreHeaders.h"
#include "KhaosVolumeProbeNode.h"
#include "KhaosSceneGraph.h"
#include "KhaosSysResManager.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    const Matrix4& VolProbRenderable::getImmMatWorld()
    {
        return m_node->getDerivedMatrix();
    }

    const AxisAlignedBox& VolProbRenderable::getImmAABBWorld()
    {
        return m_node->getWorldAABB();
    }

    void VolProbRenderable::render()
    {
        g_sysResManager->getVolProbeCubeMesh()->drawSub(0);
    }

    //////////////////////////////////////////////////////////////////////////
    VolumeProbe::VolumeProbe( SceneNode* owner ) : 
         m_owner(owner), m_volID(-1), m_resolutionX(0), m_resolutionY(0), m_resolutionZ(0)
    {
    }

    VolumeProbe::~VolumeProbe()
    {
        unload();
    }

    void VolumeProbe::setVolID( int volID )
    {
        //khaosAssert( volID >= 0 );
        m_volID = volID;
    }

    void VolumeProbe::setResolution( int cx, int cy, int cz )
    {
        m_resolutionX = cx;
        m_resolutionY = cy;
        m_resolutionZ = cz;
    }

    bool VolumeProbe::isPrepared() const
    {
        if ( m_volID >= 0 )
        {
            VolumeProbeData* vpd = m_owner->getSceneGraph()->getInstSharedDataMgr().getVolumeProbe( m_volID );

            if ( vpd )
            {
                return vpd->isPrepared();
            }
        }

        return false;
    }

    void VolumeProbe::load( int volID, const String& files )
    {
        setVolID( volID );
        load( files );
    }

    void VolumeProbe::load( const String& files )
    {
        if ( m_volID >= 0 )
        {
            VolumeProbeData* vpd = m_owner->getSceneGraph()->getInstSharedDataMgr().getVolumeProbe( m_volID );
            if ( !vpd )
                vpd = m_owner->getSceneGraph()->getInstSharedDataMgr().createVolumeProbe( m_volID );
            vpd->load( files );
        }
    }

    void VolumeProbe::unload()
    {
        if ( m_volID >= 0 )
        {
            m_owner->getSceneGraph()->getInstSharedDataMgr().removeVolumeProbe( m_volID );
        }        
    }

    void VolumeProbe::updateMatrix()
    {
        // 假想本地为单位cube，最终的缩放即体大小
        Vector3 pos;
        Quaternion rot;
        getWorldMatrix().decomposition( pos, m_volumeSize, rot );

        m_gridSize = m_volumeSize / 
            Vector3((float)(m_resolutionX), (float)(m_resolutionY), (float)(m_resolutionZ));
    }

    const Matrix4& VolumeProbe::getWorldMatrix() const
    {
        return m_owner->getDerivedMatrix();
    }

    Vector3 VolumeProbe::getCenterWorldPos( int ix, int iy, int iz ) const
    {
        // 首先构造局部位置
        // 由于局部是个单位cube
        Vector3 localGridSize = Vector3::UNIT_SCALE / Vector3((float)m_resolutionX, (float)m_resolutionY, (float)m_resolutionZ);

        Vector3 localPos = localGridSize * Vector3( ix+0.5f, iy+0.5f, iz+0.5f ) + Vector3(-0.5f);

        // 局部坐标变换到世界
        return getWorldMatrix().transformAffine( localPos );
    }

    const Vector3& VolumeProbe::getGridSize() const
    {
        return m_gridSize;
    }

    const Vector3& VolumeProbe::getVolumeSize() const
    {
        return m_volumeSize;
    }

    //////////////////////////////////////////////////////////////////////////
    VolumeProbeNode::VolumeProbeNode()
    {
        m_type = NT_VOLPROBE;

        m_probe = KHAOS_NEW VolumeProbe(this);
        m_probe->setObjectListener( this );

        m_volProbeRenderable._setNode( this );
    }

    VolumeProbeNode::~VolumeProbeNode()
    {
        KHAOS_DELETE m_probe;
    }

    void VolumeProbeNode::_onDestroyBefore() 
    {
        m_probe->unload();
        m_probe->setVolID(-1);
    }

    bool VolumeProbeNode::_checkUpdateTransform()
    {
        if ( SceneNode::_checkUpdateTransform() )
        {
            // 将节点变换数据设置给probe
            m_probe->updateMatrix();
            return true;
        }

        return false;
    }

    void VolumeProbeNode::_makeWorldAABB()
    {
        m_worldAABB = AxisAlignedBox::BOX_UNIT;
        m_worldAABB.transformAffine( this->getDerivedMatrix() );
    }
}

