#pragma once
#include "KhaosColor.h"
#include "KhaosBitSet.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class SkyLayer : public AllocatedObject
    {
    public:
        enum
        {
            SIMPLE_COLOR
        };

    public:
        SkyLayer();

        void setType( int type ) { m_type = type; }
        int  getType() const { return m_type; }

        void setColor( const Color& clr ) { m_color = clr; }
        const Color& getColor() const { return m_color; }

        void enableBlend( bool b ) { m_blendEnabled = b; }
        bool isBlendEnabled() const { return m_blendEnabled; }

    private:
        int   m_type;
        Color m_color;
        bool  m_blendEnabled;
    };

    //////////////////////////////////////////////////////////////////////////
    class SkyEnv : public AllocatedObject
    {
        typedef vector<SkyLayer>::type SkyLayers;

    public:
        enum
        {
            SKY_NULL        = 0x0,
            SKY_SIMPLEAMB   = 0x1
        };

    public:
        SkyEnv();

    public:
        // sky layers
        void setSkyLayersCount( int layersCnt );
        void insertSkyLayer( int index );
        void removeSkyLayer( int index );

        int       getSkyLayersCount() const { return (int)m_skyLayers.size(); }
        SkyLayer* getSkyLayer( int index );

        // sky lighting
        void setAmbConstClr( const Color& clr );
        void setAmbUpperClr( const Color& clr );
        void setAmbLowerClr( const Color& clr );

        const Color& getAmbConstClr() const { return m_ambConstClr; }
        const Color& getAmbUpperClr() const { return m_ambUpperClr; }
        const Color& getAmbLowerClr() const { return m_ambLowerclr; }

    public:
        void setEnabled( uint item );
        bool isEnabled( uint item ) const;

    private:
        SkyLayers m_skyLayers; 

        Color m_ambConstClr;
        Color m_ambUpperClr;
        Color m_ambLowerclr;

        BitSet32 m_flag;
    };
}

