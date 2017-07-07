#include "KhaosPreHeaders.h"
#include "KhaosStrConv.h"
#include "KhaosStringUtil.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
#define KHAOS_BEG_STR_TO_MAC(enumType) \
    enumType strTo##enumType( const char* str ) \
    { \
        static map<String, int>::type s_map; \
        if ( s_map.empty() ) \
        {

#define KHAOS_BEG_MAC_TO_STR(enumType) \
    const char* enumType##ToStr( enumType e ) \
    { \
        switch ( e ) \
        {
    
#define KHAOS_STR_TO_MAC_ITEM(val) \
            s_map[#val] = KHAOS_PP_CAT(KHAOS_STR_TO_MAC_PRE, val);

#define KHAOS_MAC_TO_STR_ITEM(val) \
            case KHAOS_PP_CAT(KHAOS_STR_TO_MAC_PRE, val) : return #val;

#define KHAOS_END_STR_TO_MAC(enumType) \
        } \
        map<String, int>::type::iterator it = s_map.find(str); \
        if ( it != s_map.end() ) \
            return (enumType)it->second; \
        khaosLogLn( KHL_L2, "strToEnumType: not found %s", str ); \
        return (enumType)-1; \
    }

#define KHAOS_END_MAC_TO_STR(enumType) \
        } \
        return ""; \
    }

#define KHAOS_BUILD_STR_TO_MAC_2(enumType, v0, v1) \
    KHAOS_BEG_STR_TO_MAC(enumType) \
    KHAOS_STR_TO_MAC_ITEM(v0) \
    KHAOS_STR_TO_MAC_ITEM(v1) \
    KHAOS_END_STR_TO_MAC(enumType) \
    KHAOS_BEG_MAC_TO_STR(enumType) \
    KHAOS_MAC_TO_STR_ITEM(v0) \
    KHAOS_MAC_TO_STR_ITEM(v1) \
    KHAOS_END_MAC_TO_STR(enumType)

#define KHAOS_BUILD_STR_TO_MAC_3(enumType, v0, v1, v2) \
    KHAOS_BEG_STR_TO_MAC(enumType) \
    KHAOS_STR_TO_MAC_ITEM(v0) \
    KHAOS_STR_TO_MAC_ITEM(v1) \
    KHAOS_STR_TO_MAC_ITEM(v2) \
    KHAOS_END_STR_TO_MAC(enumType) \
    KHAOS_BEG_MAC_TO_STR(enumType) \
    KHAOS_MAC_TO_STR_ITEM(v0) \
    KHAOS_MAC_TO_STR_ITEM(v1) \
    KHAOS_MAC_TO_STR_ITEM(v2) \
    KHAOS_END_MAC_TO_STR(enumType)

#define KHAOS_BUILD_STR_TO_MAC_4(enumType, v0, v1, v2, v3) \
    KHAOS_BEG_STR_TO_MAC(enumType) \
    KHAOS_STR_TO_MAC_ITEM(v0) \
    KHAOS_STR_TO_MAC_ITEM(v1) \
    KHAOS_STR_TO_MAC_ITEM(v2) \
    KHAOS_STR_TO_MAC_ITEM(v3) \
    KHAOS_END_STR_TO_MAC(enumType) \
    KHAOS_BEG_MAC_TO_STR(enumType) \
    KHAOS_MAC_TO_STR_ITEM(v0) \
    KHAOS_MAC_TO_STR_ITEM(v1) \
    KHAOS_MAC_TO_STR_ITEM(v2) \
    KHAOS_MAC_TO_STR_ITEM(v3) \
    KHAOS_END_MAC_TO_STR(enumType)

#define KHAOS_BUILD_STR_TO_MAC_5(enumType, v0, v1, v2, v3, v4) \
    KHAOS_BEG_STR_TO_MAC(enumType) \
    KHAOS_STR_TO_MAC_ITEM(v0) \
    KHAOS_STR_TO_MAC_ITEM(v1) \
    KHAOS_STR_TO_MAC_ITEM(v2) \
    KHAOS_STR_TO_MAC_ITEM(v3) \
    KHAOS_STR_TO_MAC_ITEM(v4) \
    KHAOS_END_STR_TO_MAC(enumType) \
    KHAOS_BEG_MAC_TO_STR(enumType) \
    KHAOS_MAC_TO_STR_ITEM(v0) \
    KHAOS_MAC_TO_STR_ITEM(v1) \
    KHAOS_MAC_TO_STR_ITEM(v2) \
    KHAOS_MAC_TO_STR_ITEM(v3) \
    KHAOS_MAC_TO_STR_ITEM(v4) \
    KHAOS_END_MAC_TO_STR(enumType)

#define KHAOS_BUILD_STR_TO_MAC_6(enumType, v0, v1, v2, v3, v4, v5) \
    KHAOS_BEG_STR_TO_MAC(enumType) \
    KHAOS_STR_TO_MAC_ITEM(v0) \
    KHAOS_STR_TO_MAC_ITEM(v1) \
    KHAOS_STR_TO_MAC_ITEM(v2) \
    KHAOS_STR_TO_MAC_ITEM(v3) \
    KHAOS_STR_TO_MAC_ITEM(v4) \
    KHAOS_STR_TO_MAC_ITEM(v5) \
    KHAOS_END_STR_TO_MAC(enumType) \
    KHAOS_BEG_MAC_TO_STR(enumType) \
    KHAOS_MAC_TO_STR_ITEM(v0) \
    KHAOS_MAC_TO_STR_ITEM(v1) \
    KHAOS_MAC_TO_STR_ITEM(v2) \
    KHAOS_MAC_TO_STR_ITEM(v3) \
    KHAOS_MAC_TO_STR_ITEM(v4) \
    KHAOS_MAC_TO_STR_ITEM(v5) \
    KHAOS_END_MAC_TO_STR(enumType)

    //////////////////////////////////////////////////////////////////////////
#define KHAOS_STR_TO_MAC_PRE TEXTYPE_
        KHAOS_BUILD_STR_TO_MAC_4(TextureType, 2D, 3D, CUBE, VOLUME)
#undef KHAOS_STR_TO_MAC_PRE


#define KHAOS_STR_TO_MAC_PRE TEXUSA_
        KHAOS_BUILD_STR_TO_MAC_4(TextureUsage, STATIC, DYNAMIC, RENDERTARGET, DEPTHSTENCIL)
#undef KHAOS_STR_TO_MAC_PRE


#define KHAOS_STR_TO_MAC_PRE TEXF_
        KHAOS_BUILD_STR_TO_MAC_6(TextureFilter, NONE, POINT, LINEAR, ANISOTROPIC, PYRAMIDALQUAD, GAUSSIANQUAD)
#undef KHAOS_STR_TO_MAC_PRE


#define KHAOS_STR_TO_MAC_PRE TEXADDR_
        KHAOS_BUILD_STR_TO_MAC_5(TextureAddress, WRAP, MIRROR, CLAMP, BORDER, MIRRORONCE)
#undef KHAOS_STR_TO_MAC_PRE

    //////////////////////////////////////////////////////////////////////////
    Color strToColor( const String& str )
    {
        StringVector outlists; 
        splitString<String>( outlists, str, ", ", true );
        
        if ( outlists.size() < 3 )
            outlists.resize(3);

        if ( outlists.size() < 4 )
            outlists.push_back( "1" );

        return Color( stringToFloat(outlists[0].c_str()), stringToFloat(outlists[1].c_str()), 
            stringToFloat(outlists[2].c_str()), stringToFloat(outlists[3].c_str()) );
    }

    Color strsToColor( const String* v, int n )
    {
        Color c( Color::BLACK );

        for ( int i = 0; i < n; ++i )
            c[i] = stringToFloat( v[i].c_str() );

        return c;
    }

    String colorToStr( const Color& clr )
    {
        return floatToString( clr.r ) + "," +
               floatToString( clr.g ) + "," +
               floatToString( clr.b ) + "," +
               floatToString( clr.a );
    }

    //////////////////////////////////////////////////////////////////////////
    Vector2 strsToVector2( const String v[2] )
    {
        return Vector2(
                stringToFloat( v[0].c_str() ),
                stringToFloat( v[1].c_str() )
            );
    }

    Vector3 strsToVector3( const String v[3] )
    {
        return Vector3(
            stringToFloat( v[0].c_str() ),
            stringToFloat( v[1].c_str() ),
            stringToFloat( v[2].c_str() )
            );
    }
}

