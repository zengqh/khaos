#pragma once
#include "KhaosPortal.h"
#include "KhaosResourceManager.h"

#if 0

namespace Khaos
{
    class PortalManager : public ResourceManager
    {
    public:
        PortalManager();
        virtual ~PortalManager();

    protected:
        virtual Resource* _createImpl();
    };

    //////////////////////////////////////////////////////////////////////////
    extern PortalManager* g_portalManager;
}

#endif

