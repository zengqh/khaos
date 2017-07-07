#include "KhaosPreHeaders.h"
#include "KhaosCameraNode.h"


namespace Khaos
{

    //////////////////////////////////////////////////////////////////////////
    CameraNode::CameraNode() : m_camera(KHAOS_NEW Camera)
    {
        m_type = NT_CAMERA;
        m_featureFlag.unsetFlag( TS_ENABLE_SCALE ); // ����������
        m_camera->setObjectListener( this );
    }

    CameraNode::~CameraNode()
    {
        KHAOS_DELETE m_camera;
    }

    bool CameraNode::_checkUpdateTransform()
    {
        if ( SceneNode::_checkUpdateTransform() )
        {
            // ���ڵ�任�������ø�camera
            m_camera->setTransform( m_derivedMatrix );
            return true;
        }
        
        return false;
    }
    
    void CameraNode::_makeWorldAABB()
    {
        m_worldAABB = m_camera->getAABB();
    }

    bool CameraNode::testVisibility( const AxisAlignedBox& bound ) const
    {
        const_cast<CameraNode*>(this)->_checkUpdateTransform();
        return m_camera->testVisibility( bound );
    }

    Frustum::Visibility CameraNode::testVisibilityEx( const AxisAlignedBox& bound ) const
    {
        const_cast<CameraNode*>(this)->_checkUpdateTransform();
        return m_camera->testVisibilityEx( bound );
    }
}

