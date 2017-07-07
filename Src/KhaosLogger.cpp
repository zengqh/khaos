#include "KhaosPreHeaders.h"
#include "KhaosLogger.h"

#define KHAOS_MAX_LOGSTR_LEN 4096

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    Logger::Logger() : m_logFile("log.txt"), m_minLevel(L_3), m_fp(0)
    {
    }

    Logger::~Logger()
    {
        if ( m_fp )
            fclose( m_fp );
    }

    Logger& Logger::instance()
    {
        static Logger inst;
        return inst;
    }

    void Logger::setLogFile( const std::string& file )
    {
        m_logFile = file;
    }

    void Logger::setMinLevel( Level level )
    {
        m_minLevel = level;
    }

    void Logger::printMsg( Level level, const char* fmt, ... )
    {
        if ( level > m_minLevel )
            return;

        KHAOS_STRFMT( buff, KHAOS_MAX_LOGSTR_LEN, fmt );
        _writeMsg( level, buff );
    }

    void Logger::printMsgLn( Level level, const char* fmt, ... )
    {
        if ( level > m_minLevel )
            return;

        KHAOS_STRFMT_LN( buff, KHAOS_MAX_LOGSTR_LEN, fmt );
        _writeMsg( level, buff );
    }

    const char* Logger::_levelToStr( Level level )
    {
        switch ( level )
        {
        case L_1:
            return "L_1";

        case L_2:
            return "L_2";

        case L_3:
            return "L_3";

        default:
            return "";
        }
    }

    void Logger::_writeMsg( Level level, const char* msg )
    {
        std::string tot;
        tot += "[";
        tot += _levelToStr(level);
        tot += "]";
        tot += msg;

        KHAOS_LOCK_MUTEX

        if ( !m_fp )
            m_fp = fopen( m_logFile.c_str(), "w" );

        fputs( tot.c_str(), m_fp );
        fflush( m_fp );
    }
}

