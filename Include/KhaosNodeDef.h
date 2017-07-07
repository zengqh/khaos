#pragma once
#include "KhaosBitSet.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    // Dirty Flags
    // Node
    static const uint32 DF_SELF_TRANSFORM   = KhaosBitSetFlag32(0);
    static const uint32 DF_PARENT_TRANSFORM = KhaosBitSetFlag32(1);

    // SceneNode
    static const uint32 DF_WORLD_AABB       = KhaosBitSetFlag32(2);
    static const uint32 DF_OCTREE_POS       = KhaosBitSetFlag32(3);


    //////////////////////////////////////////////////////////////////////////
    // Node Feature Flags
    static const uint32 TS_ENABLE_POS    = KhaosBitSetFlag32(0);
    static const uint32 TS_ENABLE_ROT    = KhaosBitSetFlag32(1);
    static const uint32 TS_ENABLE_SCALE  = KhaosBitSetFlag32(2);

    static const uint32 SNODE_ENABLE     = KhaosBitSetFlag32(3);
    static const uint32 SHDW_ENABLE_CAST = KhaosBitSetFlag32(4);
    static const uint32 SHDW_ENABLE_RECE = KhaosBitSetFlag32(5);


    //////////////////////////////////////////////////////////////////////////
    // Node Type
    enum NodeType
    {
        NT_NODE,
        NT_SCENE,
        NT_AREA,
        NT_RENDER,
        NT_LIGHT,
        NT_CAMERA,
        NT_ENVPROBE,
        NT_VOLPROBE,
        NT_MAX
    };
}

