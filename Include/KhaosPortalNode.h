#pragma once
#include "KhaosSceneNode.h"
#include "KhaosPortal.h"

#if 0

namespace Khaos
{
    class PortalNode : public SceneNode
    {
        KHAOS_DECLARE_RTTI(PortalNode)

    public:
        PortalNode();
        virtual ~PortalNode();

    public:
        void setPortal( const String& name );
        PortalPtr getPortal() const { return m_portal; }

    protected:
        PortalPtr m_portal;
    };
}

#endif

