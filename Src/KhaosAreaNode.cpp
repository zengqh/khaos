#include "KhaosPreHeaders.h"
#include "KhaosAreaNode.h"
#include "KhaosAreaOctree.h"


namespace Khaos
{
    AreaNode::AreaNode() : m_areaOctree(KHAOS_NEW AreaOctree)
    {
        m_type = NT_AREA;
        m_areaOctree->setAreaRoot( this );
        m_featureFlag.unsetFlag( TS_ENABLE_POS | TS_ENABLE_ROT | TS_ENABLE_SCALE ); // ²»ÔÊÐí±ä»»
    }

    AreaNode::~AreaNode()
    {
        KHAOS_DELETE m_areaOctree;
    }

    void AreaNode::_makeWorldAABB()
    {
        m_worldAABB = m_areaOctree->getCullBoundBox();
    }
}

