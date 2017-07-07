#include "KhaosPreHeaders.h"
#include "KhaosMemory.h"


#if KHAOS_MEMORY_ALLOCATOR == KHAOS_MEMORY_ALLOCATOR_STD
    #include "KhaosStdAlloc.h"
    #define MemAlloc_ StdAlloc
#elif KHAOS_MEMORY_ALLOCATOR == KHAOS_MEMORY_ALLOCATOR_DEFAULT
    #include "KhaosDefaultAlloc.h"
    #define MemAlloc_ DefaultAlloc
#else
    #define MemAlloc_ UserAlloc
#endif

#define _TEST_MEMORY_CRASH  0

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    std::string _limitStr( const std::string& str, size_t len )
    {
        if ( str.size() <= len )
            return str;    
        return str.substr(0, len);
    }

    //////////////////////////////////////////////////////////////////////////
    MemoryTracker::MemoryTracker() : m_logFile("memory.txt")//, m_maxAllocBytes(0), m_currAllocBytes(0)
    {
    }

    MemoryTracker::~MemoryTracker()
    {
        report();
    }

    MemoryTracker& MemoryTracker::instance()
    {
        static MemoryTracker inst;
        return inst;
    }

    void MemoryTracker::setReportFile( const std::string& file )
    {
        m_logFile = file;
    }

    void MemoryTracker::registerBytes( void* p, size_t size, const char* file, int line, const char* func )
    {
        //KHAOS_LOCK_MUTEX
        bool ret =
            m_blkDataMap.insert( BlockDataMap::value_type(p, BlockData(file, line, func, size)) ).second;
        khaosAssert( ret );
        //m_currAllocBytes += size;
        //if ( m_currAllocBytes > m_maxAllocBytes )
        //    m_maxAllocBytes = m_currAllocBytes;

#if _TEST_MEMORY_CRASH
        char buf[4096] = {};
        sprintf( buf, "new at %x, %d = %s, %s, %d\n", p, size, 
            file, func, line );
        OutputDebugStringA( buf );
#endif
    }

    void MemoryTracker::unregisterBytes( void* p )
    {
        //KHAOS_LOCK_MUTEX
        BlockDataMap::iterator it = m_blkDataMap.find( p );
        if ( it != m_blkDataMap.end() )
        {
#if _TEST_MEMORY_CRASH
            BlockData& bd = it->second;
            char buf[4096] = {};
            sprintf( buf, "free at %x, %d = %s, %s, %d\n", p, bd.bytes, 
                bd.file.c_str(), bd.func.c_str(), bd.line );
            OutputDebugStringA( buf );
#endif
            //m_currAllocBytes -= it->second.bytes;
            m_blkDataMap.erase( it );
        }
        else
        {
            khaosAssert( !p );
        }
    }

    void MemoryTracker::report()
    {
        KHAOS_LOCK_MUTEX
        
        FILE* fp = 0;
        if ( !m_logFile.empty() )
            fp = fopen( m_logFile.c_str(), "w" );

        char buff[4096] = {};
        //sprintf( buff, "Memory MaxAllocBytes: %d\n", m_maxAllocBytes );

        //if ( fp )
        //    fputs( buff, fp );
        //khaosOutputStr( buff );

        for ( BlockDataMap::const_iterator it = m_blkDataMap.begin(), ite = m_blkDataMap.end(); it != ite; ++it )
        {
            void* ptr = it->first;
            const BlockData& bd = it->second;
            if ( bd.line < 0 )
                continue;

            sprintf( buff, "[0x%X] file:%s line:%d @%s -- %d bytes leak!\n", 
                ptr, _limitStr(bd.file, 1024).c_str(), bd.line, _limitStr(bd.func, 1024).c_str(), bd.bytes );

            if ( fp )
                fputs( buff, fp );
            khaosOutputStr( buff );
        }

        if ( fp )
            fclose(fp);
    }

    //////////////////////////////////////////////////////////////////////////
    void* Memory::allocBytes( size_t size )
    {
        
#if KHAOS_MEMORY_TRACKER
        MemoryTrackerLock__ // 放到这里锁定，要原子操作
        void* p = MemAlloc_::allocBytes( size );
        g_memoryTracker.registerBytes( p, size, "", -1, "" );
#else
        void* p = MemAlloc_::allocBytes( size );
#endif
        return p;
    }

    void* Memory::allocBytes( size_t size, const char* file, int line, const char* func )
    {
        
#if KHAOS_MEMORY_TRACKER
        MemoryTrackerLock__ // 放到这里锁定，要原子操作
        void* p = MemAlloc_::allocBytes( size );
        g_memoryTracker.registerBytes( p, size, file, line, func );
#else
        void* p = MemAlloc_::allocBytes( size );
#endif
        return p;
    }

    void Memory::freeBytes( void* p )
    {
        
#if KHAOS_MEMORY_TRACKER
        MemoryTrackerLock__ // 放到这里锁定，要原子操作
        MemAlloc_::freeBytes( p );
        g_memoryTracker.unregisterBytes( p );
#else
        MemAlloc_::freeBytes( p );
#endif
    }

    size_t Memory::getMaxAllocBytes()
    {
#ifdef max
#undef max
#endif
        return std::numeric_limits<size_t>::max();
    }
}

