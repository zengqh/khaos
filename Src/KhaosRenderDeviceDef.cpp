#include "KhaosPreHeaders.h"
#include "KhaosRenderDeviceDef.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    DepthStateSet DepthStateSet::SOLID_DRAW( true, true, CMP_LESSEQUAL );
    DepthStateSet DepthStateSet::TRANS_DRAW( true, false, CMP_LESSEQUAL );
    DepthStateSet DepthStateSet::TEST_GE( true, false, CMP_GREATEREQUAL );
    DepthStateSet DepthStateSet::ALL_DISABLED( false, false, CMP_NEVER );

    MaterialStateSet MaterialStateSet::FRONT_DRAW( CULL_CW, false );
    MaterialStateSet MaterialStateSet::BACK_DRAW( CULL_CCW, false );
    MaterialStateSet MaterialStateSet::TWOSIDE_DRAW( CULL_NONE, false );

    BlendStateSet BlendStateSet::REPLACE(BLENDOP_ADD, BLEND_ONE, BLEND_ZERO);
    BlendStateSet BlendStateSet::ALPHA(BLENDOP_ADD, BLEND_SRCALPHA, BLEND_INVSRCALPHA);
    BlendStateSet BlendStateSet::ADD(BLENDOP_ADD, BLEND_ONE, BLEND_ONE);

    TextureFilterSet TextureFilterSet::NEAREST(TEXF_POINT, TEXF_POINT, TEXF_NONE);
    TextureFilterSet TextureFilterSet::BILINEAR(TEXF_LINEAR, TEXF_LINEAR, TEXF_POINT);
    TextureFilterSet TextureFilterSet::TRILINEAR(TEXF_LINEAR, TEXF_LINEAR, TEXF_LINEAR);
    TextureFilterSet TextureFilterSet::ANISOTROPIC(TEXF_ANISOTROPIC, TEXF_ANISOTROPIC, TEXF_LINEAR);

    TextureAddressSet TextureAddressSet::WRAP(TEXADDR_WRAP, TEXADDR_WRAP, TEXADDR_WRAP);
    TextureAddressSet TextureAddressSet::CLAMP(TEXADDR_CLAMP, TEXADDR_CLAMP, TEXADDR_CLAMP);
    TextureAddressSet TextureAddressSet::BORDER(TEXADDR_BORDER, TEXADDR_BORDER, TEXADDR_BORDER);

    //////////////////////////////////////////////////////////////////////////
}

