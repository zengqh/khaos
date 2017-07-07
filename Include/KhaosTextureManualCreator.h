#include "KhaosTextureManager.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class TextureColorCreatorImpl : public AllocatedObject, public IResourceCreator
    {
    public:
        TextureColorCreatorImpl( const Color& clr ) : m_color(clr.getAsARGB()) {}

        virtual void release()
        {
            KHAOS_DELETE this;
        }

        virtual bool prepareResource( Resource* res, void* context )
        {
            return true;
        }

        virtual bool buildResource( Resource* res )
        {
            const int texSize = 1;

            Texture* tex = static_cast<Texture*>(res);

            TexObjCreateParas paras;
            paras.type      = TEXTYPE_2D;
            paras.usage     = TEXUSA_STATIC;
            paras.format    = PIXFMT_A8R8G8B8;
            paras.width     = texSize;
            paras.height    = texSize;
            paras.levels    = 1;

            if ( !tex->createTex( paras ) )
                return false;

            IntRect rect( 0, 0, texSize, texSize );
            vector<uint32>::type clr( texSize*texSize, m_color );

            tex->fillData( 0, &rect, &clr[0], 0 );
            return true;
        }

    private:
        uint32 m_color;
    };

}

