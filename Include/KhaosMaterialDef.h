#pragma once
#include "KhaosBitSet.h"


namespace Khaos
{
    enum MaterialPass
    {
        MP_MATERIAL,
        MP_SHADOW,
        MP_REFLECT,
        MP_DEFPRE,
        MP_MAX,
        MP_LITACC,
        MP_ETC
    };

    struct MtrCommState
    {
        typedef uint8 StateID;

        static const StateID VTXCLR = KhaosBitSetFlag8(0);

        // flag alias
#define KHAOS_MTR_COMM_STATE_BIT_DEFINE_(i) \
        static const StateID BIT##i = KhaosBitSetFlag8(i);

        KHAOS_MTR_COMM_STATE_BIT_DEFINE_(0)
        KHAOS_MTR_COMM_STATE_BIT_DEFINE_(1)
        KHAOS_MTR_COMM_STATE_BIT_DEFINE_(2)
        KHAOS_MTR_COMM_STATE_BIT_DEFINE_(3)
        KHAOS_MTR_COMM_STATE_BIT_DEFINE_(4)
        KHAOS_MTR_COMM_STATE_BIT_DEFINE_(5)
        KHAOS_MTR_COMM_STATE_BIT_DEFINE_(6)
        KHAOS_MTR_COMM_STATE_BIT_DEFINE_(7)
    };
}

