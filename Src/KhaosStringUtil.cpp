#pragma once
#include "KhaosPreHeaders.h"
#include "KhaosStringUtil.h"

#define MAX_CONV_STR_BUFF_SIZE 64

namespace Khaos
{
    String intToString( int i )
    {
        char buf[MAX_CONV_STR_BUFF_SIZE] = {};
        _ltoa( i, buf, 10 );
        return buf;
    }

    String uint64ToString( uint64 x )
    {
        char buf[MAX_CONV_STR_BUFF_SIZE] = {};
        _ui64toa( x, buf, 10 );
        return buf;
    }

    String uint64ToHexStr( uint64 x )
    {
        char buf[MAX_CONV_STR_BUFF_SIZE] = {};
        _ui64toa( x, buf, 16 );
        return buf;
    }

    String ptrToString( void* p )
    {
        char buf[MAX_CONV_STR_BUFF_SIZE] = {};
        _ui64toa( (uint64)p, buf, 16 );
        return buf;
    }

    float stringToFloat( const char* str )
    {
        float v = 0;
        sscanf( str, "%f", &v );
        return v;
    }

    float stringToFloat( const wchar_t* str )
    {
        float v = 0;
        swscanf( str, L"%f", &v );
        return v;
    }

    String floatToString( float v )
    {
        char buf[MAX_CONV_STR_BUFF_SIZE] = {0};
        sprintf( buf, "%f", v );
        return buf;
    }

    void toLower( String& str )
    {
        std::transform( str.begin(), str.end(), str.begin(), tolower );
    }
}

