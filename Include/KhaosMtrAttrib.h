#pragma once
#include "KhaosBitSet.h"
#include "KhaosIterator.h"
#include "KhaosColor.h"
#include "KhaosTexture.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class BinStreamReader;
    class BinStreamWriter;
    class MtrAttribMap;

    typedef uint MtrAttribType;

    enum  // ≤ƒ÷ ¿‡–Õid
    {
        MA_BASECOLOR,
        MA_METALLIC,
        MA_DSPECULAR,
        MA_ROUGHNESS,
        MA_ALPHA,
        MA_ALPHATEST,
        MA_EMISSIVE,

        MA_BASEMAP,
        MA_SPECULARMAP,
        MA_EMISSIVEMAP,
        MA_NORMALMAP,
        MA_OPACITYMAP,
        MA_BAKEDAOMAP
    };

    class MtrAttrib : public AllocatedObject
    {
    public:
        MtrAttrib( MtrAttribType type ) : m_owner(0), m_type(type) {}
        virtual ~MtrAttrib() {}

    public:
        void _setOwner( MtrAttribMap* owner ) { m_owner = owner; }

        MtrAttribType getType() const { return m_type; }

        virtual void _importAttrib( BinStreamReader& reader ) = 0;
        virtual void _exportAttrib( BinStreamWriter& writer ) = 0;

        virtual void copyFrom( const MtrAttrib* rhs ) = 0;

    protected:
        void _needUpdateAttr();

    protected:
        MtrAttribMap*   m_owner;
        MtrAttribType   m_type;
    };

    //////////////////////////////////////////////////////////////////////////
    class MtrAttribMap : public AllocatedObject
    {
    public:
        typedef BitSet<uint32> FlagType;
        typedef map<MtrAttribType, MtrAttrib*>::type AttribMap;
        typedef RangeIterator<AttribMap> Iterator;

    public:
        virtual ~MtrAttribMap();

    public:
        // attribs
        MtrAttrib* createAttrib( MtrAttribType type );

        template<class T>
        T* createAttrib() { return static_cast<T*>(createAttrib(T::ID)); }

        void removeAttrib( MtrAttribType type );

        void clear();

        MtrAttrib* useAttrib( MtrAttribType type );
        MtrAttrib* getAttrib( MtrAttribType type ) const;
        MtrAttrib* getEnabledAttrib( MtrAttribType type ) const;

        template<class T>
        T* useAttrib() { return static_cast<T*>(useAttrib(T::ID)); }

        template<class T>
        T* getAttrib() const { return static_cast<T*>(getAttrib(T::ID)); }

        template<class T>
        T* getEnabledAttrib() const { return static_cast<T*>(getEnabledAttrib(T::ID)); }

        Iterator getAllAttribs() const { return Iterator(m_attrs); }

        int getAttribSize() const { return (int)m_attrs.size(); }

    public:
        // flag
        void setAttribsFlag( const FlagType& flag );
        const FlagType& getAttribsFlag() const { return m_flag; }

        void enableAttrib( MtrAttribType type, bool en );
        bool isAttribEnabled( MtrAttribType type ) const;

    public:
        void copyFrom( const MtrAttribMap* rhs );

    protected:
        void _enableFlag( MtrAttribType type, bool en );
        bool _isFlagEnabled( MtrAttribType type ) const;

    public:
        virtual void _notifyNeedUpdateAttr( MtrAttrib* attr ) {};

    private:
        FlagType    m_flag;
        AttribMap   m_attrs;
    };

    //////////////////////////////////////////////////////////////////////////
    class MtrAttribFactory : public AllocatedObject
    {
    private:
        template<class T>
        static MtrAttrib* _createProxy() { return KHAOS_NEW T; }

    public:
        typedef MtrAttrib* (*CreateFunc)();
        typedef unordered_map<MtrAttribType, CreateFunc>::type CreatorMap;

    public:
        MtrAttribFactory();
        ~MtrAttribFactory();

    public:
        void registerCreator( MtrAttribType type, CreateFunc func );

        template<class T>
        void registerCreator()
        {
            registerCreator( T::ID, _createProxy<T> );
        }

        MtrAttrib* createAttrib( MtrAttribMap* owner, MtrAttribType type );

        template<class T>
        T* createAttrib( MtrAttribMap* owner )
        {
            return static_cast<T*>(createAttrib(owner, T::ID)); 
        }

    private:
        void _init();

    private:
        CreatorMap m_creators;
    };

    extern MtrAttribFactory* g_mtrAttribFactory;

    //////////////////////////////////////////////////////////////////////////
    // ColorAttribBase
    class ColorAttribBase : public MtrAttrib
    {
    public:
        typedef Color ValueType;

    public:
        ColorAttribBase( MtrAttribType type ) : MtrAttrib(type) {}

        virtual void setValue( const Color& v ) = 0;
        const Color& getValue() const { return m_value; }

        virtual void _importAttrib( BinStreamReader& reader );
        virtual void _exportAttrib( BinStreamWriter& writer );

        virtual void copyFrom( const MtrAttrib* rhs ) 
        {
            const ColorAttribBase* attOth = static_cast<const ColorAttribBase*>(rhs);
            this->m_value = attOth->m_value;
        }

    protected:
        Color m_value;
    };

    class RGBAColorAttribBase : public ColorAttribBase
    {
    public:
        RGBAColorAttribBase( MtrAttribType type ) : ColorAttribBase(type) {}
        RGBAColorAttribBase( MtrAttribType type, const Color& defVal ) : ColorAttribBase(type) { setValue(defVal); }
        void setValue( const Color& v ) { m_value = v; }
    };

    class RGBColorAttribBase : public ColorAttribBase
    {
    public:
        RGBColorAttribBase( MtrAttribType type ) : ColorAttribBase(type) {}
        RGBColorAttribBase( MtrAttribType type, const Color& defVal ) : ColorAttribBase(type) { setValue(defVal); }
        void setValue( const Color& v ) { m_value.setRGB(v); }
    };

    //////////////////////////////////////////////////////////////////////////
    // IntensityAttribBase
    class IntensityAttribBase : public MtrAttrib
    {
    public:
        typedef float ValueType;

    public:
        IntensityAttribBase( MtrAttribType type ) : MtrAttrib(type) {}
        IntensityAttribBase( MtrAttribType type, float defVal ) : MtrAttrib(type) { setValue(defVal); }

        void setValue( float v ) { m_value = v; }
        float getValue() const { return m_value; }

        virtual void _importAttrib( BinStreamReader& reader );
        virtual void _exportAttrib( BinStreamWriter& writer );

        virtual void copyFrom( const MtrAttrib* rhs ) 
        {
            const IntensityAttribBase* attOth = static_cast<const IntensityAttribBase*>(rhs);
            this->m_value = attOth->m_value;
        }

    private:
        float m_value;
    };

    //////////////////////////////////////////////////////////////////////////
    // TextureAttribBase
    class TextureAttribBase : public MtrAttrib, public IResourceListener
    {
    public:
        typedef String ValueType;

    public:
        TextureAttribBase( MtrAttribType type ) : MtrAttrib(type) {}
        virtual ~TextureAttribBase();

        void setTexture( const String& texName );
        void setTexture( Texture* tex );

        Texture* getTexture() const { return m_texture.get(); }
        const String& getTextureName() const;

        void setValue( const String& texName ) { setTexture( texName ); }
        void setValue( Texture* tex ) { setTexture( tex ); }

        Texture* getValue() const { return getTexture(); }
        const String& getValueName() const { getTextureName(); }

        bool isTexturePrepared() const;

    public:
        virtual void _importAttrib( BinStreamReader& reader );
        virtual void _exportAttrib( BinStreamWriter& writer );
        virtual void copyFrom( const MtrAttrib* rhs );

    protected:
        virtual void onResourceLoaded( Resource* res );

    protected:
        TexturePtr m_texture;
    };


    //////////////////////////////////////////////////////////////////////////
