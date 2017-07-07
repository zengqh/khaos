#include "KhaosPreHeaders.h"
#include "KhaosPortalManager.h"

#if 0
namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    PortalManager* g_portalManager = 0;

    //////////////////////////////////////////////////////////////////////////
    PortalManager::PortalManager()
    {
        khaosAssert( !g_portalManager );
        g_portalManager = this;
    }

    PortalManager::~PortalManager()
    {
        g_portalManager = 0;
    }

    Resource* PortalManager::_createImpl()
    {
        return KHAOS_NEW Portal;
    }
}

#endif

