#include "KhaosPreHeaders.h"
#include "KhaosRenderSetting.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    RenderSettings::~RenderSettings()
    {
        KHAOS_FOR_EACH( RenderSettingMap, m_settings, it )
        {
            RenderSetting* setting = it->second;
            KHAOS_DELETE setting;
        }
    }

    void RenderSettings::_defaultSet()
    {
        setRenderMode( RM_DEFERRED_SHADING );
        setEnableNormalMap( true );
        createSetting<GammaSetting>();
    }

    void RenderSettings::_clearAllDirty()
    {
        KHAOS_FOR_EACH( RenderSettingMap, m_settings, it )
        {
            RenderSetting* setting = it->second;
            setting->_clearDirty();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    RenderSettingManager* g_renderSettingManager = 0;

    RenderSettingManager::RenderSettingManager()
    {
        khaosAssert( !g_renderSettingManager );
        g_renderSettingManager = this;
    }

    RenderSettingManager::~RenderSettingManager()
    {
        _clearAll();
        g_renderSettingManager = 0;
    }

    RenderSettings* RenderSettingManager::createRenderSettings( const String& name )
    {
        RenderSettings* settings = KHAOS_NEW RenderSettings;
        khaosAssert( !m_settings[name] );
        m_settings[name] = settings;
        return settings;
    }

    RenderSettings* RenderSettingManager::getRenderSettings( const String& name ) const
    {
        RenderSettingsMap::const_iterator it = m_settings.find( name );
        if ( it != m_settings.end() )
            return it->second;
        return 0;
    }

    void RenderSettingManager::_clearAll()
    {
        KHAOS_FOR_EACH( RenderSettingsMap, m_settings, it )
        {
            RenderSettings* settings = it->second;
            KHAOS_DELETE settings;
        }

        m_settings.clear();
    }

    //////////////////////////////////////////////////////////////////////////
    SSAORenderSetting::SSAORenderSetting()
    {
        setResolutionSize( RESSIZE_FULL );

        setDiskRadius( 1.5f );

        setSmallRadiusRatio( 0.3f );
        setLargeRadiusRatio( 4.0f );
        setBrighteningMargin( 1.1f );

        setContrast( 1.0f );
        setAmount( 1.0f );
    }

    //////////////////////////////////////////////////////////////////////////
    GammaSetting::GammaSetting()
    {
        setGammaValue( 2.2f );
    }

    //////////////////////////////////////////////////////////////////////////
    HDRSetting::HDRSetting()
    {
        setEyeAdaptionCache(4);
        setEyeAdaptationSpeed(2.0f);
        setEyeAdaptationFactor(0.85f);
        setEyeAdaptationBase( 0.25f );

        setBrightOffset(8.0f);
        setBrightThreshold(8.0f);
        setBrightLevel(1.25f);

        setBloomColor( Color::WHITE );
        setBloomMul( 0.8f );
        setGrainAmount( 0.6f );

        setHDRLevel(8.0f);
        setHDROffset(10.0f);

        setShoulderScale(1.0f);
        setMidtonesScale(1.0f);
        setToeScale(1.0f);
        setWhitePoint(4.0f);
    }

    //////////////////////////////////////////////////////////////////////////
    AntiAliasSetting::AntiAliasSetting()
    {
        setQualityLevel( QUALITY_HIGH );
        setPostAAMethod( POSTAA_FXAA );
        setEnableMain( true );
        setEnableTemporal( false );
    }
}

