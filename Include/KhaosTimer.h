#pragma once
#include "KhaosStdTypes.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class Timer
    {
    public:
        Timer();
        ~Timer();

    public:
        uint32 getTimeMS() const;
        float  getTime() const;
        
    private:
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        DWORD m_startTime;
#endif
    };

    //////////////////////////////////////////////////////////////////////////
    class TimerSystem
    {
    public:
        TimerSystem();

        static TimerSystem& instance();

        // 每帧调度
        void update();

        // 获取当前立即的时间
        uint32 getCurImmTimeMS() const { return m_timer.getTimeMS(); }

        float getCurImmTime() const { return m_timer.getTime(); }

        // 返回当前帧的时间
        float getCurTime() const { return m_curTime; }

        // 返回当前帧到上一帧逝去的时间
        float getElapsedTime() const { return m_elapsedTime; }

    private:
        Timer m_timer;
        float m_curTime;
        float m_elapsedTime;
    };

    #define g_timerSystem TimerSystem::instance()

    //////////////////////////////////////////////////////////////////////////
    class DebugTestTimer : public AllocatedObject
    {
    public:
        DebugTestTimer( const String& info );
        ~DebugTestTimer();

    private:
        uint32 m_time;
        String m_info;
    };

    #define KHAOS_DEBUG_TEST_TIMER(x)  DebugTestTimer dtt_(x);
    //#define KHAOS_DEBUG_TEST_TIMER(x)  (void)0;
}

