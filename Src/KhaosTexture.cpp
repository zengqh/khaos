#include "KhaosPreHeaders.h"
#include "KhaosTexture.h"
#include "KhaosRenderDevice.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    Texture::Texture()
    {   
    }

    Texture::~Texture()
    {
        _destructResImpl();
    }

    void Texture::_destructResImpl()
    {
        freeTextureObj();
    }
}

