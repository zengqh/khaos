#pragma once
#include "KhaosResource.h"
#include "KhaosTextureObj.h"

namespace Khaos
{
    class Texture : public Resource, public TextureObjUnit
    {
        KHAOS_DECLARE_RTTI(Texture)

    public:
        Texture();
        virtual ~Texture();

    public:
        virtual void _destructResImpl();
    };

    typedef ResPtr<Texture> TexturePtr;
}

