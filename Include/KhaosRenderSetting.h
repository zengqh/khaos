#pragma once
#include "KhaosStdTypes.h"
#include "KhaosRTTI.h"
#include "KhaosColor.h"


namespace Khaos
{
#define SETTING_PROPERTY_BY_VAL(type, name) \
    public: \
        void set##name( type v ) { m_##name = v; _setDirty(); } \
        type get##name() const { return m_##name; } \
    private:\
        type m_##name;

#define SETTING_PROPERTY_BY_REF(type, name) \
    public: \
        void set##name( const type& v ) { m_##name = v; _setDirty(); } \
        const type& get##name() const { return m_##name; } \
    private:\
        type m_##name;

    //////////////////////////////////////////////////////////////////////////
    enum RenderMode
    {
        RM_FORWARD,
        RM_DEFERRED_SHADING
    };

    enum ResolutionSize
    {
        RESSIZE_FULL,
        RESSIZE_THREE_QUARTER,
        RESSIZE_HALF
    };

    enum QualityLevel
    {
        QUALITY_LOW,
        QUALITY_MID,
        QUALITY_HIGH
    };

    enum PostAAMethod
    {
        POSTAA_SMAA,
        POSTAA_FXAA
    };

    //////////////////////////////////////////////////////////////////////////
    class RenderSetting : public AllocatedObject
    {
        KHAOS_DECLARE_RTTI(RenderSetting)

    public:
        RenderSetting() : m_dirty(false)
        {
            setEnabled( true );
        }

        virtual ~RenderSetting() {}

    public:
        SETTING_PROPERTY_BY_VAL(bool, Enabled)

    public:
        bool _isDirty() const { return m_dirty; }
        void _clearDirty() { m_dirty = false; }

    protected:
        void _setDirty() { m_dirty = true; }

    protected:
        bool m_dirty;
    };

    //////////////////////////////////////////////////////////////////////////
    class RenderSettings : public AllocatedObject
    {
    private:
        typedef map<ClassType, RenderSetting*>::type RenderSettingMap;

    public:
        RenderSettings()
        {
            _defaultSet();
        }

        ~RenderSettings();

        template<class T>
        T* createSetting()
        {
            if ( T* setting = getSetting<T>() )
                return setting;

            T* setting = KHAOS_NEW T;
            m_settings[KHAOS_CLASS_TYPE(T)] = setting;
            return setting;
        }
        
        RenderSetting* getSetting( ClassType type ) const
        {
            RenderSettingMap::const_iterator it = m_settings.find( type );
            if ( it != m_settings.end() )
                return it->second;
            return 0;
        }

        RenderSetting* getEnabledSetting( ClassType type ) const
        {
            RenderSetting* ret = getSetting( type );
            if ( ret && ret->getEnabled() )
                return ret;
            return 0;
        }

        template<class T>
        T* getSetting() const
        {
            return static_cast<T*>( getSetting( KHAOS_CLASS_TYPE(T) ) );
        }

        template<class T>
        T* getEnabledSetting() const
        {
            return static_cast<T*>( getEnabledSetting( KHAOS_CLASS_TYPE(T) ) );
        }

        void enableSetting( ClassType type, bool en )
        {
            if ( RenderSetting* setting = getSetting(type) )
                setting->setEnabled( en );
        }

        template<class T>
        void enableSetting( bool en )
        {
            enableSetting( KHAOS_CLASS_TYPE(T), en );
        }

        bool isSettingEnabled( ClassType type ) const
        {
            if ( RenderSetting* settings = getSetting(type) )
                return settings->getEnabled();
            else
                return false;
        }

        template<class T>
        bool isSettingEnabled() const
        {
            return isSettingEnabled( KHAOS_CLASS_TYPE(T) );
        }

    public:
        // common setting
        SETTING_PROPERTY_BY_VAL(RenderMode, RenderMode)
        SETTING_PROPERTY_BY_VAL(bool, EnableNormalMap)