#define KHAOS_ATTRIB_CLASS_(baseCls, name, id, defValInit) \
    class name : public baseCls \
    { \
    public: \
        enum { ID = id }; \
        name() : baseCls(ID) { setValue(defValInit); } \
        name( const ValueType& defVal ) : baseCls(ID, defVal) {} \
    };

#define KHAOS_RGBCLR_ATTRIB_CLASS(name, id, defVal) \
    KHAOS_ATTRIB_CLASS_(RGBColorAttribBase, name, id, defVal)

#define KHAOS_INTENSITY_ATTRIB_CLASS(name, id, defVal) \
    KHAOS_ATTRIB_CLASS_(IntensityAttribBase, name, id, defVal)

#define KHAOS_TEXTURE_ATTRIB_CLASS(name, id) \
    class name : public TextureAttribBase \
    { \
        public: \
        enum { ID = id }; \
        name() : TextureAttribBase(ID) {} \
    };


    //////////////////////////////////////////////////////////////////////////
    KHAOS_RGBCLR_ATTRIB_CLASS(BaseColorAttrib, MA_BASECOLOR, Color::GRAY)
    KHAOS_INTENSITY_ATTRIB_CLASS(MetallicAttrib, MA_METALLIC, 0.0f)
    KHAOS_INTENSITY_ATTRIB_CLASS(DSpecularAttrib, MA_DSPECULAR, 0.5f)
    KHAOS_INTENSITY_ATTRIB_CLASS(RoughnessAttrib, MA_ROUGHNESS, 0.5f)
    KHAOS_INTENSITY_ATTRIB_CLASS(AlphaAttrib, MA_ALPHA, 1.0f)
    KHAOS_INTENSITY_ATTRIB_CLASS(AlphaTestAttrib, MA_ALPHATEST, 0.5f)
    KHAOS_RGBCLR_ATTRIB_CLASS(EmissiveAttrib, MA_EMISSIVE, Color::WHITE)

    KHAOS_TEXTURE_ATTRIB_CLASS(BaseMapAttrib, MA_BASEMAP)
    KHAOS_TEXTURE_ATTRIB_CLASS(EmissiveMapAttrib, MA_EMISSIVEMAP)

    KHAOS_TEXTURE_ATTRIB_CLASS(NormalMapAttrib, MA_NORMALMAP)
    KHAOS_TEXTURE_ATTRIB_CLASS(OpacityMapAttrib, MA_OPACITYMAP)

    KHAOS_TEXTURE_ATTRIB_CLASS(BakedAOMapAttrib, MA_BAKEDAOMAP)

    //////////////////////////////////////////////////////////////////////////
    class SpecularMapAttrib : public TextureAttribBase
    {
    public:
        enum { ID = MA_SPECULARMAP };

        SpecularMapAttrib() : TextureAttribBase(ID), 
            m_metallicEnabled(false), m_dspecularEnabled(false), m_roughnessEnabled(false) {}

        void setMetallicEnabled( bool en );
        void setDSpecularEnabled( bool en );
        void setRoughnessEnabled( bool en );

        bool isMetallicEnabled()  const { return m_metallicEnabled;  }
        bool isDSpecularEnabled() const { return m_dspecularEnabled; }
        bool isRoughnessEnabled() const { return m_roughnessEnabled; }

    public:
        virtual void _importAttrib( BinStreamReader& reader );
        virtual void _exportAttrib( BinStreamWriter& writer );
        virtual void copyFrom( const MtrAttrib* rhs );

    private:
        int _validChannel( int c ) const;

    private:
        bool m_metallicEnabled;
        bool m_dspecularEnabled;
        bool m_roughnessEnabled;
    };

    //////////////////////////////////////////////////////////////////////////
    struct InnerAttribSet
    {
#define KHAOS_DEFINE_INNER_ATTR_(name, offVal, fullVal) \
        static const name* get##name( int kind ) \
        { \
            static name s_v_off( offVal ); \
            static name s_v_full( fullVal ); \
            return kind == 0 ? &s_v_off : &s_v_full; \
        }

        KHAOS_DEFINE_INNER_ATTR_(BaseColorAttrib, Color::GRAY, Color::WHITE)
        KHAOS_DEFINE_INNER_ATTR_(MetallicAttrib, 0.0f, 1.0f)
        KHAOS_DEFINE_INNER_ATTR_(DSpecularAttrib, 0.0f, 1.0f)
        KHAOS_DEFINE_INNER_ATTR_(RoughnessAttrib, 0.5f, 1.0f)
        KHAOS_DEFINE_INNER_ATTR_(EmissiveAttrib, Color::BLACK, Color::WHITE)
    };
}

