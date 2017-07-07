#include "KhaosPreHeaders.h"
#include "KhaosTimer.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    Timer::Timer()
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        timeBeginPeriod( 1 );
        m_startTime = timeGetTime();
#endif
    }

    Timer::~Timer()
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        timeEndPeriod( 1 );
#endif
    }

    uint32 Timer::getTimeMS() const
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        DWORD elapsedTime = timeGetTime() - m_startTime;
        return elapsedTime;
#endif
    }

    float Timer::getTime() const
    {
        return getTimeMS() * 0.001f;
    }

    //////////////////////////////////////////////////////////////////////////
    TimerSystem& TimerSystem::instance()
    {
        static TimerSystem inst;
        return inst;
    }

    TimerSystem::TimerSystem()
    {
        m_curTime = getCurImmTime();
        m_elapsedTime = 0;
    }

    void TimerSystem::update()
    {
        float curTime = getCurImmTime();
        m_elapsedTime = curTime - m_curTime;
        m_curTime = curTime;
    }

    //////////////////////////////////////////////////////////////////////////
    DebugTestTimer::DebugTestTimer( const String& info ) : m_info(info)
    {
        m_time = g_timerSystem.getCurImmTimeMS();
    }

    DebugTestTimer::~DebugTestTimer()
    {
        uint32 curTime = g_timerSystem.getCurImmTimeMS();
        int usedTime = (int)(curTime - m_time);

        int usedTimeSec = usedTime / 1000;
        int usedMS      = usedTime % 1000;

        int usedTimeMin = usedTimeSec / 60;
        int usedSec     = usedTimeSec % 60;

        int usedHour    = usedTimeMin / 60;
        int usedMin     = usedTimeMin % 60;

        String debugInfo = m_info + 
            " => [" + intToString(usedTime) + " ms] " + 
            intToString(usedHour) + " hour " + 
            intToString(usedMin) + " min " + 
            intToString(usedSec) + " sec " +
            intToString(usedMS) + " ms\n";

        Khaos::_outputDebugStr( debugInfo.c_str() );
    }
}

