#include "KhaosPreHeaders.h"
#include "KhaosMtrAttrib.h"
#include "KhaosBinStream.h"
#include "KhaosTextureManager.h"
#include "KhaosNameDef.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void MtrAttrib::_needUpdateAttr()
    {
        m_owner->_notifyNeedUpdateAttr( this );
    }

    //////////////////////////////////////////////////////////////////////////
    MtrAttribMap::~MtrAttribMap()
    {
        clear();
    }

    void MtrAttribMap::_enableFlag( MtrAttribType type, bool en )
    {
        m_flag.enableBit( type, en );
    }

    bool MtrAttribMap::_isFlagEnabled( MtrAttribType type ) const
    {
        return m_flag.testBit( type );
    }

    MtrAttrib* MtrAttribMap::createAttrib( MtrAttribType type )
    {
        if ( m_attrs.find(type) == m_attrs.end() )
        {
            MtrAttrib* att = g_mtrAttribFactory->createAttrib(this, type);
            m_attrs[type] = att;
            _enableFlag( type, true );
            _notifyNeedUpdateAttr( att );
            return att;
        }
        
        khaosLogLn( KHL_L2, "MtrAttribMap::createAttrib %d", type );
        return 0;
    }

    MtrAttrib* MtrAttribMap::useAttrib( MtrAttribType type )
    {
        AttribMap::iterator it = m_attrs.find(type);
        if ( it != m_attrs.end() )
            return it->second;

        MtrAttrib* att = g_mtrAttribFactory->createAttrib(this, type);
        m_attrs[type] = att;
        _enableFlag( type, true );
        _notifyNeedUpdateAttr( att );
        return att;
    }

    void MtrAttribMap::removeAttrib( MtrAttribType type )
    {
        AttribMap::iterator it = m_attrs.find( type );
        if ( it != m_attrs.end() )
        {
            MtrAttrib* att = it->second;
            _enableFlag( type, false );
            KHAOS_DELETE att;
            m_attrs.erase(it);
            _notifyNeedUpdateAttr( 0 );
        }
    }

    MtrAttrib* MtrAttribMap::getAttrib( MtrAttribType type ) const
    {
        AttribMap::const_iterator it = m_attrs.find( type );
        if ( it != m_attrs.end() )
            return it->second;
        return 0;
    }

    MtrAttrib* MtrAttribMap::getEnabledAttrib( MtrAttribType type ) const
    {
        if ( isAttribEnabled(type) )
            return getAttrib( type );
        return 0;
    }

    void MtrAttribMap::clear()
    {
        KHAOS_FOR_EACH( AttribMap, m_attrs, it )
        {
            MtrAttrib* att = it->second;
            KHAOS_DELETE att;
        }

        m_attrs.clear();
        m_flag.clear();

        _notifyNeedUpdateAttr(0);
    }

    void MtrAttribMap::setAttribsFlag( const FlagType& flag )
    {
        m_flag = flag;

        // check!!
#if KHAOS_DEBUG
        for ( int i = 0; i < FlagType::BitCount; ++i )
        {
            if ( _isFlagEnabled( (MtrAttribType)i ) )
            {
                khaosAssert( getAttrib( i ) );
            }
        }
#endif

        _notifyNeedUpdateAttr(0);
    }

    void MtrAttribMap::enableAttrib( MtrAttribType type, bool en )
    {
        if ( MtrAttrib* att = getAttrib(type) )
        {
            _enableFlag( type, en );
            _notifyNeedUpdateAttr( att );
        }
    }

    bool MtrAttribMap::isAttribEnabled( MtrAttribType type ) const
    {
        return _isFlagEnabled( type );
    }

    void MtrAttribMap::copyFrom( const MtrAttribMap* rhs )
    {
        clear();

        KHAOS_FOR_EACH_CONST( AttribMap, rhs->m_attrs, it )
        {
            MtrAttribType attType = it->first;
            MtrAttrib* attObj = it->second;

            MtrAttrib* newAtt = createAttrib( attType );
            newAtt->copyFrom( attObj );
        }

        setAttribsFlag( rhs->m_flag ); // ±£ÁôÔ­Ê¼flag×´Ì¬
    }

    //////////////////////////////////////////////////////////////////////////
    void ColorAttribBase::_importAttrib( BinStreamReader& reader )
    {
        Color v;
        reader.read( v );
        setValue( v );
    }

    void ColorAttribBase::_exportAttrib( BinStreamWriter& writer )
    {
        writer.write( m_value );
    }

    //////////////////////////////////////////////////////////////////////////
    void IntensityAttribBase::_importAttrib( BinStreamReader& reader )
    {
        float v = 0;
        reader.read( v );
        setValue( v );
    }

    void IntensityAttribBase::_exportAttrib( BinStreamWriter& writer )
    {
        writer.write( m_value );
    }

    //////////////////////////////////////////////////////////////////////////
    TextureAttribBase::~TextureAttribBase()
    {
        if ( m_texture )
            m_texture->removeListener( this );
    }

    void TextureAttribBase::onResourceLoaded( Resource* res )
    {
        _needUpdateAttr();
    }

    void TextureAttribBase::setTexture( const String& texName )
    {
        _bindResourceRoutine( m_texture, texName, this );
        _needUpdateAttr();
    }

    void TextureAttribBase::setTexture( Texture* tex )
    {
        _bindResourceRoutine( m_texture, tex, this );
        _needUpdateAttr();
    }

    const String& TextureAttribBase::getTextureName() const 
    {
        return m_texture.get() ? m_texture->getName() : STRING_EMPTY; 
    }

    bool TextureAttribBase::isTexturePrepared() const
    {
        if ( Texture* tex = m_texture.get() )
        {
            return tex->isLoaded();
        }

        return false;
    }

    void TextureAttribBase::_importAttrib( BinStreamReader& reader )
    {
        String name;
        reader.readString( name );

        if ( name.size() )
            setTexture( name );
    }

    void TextureAttribBase::_exportAttrib( BinStreamWriter& writer )
    {
        if ( m_texture )
            writer.writeString( m_texture->getName() );
        else
            writer.writeString( STRING_EMPTY );
    }

    void TextureAttribBase::copyFrom( const MtrAttrib* rhs ) 
    {
        const TextureAttribBase* attOth = static_cast<const TextureAttribBase*>(rhs);
        this->m_texture = attOth->m_texture;
    }

    //////////////////////////////////////////////////////////////////////////
    void SpecularMapAttrib::_importAttrib( BinStreamReader& reader )
    {
        TextureAttribBase::_importAttrib( reader );

        int m, d, r;

        reader.read(m);
        reader.read(d);
        reader.read(r);

        setMetallicEnabled( m != 0 );
        setDSpecularEnabled( d != 0 );
        setRoughnessEnabled( r != 0 );
    }

    void SpecularMapAttrib::_exportAttrib( BinStreamWriter& writer )
    {
        TextureAttribBase::_exportAttrib( writer );

        writer.write( (int)m_metallicEnabled );
        writer.write( (int)m_dspecularEnabled );
        writer.write( (int)m_roughnessEnabled );
    }

    void SpecularMapAttrib::copyFrom( const MtrAttrib* rhs )
    {
        TextureAttribBase::copyFrom( rhs );

        const SpecularMapAttrib* attOth = static_cast<const SpecularMapAttrib*>(rhs);
        setMetallicEnabled( attOth->isMetallicEnabled() );
        setDSpecularEnabled( attOth->isDSpecularEnabled() );
        setRoughnessEnabled( attOth->isRoughnessEnabled() );
    }

    void SpecularMapAttrib::setMetallicEnabled( bool en )
    {
        m_metallicEnabled = en;
    }

    void SpecularMapAttrib::setDSpecularEnabled( bool en )
    {
        m_dspecularEnabled = en;
    }

    void SpecularMapAttrib::setRoughnessEnabled( bool en )
    {
        m_roughnessEnabled = en;
    }

    //////////////////////////////////////////////////////////////////////////
    MtrAttribFactory* g_mtrAttribFactory = 0;

    MtrAttribFactory::MtrAttribFactory()
    {
        khaosAssert(!g_mtrAttribFactory);
        g_mtrAttribFactory = this;
        _init();
    }

    MtrAttribFactory::~MtrAttribFactory()
    {
        g_mtrAttribFactory = 0;
    }

    void MtrAttribFactory::registerCreator( MtrAttribType type, CreateFunc func )
    {
        khaosAssert( func );

        if ( !m_creators.insert( CreatorMap::value_type(type, func) ).second )
        {
            khaosLogLn( KHL_L2, "MtrAttribFactory::registerCreator %d", type );
        }
    }

    MtrAttrib* MtrAttribFactory::createAttrib( MtrAttribMap* owner, MtrAttribType type )
    {
        CreatorMap::iterator it = m_creators.find(type);
        if ( it != m_creators.end() )
        {
            CreateFunc func = it->second;
            MtrAttrib* attr = func();
            attr->_setOwner( owner );
            return attr;
        }

        khaosLogLn( KHL_L2, "MtrAttribFactory::createAttrib %d", type );
        return 0;
    }

    void MtrAttribFactory::_init()
    {
        registerCreator<BaseColorAttrib>();
        registerCreator<MetallicAttrib>();
        registerCreator<DSpecularAttrib>();
        registerCreator<RoughnessAttrib>();

        registerCreator<EmissiveAttrib>();
        registerCreator<AlphaAttrib>();
        registerCreator<AlphaTestAttrib>();

        registerCreator<BaseMapAttrib>();
        registerCreator<SpecularMapAttrib>();

        registerCreator<NormalMapAttrib>();
        registerCreator<OpacityMapAttrib>();

        registerCreator<BakedAOMapAttrib>();
    }
}

