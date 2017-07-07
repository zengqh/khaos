#include "KhaosPreHeaders.h"
#include "KhaosGeneralRender.h"
#include "KhaosEffectBuildStrategy.h"

namespace Khaos
{
    GeneralRender* g_generalRender = 0;

    GeneralRender::GeneralRender()
    {
        khaosAssert(!g_generalRender);
        g_generalRender = this;
    }

    GeneralRender::~GeneralRender()
    {
        g_generalRender = 0;
    }

    void GeneralRender::init()
    {
        _registerEffectVSPS2HS( ET_DEF_DEFPRE, "deferPre", "deferPre", 
            ("pbrUtil", "materialUtil", "vtxUtil", "fragUtil"), DeferPreBuildStrategy );

        _registerEffectVSPS2HS( ET_DEF_MATERIAL, "general", "general", 
            ("pbrUtil", "materialUtil", "shadowUtil", "vtxUtil", "fragUtil"), DefaultBuildStrategy );

        _registerEffectVSPS2HS( ET_DEF_SHADOW, "shadowPre", "shadowPre",
            ("materialUtil"), ShadowPreBuildStrategy );

        _registerEffectVSPS2H( ET_DEF_REFLECT, "general", "general",
            "fragUtil", ReflectBuildStrategy );
        
    }
}

