#pragma once
#include "KhaosTexture.h"
#include "KhaosResourceManager.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class TextureResAutoCreator : public ResAutoCreatorBase
    {
    public:
        TextureResAutoCreator();

    public:
        virtual String getLocateFileName( const String& name ) const;

    private:
        virtual bool buildResource( Resource* res );
        virtual bool _prepareResourceImpl( Resource* res, DataBuffer& buff );
    };

    //////////////////////////////////////////////////////////////////////////
    class TextureManager
    {
        KHAOS_RESMAN_COMM_IMPL(Texture, TextureResAutoCreator)
    };
}

