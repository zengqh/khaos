#include "KhaosPreHeaders.h"
#include "KhaosMaterialManager.h"
#include "KhaosMaterialFile.h"


namespace Khaos
{
    RIDObject* MaterialManager_requestRID( void* context )
    {
        return MaterialManager::_requestRID( context );
    }

    void MaterialManager_freeRID( RIDObject* rid )
    {
        MaterialManager::_freeRID( rid );
    }

    void MaterialManager_dirtyRIDFlag()
    {
        MaterialManager::_dirtyRIDFlag();
    }

    //////////////////////////////////////////////////////////////////////////
    MaterialResAutoCreator::MaterialResAutoCreator()
    {
        m_basePath = "/Material/";
    }

    bool MaterialResAutoCreator::_prepareResourceImpl( Resource* res, DataBuffer& buff )
    {
        res->_getBufferTmp() = buff;
        return true; 
    }

    bool MaterialResAutoCreator::buildResource( Resource* res ) 
    { 
        Material* mtr = static_cast<Material*>(res);
        FileSystem::DataBufferAuto buffAuto(mtr->_getBufferTmp());

        MaterialImporter mtrImp;
        mtrImp.importMaterial( buffAuto, mtr );
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    RIDObjectManager* MaterialManager::s_ridObjMgr = 0;

    void MaterialManager::shutdown() 
    {
        g_resourceManager->getGroup(KHAOS_CLASS_TYPE(Material))->clearResources(); 

        // NB:sysResMgr必须shutdown在此之前，因为s_ridObjMgr为共用
        if ( s_ridObjMgr )
        {
            KHAOS_DELETE s_ridObjMgr;
            s_ridObjMgr = 0;
        }
    }

    RIDObject* MaterialManager::_requestRID( void* context )
    {
        return _getRIDMgr()->requestRIDObj( context );
    }

    void MaterialManager::_freeRID( RIDObject* rid )
    {
        khaosAssert( s_ridObjMgr );
        if ( s_ridObjMgr )
            s_ridObjMgr->freeRIDObj( rid );
    }

    void MaterialManager::_dirtyRIDFlag()
    {
        _getRIDMgr()->setDirty();
    }

    RIDObjectManager* MaterialManager::_getRIDMgr()
    {
        if ( s_ridObjMgr )
            return s_ridObjMgr;

        s_ridObjMgr = KHAOS_NEW RIDObjectManager;
        s_ridObjMgr->setSortFunc( _compareMtr );
        return s_ridObjMgr;
    }

    void MaterialManager::update()
    {
        if ( s_ridObjMgr )
            s_ridObjMgr->update();
    }

    bool MaterialManager::_compareMtr( const RIDObject* lhs,  const RIDObject* rhs )
    {
        Material* mtrLeft  = (Material*)lhs->getOwner();
        Material* mtrRight = (Material*)rhs->getOwner();

        Material::ShaderID sidLeft  = mtrLeft->getShaderFlag(); // 先调用将更新内部shaderid
        Material::ShaderID sidRight = mtrRight->getShaderFlag();

        // base纹理比较
        BaseMapAttrib* baseLeft  = mtrLeft->getEnabledAttrib<BaseMapAttrib>();
        BaseMapAttrib* baseRight = mtrRight->getEnabledAttrib<BaseMapAttrib>();

        Texture* texBaseLeft  = baseLeft ? baseLeft->getTexture() : 0;
        Texture* texBaseRight = baseRight ? baseRight->getTexture() : 0;

        if ( texBaseLeft != texBaseRight )
            return texBaseLeft < texBaseRight;

        // 比较shader的flag
        return sidLeft < sidRight;
    }
}

