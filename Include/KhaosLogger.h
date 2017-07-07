#pragma once
#include "KhaosThread.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class Logger
    {
    public:
        enum Level
        {
            L_DISABLED,
            L_1,
            L_2,
            L_3
        };

    private:
        KHAOS_OBJECT_MUTEX

    public:
        Logger();
        ~Logger();

        static Logger& instance();

    public:
        void setLogFile( const std::string& file );
        void setMinLevel( Level level );

    public:
        void printMsg( Level level, const char* fmt, ... );
        void printMsgLn( Level level, const char* fmt, ... );

    private:
        const char* _levelToStr( Level level );
        void _writeMsg( Level level, const char* msg );

    private:
        std::string m_logFile;
        Level       m_minLevel;
        FILE*       m_fp;
    };

    //////////////////////////////////////////////////////////////////////////
    #define g_logger    Logger::instance()
    #define KHL_L1      Khaos::Logger::L_1
    #define KHL_L2      Khaos::Logger::L_2
    #define KHL_L3      Khaos::Logger::L_3
    #define khaosLog    Khaos::g_logger.printMsg
    #define khaosLogLn  Khaos::g_logger.printMsgLn
}