    public:
        void _clearAllDirty(); // 每帧结束会调用清楚掉

    private:
        void _defaultSet();

        void _setDirty() {}

    private:
        RenderSettingMap m_settings;
    };

    //////////////////////////////////////////////////////////////////////////
    class RenderSettingManager : public AllocatedObject
    {
        typedef map<String, RenderSettings*>::type RenderSettingsMap;

    public:
        RenderSettingManager();
        ~RenderSettingManager();

        RenderSettings* createRenderSettings( const String& name );
        RenderSettings* getRenderSettings( const String& name ) const;

    private:
        void _clearAll();

    private:
        RenderSettingsMap m_settings;
    };

    extern RenderSettingManager* g_renderSettingManager;

    //////////////////////////////////////////////////////////////////////////
    class SSAORenderSetting : public RenderSetting
    {
        KHAOS_DECLARE_RTTI(SSAORenderSetting)

    public:
        SSAORenderSetting();

    public:
        SETTING_PROPERTY_BY_VAL(ResolutionSize, ResolutionSize)

        // ssao
        SETTING_PROPERTY_BY_VAL(float, DiskRadius)

        SETTING_PROPERTY_BY_VAL(float, SmallRadiusRatio)
        SETTING_PROPERTY_BY_VAL(float, LargeRadiusRatio)
        SETTING_PROPERTY_BY_VAL(float, BrighteningMargin)

        SETTING_PROPERTY_BY_VAL(float, Contrast)
        SETTING_PROPERTY_BY_VAL(float, Amount)
    };

    //////////////////////////////////////////////////////////////////////////
    class GammaSetting : public RenderSetting
    {
        KHAOS_DECLARE_RTTI(GammaSetting)

    public:
        GammaSetting();

    public:
        SETTING_PROPERTY_BY_VAL(float, GammaValue)
    };

    //////////////////////////////////////////////////////////////////////////
    class HDRSetting : public RenderSetting
    {
        KHAOS_DECLARE_RTTI(HDRSetting)

    public:
        HDRSetting();

        SETTING_PROPERTY_BY_VAL(int,   EyeAdaptionCache)
        SETTING_PROPERTY_BY_VAL(float, EyeAdaptationSpeed)
        SETTING_PROPERTY_BY_VAL(float, EyeAdaptationFactor)
        SETTING_PROPERTY_BY_VAL(float, EyeAdaptationBase)
        
        SETTING_PROPERTY_BY_VAL(float, BrightOffset)
        SETTING_PROPERTY_BY_VAL(float, BrightThreshold)
        SETTING_PROPERTY_BY_VAL(float, BrightLevel)

        SETTING_PROPERTY_BY_VAL(Color, BloomColor)
        SETTING_PROPERTY_BY_VAL(float, BloomMul)
        SETTING_PROPERTY_BY_VAL(float, GrainAmount)

        SETTING_PROPERTY_BY_VAL(float, HDRLevel)
        SETTING_PROPERTY_BY_VAL(float, HDROffset)

        SETTING_PROPERTY_BY_VAL(float, ShoulderScale)
        SETTING_PROPERTY_BY_VAL(float, MidtonesScale)
        SETTING_PROPERTY_BY_VAL(float, ToeScale)
        SETTING_PROPERTY_BY_VAL(float, WhitePoint)
    };

    //////////////////////////////////////////////////////////////////////////
    class AntiAliasSetting : public RenderSetting
    {
        KHAOS_DECLARE_RTTI(AntiAliasSetting)

    public:
        AntiAliasSetting();

    public:
        SETTING_PROPERTY_BY_VAL(QualityLevel, QualityLevel)
        SETTING_PROPERTY_BY_VAL(PostAAMethod, PostAAMethod)
        SETTING_PROPERTY_BY_VAL(bool, EnableMain)
        SETTING_PROPERTY_BY_VAL(bool, EnableTemporal)
    };
}

