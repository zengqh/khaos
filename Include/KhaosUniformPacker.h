#pragma once
#include "KhaosBinStream.h"
#include "KhaosVector2.h"
#include "KhaosVector3.h"
#include "KhaosVector4.h"
#include "KhaosColor.h"
#include "KhaosDebug.h"


namespace Khaos
{
    class UniformPacker : public BinStreamWriter
    {
    public:
        using BinStreamWriter::write;

    public:
        UniformPacker( uint needBytes ) : BinStreamWriter( needBytes )
        {
        }

        UniformPacker( void* block, uint size ) : BinStreamWriter(block, size)
        {
        }

        void writeAsVector4( float v )
        {
            write( v );
            _writeBlankFloat(3);
        }

        void writeAsVector4( const Vector2& v )
        {
            write( v );
            _writeBlankFloat(2);
        }

        void writeAsVector4( const Vector3& v )
        {
            write( v );
            _writeBlankFloat(1);
        }

        void writeAsVector4( const Vector4& v )
        {
            write( v );
        }

        void writeAsVector4( const Color& clr )
        {
            KhaosStaticAssert( sizeof(clr) == sizeof(Vector4) );
            write( clr );
        }

        void writeAsVector4( const float* v, int arrsize )
        {
            khaosAssert( arrsize <= 4 );

            if ( arrsize > 4 ) 
                arrsize = 4;
            write( v, sizeof(float) * arrsize );

            int needs = 4 - arrsize;
            if ( needs > 0 )
                _writeBlankFloat( needs );
        }

        void writeVector4Array( const Vector4* v, int arrSize )
        {
            write( v->ptr(), sizeof(Vector4)*arrSize );
        }

    private:
        void _writeBlankFloat( uint c )
        {
            static float t[4] = {};
            khaosAssert( c <= 4 );
            write( t, sizeof(float)*c );
        }
    };
}

