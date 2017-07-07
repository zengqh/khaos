#include "KhaosPreHeaders.h"
#include "KhaosSkyEnv.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    SkyLayer::SkyLayer() : m_type(0), m_blendEnabled(true)
    {
    }


    //////////////////////////////////////////////////////////////////////////
    SkyEnv::SkyEnv()
    {
    }

    void SkyEnv::setSkyLayersCount( int layersCnt )
    {
        m_skyLayers.resize( layersCnt );
    }

    void SkyEnv::insertSkyLayer( int index )
    {
        m_skyLayers.insert( m_skyLayers.begin()+index, SkyLayer() );
    }

    void SkyEnv::removeSkyLayer( int index )
    {
        m_skyLayers.erase( m_skyLayers.begin()+index );
    }

    SkyLayer* SkyEnv::getSkyLayer( int index )
    {
        return &m_skyLayers[index];
    }

    void SkyEnv::setAmbConstClr( const Color& clr )
    {
        m_ambConstClr = clr;
    }

    void SkyEnv::setAmbUpperClr( const Color& clr )
    {
        m_ambUpperClr = clr;
    }

    void SkyEnv::setAmbLowerClr( const Color& clr )
    {
        m_ambLowerclr = clr;
    }

    void SkyEnv::setEnabled( uint item )
    {
        m_flag.setFlag( item );
    }

    bool SkyEnv::isEnabled( uint item ) const
    {
        return m_flag.testFlag( item );
    }
}

