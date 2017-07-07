#pragma once
#include "KhaosRenderDeviceDef.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    struct ISimpleTexData
    {
        virtual int getWidth() const = 0;
        virtual int getHeight() const = 0;

        virtual float getAlphaVal( int x, int y ) const = 0;
        virtual void  setAlphaVal( int x, int y, float a ) = 0;
    };

    struct Dist2AlphaConfig
    {
        enum 
        {
            R_ENABLED = 0x1,
            G_ENABLED = 0x2,
            B_ENABLED = 0x4,
            A_ENABLED = 0x8,
        };

        Dist2AlphaConfig() : 
            spread(1.0f), alphaInSideRef(0.2f),
            destTexFmt(PIXFMT_INVALID),
            destWidth(0), destHeight(0),
            mipmapGenMinSize(0), mipmapReadSize(0), mipmapFilter(TEXF_LINEAR),
            sRGBWrite(-1),
            flag(A_ENABLED) {}

        float spread;
        float alphaInSideRef;

        PixelFormat destTexFmt;
        
        int destWidth;
        int destHeight;

        int mipmapGenMinSize;
        int mipmapReadSize;
        int mipmapFilter;

        int sRGBWrite;

        uint flag;
    };

    void dist2Alpha( ISimpleTexData* srcData, ISimpleTexData* destData, const Dist2AlphaConfig& cfg );


    //////////////////////////////////////////////////////////////////////////
    class TextureObjUnit;
    void dist2Alpha( TextureObjUnit* srcTex, const String& destTexFile, const Dist2AlphaConfig& cfg );
}

