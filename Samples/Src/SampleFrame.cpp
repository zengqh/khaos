#include "SampleFrame.h"
#include <KhaosRoot.h>

using namespace Khaos;

SampleFrame* g_sampleFrame = 0;

SampleFrame::SampleFrame()
{
    khaosAssert( !g_sampleFrame );
    g_sampleFrame = this;
}

SampleFrame::~SampleFrame()
{
    g_sampleFrame = 0;
}

bool SampleFrame::create( const SampleCreateContext& scc )
{
    RootConfig rootConfig;

    rootConfig.rdcContext.handleWindow = scc.handleWindow;
    rootConfig.rdcContext.windowWidth  = scc.windowWidth;
    rootConfig.rdcContext.windowHeight = scc.windowHeight;
    rootConfig.fileSystemPath = String(scc.exePath) + "../Media/";

    KHAOS_NEW Root;
    g_root->init( rootConfig );

    return _onCreateScene();
}

void SampleFrame::destroy()
{
    // engine
    _onDestroyScene();
    g_root->shutdown();
    KHAOS_DELETE g_root;
}

bool SampleFrame::runIdle()
{
    if ( !_onUpdate() )
        return false;

    g_root->run();
    return true;
}

bool SampleFrame::_onUpdate()
{
    return m_samOp.update();
}

void SampleFrame::_processKeyCommon( int key )
{
    RenderSettings* mainSettings = g_renderSettingManager->getRenderSettings( "mainSettings" );

    if ( key == 'L' )
    {
        RenderMode curRM = mainSettings->getRenderMode();

        if ( curRM == RM_DEFERRED_SHADING )
            curRM = RM_FORWARD;
        else
            curRM = RM_DEFERRED_SHADING;

        mainSettings->setRenderMode( curRM );
        _outputDebugStrLn( "setRenderMode: %d", curRM );
    }
    else if ( key == 'T' )
    {
        bool en = mainSettings->isSettingEnabled<AntiAliasSetting>();
        mainSettings->enableSetting<AntiAliasSetting>( !en );
        _outputDebugStrLn( "AntiAliasSetting: %d", !en );
    }
    else if ( key == 'R' )
    {
        if ( AntiAliasSetting* aa = mainSettings->getSetting<AntiAliasSetting>() )
        {
            aa->setEnableTemporal( !aa->getEnableTemporal() );
            _outputDebugStrLn( "getEnableTemporal: %d", aa->getEnableTemporal() );
        }
    }
    else if ( key == 'Y' )
    {
        if ( AntiAliasSetting* aa = mainSettings->getSetting<AntiAliasSetting>() )
        {
            aa->setEnableMain( !aa->getEnableMain() );
            _outputDebugStrLn( "getEnableMain: %d", aa->getEnableMain() );
        }
    }
    else if ( key == 'O' )
    {
        bool en = mainSettings->isSettingEnabled<SSAORenderSetting>();
        mainSettings->enableSetting<SSAORenderSetting>( !en );
        _outputDebugStrLn( "SSAORenderSetting: %d", !en );
    }  
    else if ( key == 'H' )
    {
        bool en = mainSettings->isSettingEnabled<HDRSetting>();
        mainSettings->enableSetting<HDRSetting>( !en );
        _outputDebugStrLn( "HDRSetting: %d", !en );
    }  
    else if ( key == 'G' )
    {
        bool en = mainSettings->isSettingEnabled<GammaSetting>();
        mainSettings->enableSetting<GammaSetting>( !en );
        _outputDebugStrLn( "GammaSetting: %d", !en );
    } 
    else if ( key == 'N' )
    {
        mainSettings->setEnableNormalMap( !mainSettings->getEnableNormalMap() );
        _outputDebugStrLn( "setEnableNormalMap: %d", mainSettings->getEnableNormalMap() );
    }
}

