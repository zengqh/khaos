#include "KhaosPreHeaders.h"
#include "KhaosTexCfgParser.h"
#include "KhaosStrConv.h"
#include "KhaosTextureManager.h"
#include <tinyxml/tinyxml.h>

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    bool TexCfgParser::parse( const void* data, int dataLen, Texture* tex )
    {
        //String buff( (const char*)data, dataLen ); // for 0 end
        TiXmlDocument doc;
        if ( !doc.Parse( (const char*)data/*buff.c_str()*/ ) )
            return false;

        // texture section
        TiXmlElement* eleTex = doc.FirstChildElement("Texture");
        if ( !eleTex )
            return false;

        // parse items
        m_tex = tex;
        m_texType = TEXTYPE_2D;
        m_texUsage = TEXUSA_STATIC;

        tex->resetDefaultSamplerState(); // reset sample state

        if ( !_parseImage( eleTex ) )
            return false;

        if ( !_parseFilter( eleTex ) )
            return false;

        if ( !_parseAddress( eleTex ) )
            return false;

        if ( !_parseBorder( eleTex ) )
            return false;

        if ( !_parseMip( eleTex ) )
            return false;

        if ( !_parseAnisotropy( eleTex ) )
            return false;

        if ( !_parseSRGB( eleTex ) )
            return false;

        return true;
    }

    bool TexCfgParser::_parseImage( TiXmlElement* eleTex )
    {
        TiXmlElement* eleImg = eleTex->FirstChildElement("Image");
        if ( !eleImg )
            return false;

        const char* type = eleImg->Attribute("type");
        if ( !type )
            return false;

        m_texType = strToTextureType( type );

        if ( const char* usage = eleImg->Attribute("usage") )
        {
            m_texUsage = strToTextureUsage( usage );
        }

        return true;
    }

    bool TexCfgParser::_parseFilter( TiXmlElement* eleTex )
    {
        TiXmlElement* eleFilter = eleTex->FirstChildElement("Filter");
        if ( !eleFilter )
            return true;

        if ( const char* flt = eleFilter->Attribute("mag") )
            m_tex->setFilterMag( strToTextureFilter(flt) );
        
        if ( const char* flt = eleFilter->Attribute("min") )
            m_tex->setFilterMin( strToTextureFilter(flt) );

        if ( const char* flt = eleFilter->Attribute("mip") )
            m_tex->setFilterMip( strToTextureFilter(flt) );

        m_mipSize = TexObjLoadParas::MIP_AUTO;

        if ( const char* flt = eleFilter->Attribute("mipSize") )
        {
            if ( strcmp(flt, "FILE") == 0 )
                m_mipSize = TexObjLoadParas::MIP_FROMFILE;
            else if ( strcmp(flt, "AUTO") == 0 )
                m_mipSize = TexObjLoadParas::MIP_AUTO;
            else
                m_mipSize = stringToInt( flt );
        }

        return true;
    }

    bool TexCfgParser::_parseAddress( TiXmlElement* eleTex )
    {
        TiXmlElement* eleAddr = eleTex->FirstChildElement("Address");
        if ( !eleAddr )
            return true;

        if ( const char* addr = eleAddr->Attribute("addrU") )
            m_tex->setAddressU( strToTextureAddress(addr) );

        if ( const char* addr = eleAddr->Attribute("addrV") )
            m_tex->setAddressV( strToTextureAddress(addr) );

        if ( const char* addr = eleAddr->Attribute("addrW") )
            m_tex->setAddressW( strToTextureAddress(addr) );

        return true;
    }

    bool TexCfgParser::_parseBorder( TiXmlElement* eleTex )
    {
        TiXmlElement* eleBorder = eleTex->FirstChildElement("Border");
        if ( !eleBorder )
            return true;

        if ( const char* clr = eleBorder->Attribute("color") )
            m_tex->setBorderColor( strToColor(clr) );

        return true;
    }

    bool TexCfgParser::_parseMip( TiXmlElement* eleTex )
    {
        TiXmlElement* eleMip = eleTex->FirstChildElement("Mip");
        if ( !eleMip )
            return true;

        if ( const char* bias = eleMip->Attribute("lodBias") )
            m_tex->setMipLodBias( stringToFloat(bias) );

        if ( const char* lv = eleMip->Attribute("maxLevel") )
            m_tex->setMipMaxLevel( stringToInt(lv) );

        return true;
    }

    bool TexCfgParser::_parseAnisotropy( TiXmlElement* eleTex )
    {
        TiXmlElement* eleAni = eleTex->FirstChildElement("Anisotropy");
        if ( !eleAni )
            return true;

        if ( const char* str = eleAni->Attribute("max") )
            m_tex->setMaxAnisotropy( stringToInt(str) );

        return true;
    }

    bool TexCfgParser::_parseSRGB( TiXmlElement* eleTex )
    {
        TiXmlElement* ele = eleTex->FirstChildElement("SRGB");
        if ( !ele )
            return true;

        if ( const char* str = ele->Attribute("enabled") )
            m_tex->setSRGB( stringToBool(str) );

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool TexCfgSaver::save( Texture* tex, const String& fileName, int mipSize )
    {
        TiXmlDocument doc;
        m_root = new TiXmlElement("Texture");
        doc.LinkEndChild( m_root );

        m_tex = tex;

        _saveImage();
        _saveFilter(mipSize);
        _saveAddress();
        _saveBorder();
        _saveMip();
        _saveAnisotropy();
        _saveSRGB();

        return doc.SaveFile( fileName.c_str() );
    }

    void TexCfgSaver::_saveImage()
    {
        TiXmlElement* eleImg = new TiXmlElement("Image");
        m_root->LinkEndChild( eleImg );

        eleImg->SetAttribute( "type", TextureTypeToStr(m_tex->getType()) );
        eleImg->SetAttribute( "usage", TextureUsageToStr(m_tex->getUsage()) );
    }

    void TexCfgSaver::_saveFilter( int mipSize )
    {
        TiXmlElement* eleFilter = new TiXmlElement("Filter");
        m_root->LinkEndChild( eleFilter );

        TextureFilterSet tfs = m_tex->getFilter();
        eleFilter->SetAttribute( "mag", TextureFilterToStr( (TextureFilter)tfs.tfMag ) );
        eleFilter->SetAttribute( "min", TextureFilterToStr( (TextureFilter)tfs.tfMin ) );
        eleFilter->SetAttribute( "mip", TextureFilterToStr( (TextureFilter)tfs.tfMip ) );

        if ( mipSize == TexObjLoadParas::MIP_FROMFILE )
        {
            eleFilter->SetAttribute( "mipSize", "FILE" );
        }
        else if ( mipSize == TexObjLoadParas::MIP_AUTO )
        {
            // default
        }
        else
        {
            eleFilter->SetAttribute( "mipSize", intToString(mipSize).c_str() );
        }
    }

    void TexCfgSaver::_saveAddress()
    {
        TiXmlElement* eleAddr = new TiXmlElement("Address");
        m_root->LinkEndChild( eleAddr );

        TextureAddressSet addr = m_tex->getAddress();
        eleAddr->SetAttribute( "addrU", TextureAddressToStr( (TextureAddress)addr.addrU ) );
        eleAddr->SetAttribute( "addrV", TextureAddressToStr( (TextureAddress)addr.addrV ) );
        eleAddr->SetAttribute( "addrW", TextureAddressToStr( (TextureAddress)addr.addrW ) );
    }

    void TexCfgSaver::_saveBorder()
    {
        TiXmlElement* eleBorder = new TiXmlElement("Border");
        m_root->LinkEndChild( eleBorder );

        eleBorder->SetAttribute( "color", colorToStr( m_tex->getBorderColor() ).c_str() );
    }

    void TexCfgSaver::_saveMip()
    {
        TiXmlElement* eleMip = new TiXmlElement("Mip");
        m_root->LinkEndChild( eleMip );

        eleMip->SetAttribute( "lodBias", floatToString(m_tex->getMipLodBias()).c_str() );
        eleMip->SetAttribute( "maxLevel", intToString(m_tex->getMipMaxLevel()).c_str() );
    }

    void TexCfgSaver::_saveAnisotropy()
    {
        TiXmlElement* eleAni = new TiXmlElement("Anisotropy");
        m_root->LinkEndChild( eleAni );

        eleAni->SetAttribute( "max", intToString(m_tex->getMaxAnisotropy()).c_str() );
    }

    void TexCfgSaver::_saveSRGB()
    {
        TiXmlElement* ele = new TiXmlElement("SRGB");
        m_root->LinkEndChild( ele );
        ele->SetAttribute( "enabled", boolToString(m_tex->isSRGB()).c_str() );
    }

    bool TexCfgSaver::saveSimple( const String& resName, TextureType type, PixelFormat format, 
        TextureAddress addr, TextureFilter minmag, TextureFilter mipmap, bool srgb, int mipSize,
        const Color* borderClr )
    {
        TexturePtr texTmp( TextureManager::createTexture(resName) );
        if ( !texTmp )
        {
            // 已经有了
            TexturePtr texCur( TextureManager::getTexture(resName) );
            if ( texCur->getType() == type /*&& texCur->getFormat() == format*/ )
            {
                texCur->setFilterMin( minmag );
                texCur->setFilterMag( minmag );
                texCur->setFilterMip( mipmap );

                texCur->setAddressU( addr );
                texCur->setAddressV( addr );
                texCur->setAddressW( addr );

                texCur->setSRGB( srgb );

                if ( borderClr )
                    texCur->setBorderColor( *borderClr );

                TexCfgSaver saver;
                return saver.save( texCur.get(), texCur->getResFullFileName(), mipSize );
            }

            // 不匹配
            return false;
        }

        texTmp->setNullCreator();

        TexObjCreateParas paras;
        
        paras.type   = type;
        paras.format = format;
        paras.width  = 1; // 随便设，宽和高不会记录在配置文件内
        paras.height = 1;
        paras.depth  = type != TEXTYPE_VOLUME ? 0 : 1;

        texTmp->createTex( paras );

        texTmp->setFilterMin( minmag );
        texTmp->setFilterMag( minmag );
        texTmp->setFilterMip( mipmap );

        texTmp->setAddressU( addr );
        texTmp->setAddressV( addr );
        texTmp->setAddressW( addr );

        texTmp->setSRGB( srgb );

        if ( borderClr )
            texTmp->setBorderColor( * borderClr );

        TexCfgSaver saver;
        return saver.save( texTmp.get(), texTmp->getResFullFileName(), mipSize );
    }
}

