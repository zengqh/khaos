#include "KhaosPreHeaders.h"
#include "KhaosMeshNode.h"
#include "KhaosMeshManager.h"
#include "KhaosMaterialManager.h"
#include "KhaosRenderSystem.h"
#include "KhaosGeneralRender.h"
#include "KhaosRenderDevice.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void SubEntity::setMaterial( const String& materialName )
    {
        _bindResourceRoutine( m_material, materialName, 0 );
    }

    const AxisAlignedBox& SubEntity::getAABBWorld() const
    {
        if ( m_aabbWorldDirty )
        {
            // aabb����
            SubEntity* pThis = const_cast<SubEntity*>(this);
            pThis->m_aabbWorld = m_meshNode->getMesh()->getSubMesh(m_subIndex)->getAABB();
            pThis->m_aabbWorld.transformAffine( m_meshNode->getDerivedMatrix() );
            pThis->m_aabbWorldDirty = false;
        }

        return m_aabbWorld;
    }

    void SubEntity::_addToRenderer()
    {
        g_generalRender->addRenderable( this );
    }

    const Matrix4& SubEntity::getImmMatWorld()
    {
        // ͬ���׾���
        return m_meshNode->getDerivedMatrix();
    }

    const AxisAlignedBox& SubEntity::getImmAABBWorld()
    {
        return getAABBWorld();
    }

    Material* SubEntity::_getImmMaterial()
    {
        return getMaterial();
    }

    LightsInfo* SubEntity::getImmLightInfo()
    {
        // ÿ��sub entity���ø��׵ģ�����ϸ��
        return m_meshNode->getLightInfo();
    }

    bool SubEntity::isReceiveShadow()
    {
        return m_meshNode->isReceiveShadow();
    }

    RenderableSharedData* SubEntity::getRDSharedData()
    {
        return &m_meshNode->getRenderSharedData();
    }

    void SubEntity::render()
    {
        SubMesh* sm = m_meshNode->getMesh()->getSubMesh( m_subIndex );
        sm->draw();
    }
   
    //////////////////////////////////////////////////////////////////////////
    MeshNode::MeshNode()
    {
    }

    MeshNode::~MeshNode()
    {
        _clearSubEntity();
        if ( m_mesh )
            m_mesh->removeListener( this );
    }

    void MeshNode::_holdTempMtr( vector<MaterialPtr>::type& mtrs )
    {
        int subCnt = getSubEntityCount();

        for ( int i = 0; i < subCnt; ++i )
        {
            mtrs.push_back( getSubEntity(i)->getMaterialPtr() );
        }
    }

    void MeshNode::setMesh( const String& name )
    {
        // Ϊ�˱���ɾ��sub entityʱ��mtr_ptr�Զ�ж�غ��ּ��أ�
        // ����������ʱ����һ�¼���
        vector<MaterialPtr>::type mtrs;
        _holdTempMtr( mtrs );

        // �ɵ�meshɾ��
        _clearSubEntity();

        // ���°�
        _bindResourceRoutine( m_mesh, name, this );
    }

    void MeshNode::setMaterial( const String& materialName )
    {
        KHAOS_FOR_EACH( SubEntityList, m_subEntityList, it )
        {
            (*it)->setMaterial( materialName );
        }
    }

    void MeshNode::_clearSubEntity()
    {
        size_t cnt = m_subEntityList.size();

        for ( size_t i = 0; i < cnt; ++i )
        {
            SubEntity* se = m_subEntityList[i];
            KHAOS_DELETE se;
        }

        m_subEntityList.clear();
    }

    void MeshNode::_initSubEntity()
    {
        khaosAssert( m_subEntityList.empty() );

        // ���ݵ�ǰmesh������Ӧsubentity
        int cnt = m_mesh->getSubMeshCount();
        m_subEntityList.resize( cnt );

        for ( int i = 0; i < cnt; ++i )
        {
            SubMesh* sm = m_mesh->getSubMesh( i );
            SubEntity* se = KHAOS_NEW SubEntity;

            se->_setMeshNode( this );
            se->_setSubIndex( i );
            se->setMaterial( sm->getMaterialName() );

            m_subEntityList[i] = se;
        }
    }

    void MeshNode::onResourceInitialized( Resource* res )
    {
        onResourceLoaded(res);
    }

    void MeshNode::onResourceLoaded( Resource* res )
    {
        _initSubEntity();
        _setWorldAABBDirty();
    }

    void MeshNode::onResourceUpdate( Resource* res )
    {
        _clearSubEntity();
        _initSubEntity();
        _setWorldAABBDirty();
    }

    void MeshNode::_addToRenderer()
    {
        size_t cnt = m_subEntityList.size();
        for ( size_t i = 0; i < cnt; ++i )
        {
            SubEntity* subE = m_subEntityList[i];
            subE->_addToRenderer();
        }
    }

    void MeshNode::_makeWorldAABB()
    {
        m_worldAABB = m_mesh ? m_mesh->getAABB() : AxisAlignedBox::BOX_NULL;
        m_worldAABB.transformAffine( m_derivedMatrix ); // ��_checkUpdateTransform()���Ѿ���ɸ���
    }

    void MeshNode::_setWorldAABBDirty()
    {
        SceneNode::_setWorldAABBDirty();

        // ��֪ÿ��sub entity��aabbҲͬʱ��
        size_t cnt = m_subEntityList.size();
        for ( size_t i = 0; i < cnt; ++i )
        {
            SubEntity* subE = m_subEntityList[i];
            subE->_setAABBWorldDirty();
        }
    }
}

