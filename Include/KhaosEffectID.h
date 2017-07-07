#pragma once
#include "KhaosBitSet.h"

namespace Khaos
{
  
    //////////////////////////////////////////////////////////////////////////
    // template id
    enum 
    {
        ET_DEF_MATERIAL = 1,
        ET_DEF_SHADOW,
        ET_DEF_REFLECT,
        ET_DEF_DEFPRE,

        ET_DEPTHCONVERT,

        ET_DEPTHBOUNDINIT,
        ET_DEPTHBOUNDNEXT,

        ET_SHADOWMASK,

        ET_SKYSIMPLECLR,

        ET_LITACC,
        ET_VOLPROBACC,
        ET_COMPOSITE,
        ET_UI,

        ET_DOWNSCALEZ,

        ET_SSAO,
        ET_SSAO_BLUR,

        ET_COMM_SCALE,
        ET_COMM_BLUR,
        ET_COMM_COPY,

        ET_HDR_INITLUMIN,
        ET_HDR_LUMINITER,
        ET_HDR_ADAPTEDLUM,
        ET_HDR_BRIGHTPASS,
        ET_HDR_FLARESPASS,
        ET_HDR_FINALPASS,

        ET_POSTAA_EDGE_DETECTION,
        ET_POSTAA_BLENDING_WEIGHT_CALC,
        ET_POSTAA_FINAL,
        ET_POSTAA_FINAL2,
        ET_POSTAA_TEMP,

        ET_SPECULAR_AA
    };

    //////////////////////////////////////////////////////////////////////////
    // EffectID
    class EffectID : public BitSet64
    {
    public:
        typedef uint32  MtrFlagType;
        
    public:

#define MAKE_EFFECT_ID_MTR_ATTRIB_(n, i) \
    static const MtrFlagType n  = KhaosBitSetFlag32(i);

#define MAKE_EFFECT_ID_MTR_ATTRIB_BIT_(i)   MAKE_EFFECT_ID_MTR_ATTRIB_(EN_BIT##i, i)

        // bit 0-7, comm state
        MAKE_EFFECT_ID_MTR_ATTRIB_(EN_VTXCLR, 0);

        // bit 8-24, attrib flag
        MAKE_EFFECT_ID_MTR_ATTRIB_(EN_SPECULAR, 8)
        MAKE_EFFECT_ID_MTR_ATTRIB_(EN_ALPHATEST, 9)
        MAKE_EFFECT_ID_MTR_ATTRIB_(EN_EMISSIVE, 10)
        MAKE_EFFECT_ID_MTR_ATTRIB_(EN_BASEMAP, 12)
        MAKE_EFFECT_ID_MTR_ATTRIB_(EN_SPECULARMAP, 13)
        MAKE_EFFECT_ID_MTR_ATTRIB_(EN_EMISSMAP, 14)
        MAKE_EFFECT_ID_MTR_ATTRIB_(EN_OPACITYMAP, 15)
        MAKE_EFFECT_ID_MTR_ATTRIB_(EN_NORMMAP, 17)
        MAKE_EFFECT_ID_MTR_ATTRIB_(EN_BAKEDAOMAP, 18)

        // bit 25-31 attrib ext flag
        //static const IDType AEX_LITMAP_MASK       = 0x3;
        //static const IDType AEX_LITMAP_BITOFFSET  = 25;
        //static const IDType AEX_LITMAP_MASKOFFSET = (AEX_LITMAP_MASK << AEX_LITMAP_BITOFFSET); // 25-26 bit 

        // bit flag alias
        MAKE_EFFECT_ID_MTR_ATTRIB_BIT_(0)
        MAKE_EFFECT_ID_MTR_ATTRIB_BIT_(1)
        MAKE_EFFECT_ID_MTR_ATTRIB_BIT_(2)
        MAKE_EFFECT_ID_MTR_ATTRIB_BIT_(3)
        MAKE_EFFECT_ID_MTR_ATTRIB_BIT_(4)
        MAKE_EFFECT_ID_MTR_ATTRIB_BIT_(5)
        MAKE_EFFECT_ID_MTR_ATTRIB_BIT_(6)
        MAKE_EFFECT_ID_MTR_ATTRIB_BIT_(7)

        // bit 32 - 55 for instance id
        static const ValueType HAS_DIR_LITS   = KhaosBitSetFlag64(32);
        static const ValueType HAS_POINT_LITS = KhaosBitSetFlag64(33);
        static const ValueType HAS_SPOT_LITS  = KhaosBitSetFlag64(34);

        static const ValueType HAS_FINIT_LITS = HAS_POINT_LITS | HAS_SPOT_LITS;
        static const ValueType HAS_CALC_LITS  = HAS_DIR_LITS | HAS_FINIT_LITS;

        static const int BITOFF_HAS_SHADOW       = 35; // 35 - 38 for shadow
        static const int BITOFF_SHADOW_TAPGROUPS = BITOFF_HAS_SHADOW; // 35 - 37 tap groups

        static const int MASK_SHADOW             = 0xF;
        static const int MASK_SHADOW_TAPGROUPS   = 0x7;

        static const ValueType HAS_SHADOW           = ValueType(MASK_SHADOW) << BITOFF_HAS_SHADOW;
        static const ValueType HAS_PSSM4            = KhaosBitSetFlag64(38); // 是否4级pssm，否则3级

        static const ValueType HAS_SPECULAR_LIGHTING = KhaosBitSetFlag64(39);

        static const ValueType HAS_DEFERRED = KhaosBitSetFlag64(40);

        static const ValueType HAS_AO = KhaosBitSetFlag64(41);

        static const ValueType HAS_SIMPLE_AMB = KhaosBitSetFlag64(42);

        static const ValueType HAS_ENVSPEC0 = KhaosBitSetFlag64(43);
        static const ValueType HAS_ENVSPEC1 = KhaosBitSetFlag64(44);

        static const ValueType HAS_ENVDIFF0 = KhaosBitSetFlag64(45);
        static const ValueType HAS_ENVDIFF1 = KhaosBitSetFlag64(46);

        static const ValueType HAS_LIGHTMAP0 = KhaosBitSetFlag64(47);
        static const ValueType HAS_LIGHTMAP1 = KhaosBitSetFlag64(48);

        // bit 56 - 63 for effect template id

        // infer bit
        static const ValueType USE_TEXCOORD0        = KhaosBitSetFlag64(0);
        static const ValueType USE_TEXCOORD1        = KhaosBitSetFlag64(1);
        static const ValueType USE_TEXCOORD01       = USE_TEXCOORD0 | USE_TEXCOORD1;
        
        static const ValueType USE_MAT_WORLD        = KhaosBitSetFlag64(2);
        static const ValueType USE_POS_WORLD        = KhaosBitSetFlag64(3);
        static const ValueType USE_NORMAL_WORLD     = KhaosBitSetFlag64(4);
        static const ValueType USE_CAMERA_POS_WORLD = KhaosBitSetFlag64(5);

        static const ValueType USE_EYEVEC           = KhaosBitSetFlag64(6);
        static const ValueType USE_UVPROJ           = KhaosBitSetFlag64(7);

        static const ValueType USE_MAP_RANDOM       = KhaosBitSetFlag64(8);
        static const ValueType USE_POISSON_DISK     = KhaosBitSetFlag64(9);
        
        static const ValueType USE_MAP_ENVLUT       = KhaosBitSetFlag64(10);
    };
}

