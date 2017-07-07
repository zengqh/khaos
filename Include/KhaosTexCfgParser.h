#pragma once
#include "KhaosTexture.h"

class TiXmlElement;

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class TexCfgParser : public AllocatedObject
    {
    public:
        TexCfgParser() : m_tex(0), m_texType(TEXTYPE_2D), m_texUsage(TEXUSA_STATIC), m_mipSize(0) {}

        bool parse( const void* data, int dataLen, Texture* tex );

        TextureType  getTextureType() const  { return m_texType; }
        TextureUsage getTextureUsage() const { return m_texUsage; }
        int          getMipSize() const { return m_mipSize; }

    private:
        bool _parseImage( TiXmlElement* eleTex );
        bool _parseFilter( TiXmlElement* eleTex );
        bool _parseAddress( TiXmlElement* eleTex );
        bool _parseBorder( TiXmlElement* eleTex );
        bool _parseMip( TiXmlElement* eleTex );
        bool _parseAnisotropy( TiXmlElement* eleTex );
        bool _parseSRGB( TiXmlElement* eleTex );

    private:
        Texture*     m_tex;
        TextureType  m_texType;
        TextureUsage m_texUsage;
        int          m_mipSize;
    };

    //////////////////////////////////////////////////////////////////////////
    class TexCfgSaver : public AllocatedObject
    {
    public:
        TexCfgSaver() : m_root(0), m_tex(0) {}

        bool save( Texture* tex, const String& fileName, int mipSize );

        static bool saveSimple( const String& resName, TextureType type, PixelFormat format,
            TextureAddress addr, TextureFilter minmag, TextureFilter mipmap, bool srgb, int mipSize,
            const Color* borderClr = 0 );

    private:
        void _saveImage();
        void _saveFilter( int mipSize );
        void _saveAddress();
        void _saveBorder();
        void _saveMip();
        void _saveAnisotropy();
        void _saveSRGB();

    private:
        TiXmlElement* m_root;
        Texture*      m_tex;
    };
}

