#include "KhaosPreHeaders.h"
#include "KhaosDebug.h"
#include "KhaosOSHeaders.h"

#define KHAOS_MAX_OUTPUTSTR_LEN 4096

namespace Khaos
{

//#if KHAOS_DEBUG
    #if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        void _outputDebugStr( const char* fmt, ... )
        {
            KHAOS_STRFMT( buff, KHAOS_MAX_OUTPUTSTR_LEN, fmt );
            OutputDebugStringA( buff );
        }

        void _outputDebugStrLn( const char* fmt, ... )
        {
            KHAOS_STRFMT_LN( buff, KHAOS_MAX_OUTPUTSTR_LEN, fmt );
            OutputDebugStringA( buff );
        }
    #else
        void _outputDebugStr( const char* fmt, ... )
        {
            KHAOS_STRFMT( buff, KHAOS_MAX_OUTPUTSTR_LEN, fmt );
            puts( buff );
        }

        void _outputDebugStrLn( const char* fmt, ... )
        {
            KHAOS_STRFMT_LN( buff, KHAOS_MAX_OUTPUTSTR_LEN, fmt );
            puts( buff );
        }
    #endif
//#endif

}

