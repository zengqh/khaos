#pragma once
#include "KhaosSceneNode.h"
#include "KhaosAreaOctree.h"


namespace Khaos
{
    class AreaNode : public SceneNode
    {
        KHAOS_DECLARE_RTTI(AreaNode)

    public:
        AreaNode();
        virtual ~AreaNode();

    public:
        AreaOctree* getArea() const { return m_areaOctree; }

    private:
        virtual AreaOctree* _getPrivateArea() const { return m_areaOctree; }

    private:
        virtual void _makeWorldAABB();

    private:
        AreaOctree* m_areaOctree;
    };
}

