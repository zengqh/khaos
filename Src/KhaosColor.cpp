#include "KhaosPreHeaders.h"
#include "KhaosColor.h"

namespace Khaos 
{
    const Color Color::ZERO  = Color( 0.0f, 0.0f, 0.0f, 0.0f );
    const Color Color::BLACK = Color( 0.0f, 0.0f, 0.0f, 1.0f );
    const Color Color::WHITE = Color( 1.0f, 1.0f, 1.0f, 1.0f );
    const Color Color::GRAY  = Color( 0.5f, 0.5f, 0.5f, 1.0f );

    const float Color::COLOR_EPSILON = 1e-3f;

    //////////////////////////////////////////////////////////////////////////
    uint32 Color::makeARGB( int a, int r, int g, int b )
    {
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    uint8 Color::f2b( float v )
    {
        return Math::clamp( Math::roundI(v * 255), 0, 255 );
    }

    uint16 Color::f2h( float v )
    {
        return Math::toFloat16( v );
    }

    uint32 Color::getAsRGBA() const
    {
        uint8 val8;
        uint32 val32 = 0;

        // Convert to 32bit pattern
        // (RGBA = 8888)

        // Red
        val8 = f2b(r);
        val32 = val8 << 24;

        // Green
        val8 = f2b(g);
        val32 += val8 << 16;

        // Blue
        val8 = f2b(b);
        val32 += val8 << 8;

        // Alpha
        val8 = f2b(a);
        val32 += val8;

        return val32;
    }

    uint32 Color::getAsARGB() const
    {
        uint8 val8;
        uint32 val32 = 0;

        // Convert to 32bit pattern
        // (ARGB = 8888)

        // Alpha
        val8 = f2b(a);
        val32 = val8 << 24;

        // Red
        val8 = f2b(r);
        val32 += val8 << 16;

        // Green
        val8 = f2b(g);
        val32 += val8 << 8;

        // Blue
        val8 = f2b(b);
        val32 += val8;

        return val32;
    }

	uint32 Color::getAsBGRA() const
	{
		uint8 val8;
		uint32 val32 = 0;

		// Convert to 32bit pattern
		// (ARGB = 8888)

		// Blue
		val8 = f2b(b);
		val32 = val8 << 24;

		// Green
		val8 = f2b(g);
		val32 += val8 << 16;

		// Red
		val8 = f2b(r);
		val32 += val8 << 8;

		// Alpha
		val8 = f2b(a);
		val32 += val8;

		return val32;
	}

    uint32 Color::getAsABGR() const
    {
        uint8 val8;
        uint32 val32 = 0;

        // Convert to 32bit pattern
        // (ABRG = 8888)

        // Alpha
        val8 = f2b(a);
        val32 = val8 << 24;

        // Blue
        val8 = f2b(b);
        val32 += val8 << 16;

        // Green
        val8 = f2b(g);
        val32 += val8 << 8;

        // Red
        val8 = f2b(r);
        val32 += val8;

        return val32;
    }

    void Color::setAsRGBA( uint32 val )
    {
        uint32 val32 = val;

        // Convert from 32bit pattern
        // (RGBA = 8888)

        // Red
        r = ((val32 >> 24) & 0xFF) / 255.0f;

        // Green
        g = ((val32 >> 16) & 0xFF) / 255.0f;

        // Blue
        b = ((val32 >> 8) & 0xFF) / 255.0f;

        // Alpha
        a = (val32 & 0xFF) / 255.0f;
    }

    void Color::setAsARGB( uint32 val )
    {
        uint32 val32 = val;

        // Convert from 32bit pattern
        // (ARGB = 8888)

        // Alpha
        a = ((val32 >> 24) & 0xFF) / 255.0f;

        // Red
        r = ((val32 >> 16) & 0xFF) / 255.0f;

        // Green
        g = ((val32 >> 8) & 0xFF) / 255.0f;

        // Blue
        b = (val32 & 0xFF) / 255.0f;
    }

	void Color::setAsBGRA( uint32 val )
	{
		uint32 val32 = val;

		// Convert from 32bit pattern
		// (ARGB = 8888)

		// Blue
		b = ((val32 >> 24) & 0xFF) / 255.0f;

		// Green
		g = ((val32 >> 16) & 0xFF) / 255.0f;

		// Red
		r = ((val32 >> 8) & 0xFF) / 255.0f;

		// Alpha
		a = (val32 & 0xFF) / 255.0f;
	}

    void Color::setAsABGR( uint32 val )
    {
        uint32 val32 = val;

        // Convert from 32bit pattern
        // (ABGR = 8888)

        // Alpha
        a = ((val32 >> 24) & 0xFF) / 255.0f;

        // Blue
        b = ((val32 >> 16) & 0xFF) / 255.0f;

        // Green
        g = ((val32 >> 8) & 0xFF) / 255.0f;

        // Red
        r = (val32 & 0xFF) / 255.0f;
    }
}

