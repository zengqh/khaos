#pragma once
#include "KhaosRenderDeviceDef.h"
#include "KhaosColor.h"
#include "KhaosVector2.h"
#include "KhaosVector3.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
#define KHAOS_DECL_STR_TO_MAC(enumType) \
    enumType strTo##enumType( const char* str ); \
    const char* enumType##ToStr( enumType e );

    KHAOS_DECL_STR_TO_MAC(TextureType)
    KHAOS_DECL_STR_TO_MAC(TextureUsage)
    KHAOS_DECL_STR_TO_MAC(TextureFilter)
    KHAOS_DECL_STR_TO_MAC(TextureAddress)

    //////////////////////////////////////////////////////////////////////////
    Color strToColor( const String& str );
    Color strsToColor( const String* v, int n );
    String colorToStr( const Color& clr );

    Vector2 strsToVector2( const String v[2] );
    Vector3 strsToVector3( const String v[3] );
}

