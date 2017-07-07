#pragma once
#include "KhaosMaterial.h"
#include "KhaosResourceManager.h"
#include "KhaosRIDMgr.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class MaterialResAutoCreator : public ResAutoCreatorBase
    {
    public:
        MaterialResAutoCreator();

    private:
        virtual bool buildResource( Resource* res );
        virtual bool _prepareResourceImpl( Resource* res, DataBuffer& buff );
    };

    //////////////////////////////////////////////////////////////////////////
    class MaterialManager
    {
        KHAOS_RESMAN_COMM_INIT(Material, MaterialResAutoCreator)
        KHAOS_RESMAN_COMM_OVERRIDE(Material)

    public:
        static void update();
        static void shutdown();

    public:
        static RIDObject* _requestRID( void* context );
        static void       _freeRID( RIDObject* rid );
        static void       _dirtyRIDFlag();

        static RIDObjectManager* _getRIDMgr();

    private:
        static bool _compareMtr( const RIDObject* lhs,  const RIDObject* rhs );

    private:
        static RIDObjectManager* s_ridObjMgr;
    };

    RIDObject* MaterialManager_requestRID( void* context );
    void MaterialManager_freeRID( RIDObject* rid );
    void MaterialManager_dirtyRIDFlag();
}

