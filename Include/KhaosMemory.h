#pragma once
#include "KhaosThread.h"


// 是否启用内存跟踪
#if KHAOS_DEBUG
    #if KHAOS_MEMORY_TRACKER_DEBUG_MODE
        #define KHAOS_MEMORY_TRACKER 1
    #else
        #define KHAOS_MEMORY_TRACKER 0
    #endif
#else
    #if KHAOS_MEMORY_TRACKER_RELEASE_MODE
        #define KHAOS_MEMORY_TRACKER 1
    #else
        #define KHAOS_MEMORY_TRACKER 0
    #endif
#endif

// 对齐
#define KHAOS_ALIGN_16  __declspec(align(16))

#define KHAOS_MALLOC_ALIGN_16(bytes)    _aligned_malloc(bytes, 16)
#define KHAOS_FREE_ALIGN_16(ptr)        _aligned_free(ptr)

#define KHAOS_NEW_T_ALIGN_16(T)         new (_aligned_malloc(sizeof(T), 16)) T
#define KHAOS_DELETE_T_ALIGN_16(ptr)    { ::Khaos::destruct1(ptr); _aligned_free(ptr); }


// 内存分配释放宏
#if KHAOS_MEMORY_TRACKER
    #define KHAOS_MALLOC(bytes)         ::Khaos::Memory::allocBytes(bytes, __FILE__, __LINE__, __FUNCTION__)
    #define KHAOS_MALLOC_T(T)           static_cast<T*>(::Khaos::Memory::allocBytes(sizeof(T), __FILE__, __LINE__, __FUNCTION__))
    #define KHAOS_MALLOC_ARRAY_T(T, n)  static_cast<T*>(::Khaos::Memory::allocBytes(sizeof(T)*(n), __FILE__, __LINE__, __FUNCTION__))
    #define KHAOS_FREE(ptr)             ::Khaos::Memory::freeBytes(ptr)

    #define KHAOS_NEW_T(T)          new (::Khaos::Memory::allocBytes(sizeof(T), __FILE__, __LINE__, __FUNCTION__)) T
    #define KHAOS_DELETE_T(ptr)     { ::Khaos::destruct1(ptr); ::Khaos::Memory::freeBytes(ptr); }

    #define KHAOS_NEW_ARRAY_T(T, n)         ::Khaos::constructN(static_cast<T*>(::Khaos::Memory::allocBytes(sizeof(T)*(n), __FILE__, __LINE__, __FUNCTION__)), n)
    #define KHAOS_DELETE_ARRAY_T(ptr, n)    { ::Khaos::destructN(ptr, n); ::Khaos::Memory::freeBytes(ptr); }

    #define KHAOS_NEW       new (__FILE__, __LINE__, __FUNCTION__)
    #define KHAOS_DELETE    delete
#else
    #define KHAOS_MALLOC(bytes)         ::Khaos::Memory::allocBytes(bytes)
    #define KHAOS_MALLOC_T(T)           static_cast<T*>(::Khaos::Memory::allocBytes(sizeof(T)))
    #define KHAOS_MALLOC_ARRAY_T(T, n)  static_cast<T*>(::Khaos::Memory::allocBytes(sizeof(T)*(n)))
    #define KHAOS_FREE(ptr)             ::Khaos::Memory::freeBytes(ptr)

    #define KHAOS_NEW_T(T)          new (::Khaos::Memory::allocBytes(sizeof(T))) T
    #define KHAOS_DELETE_T(ptr)     { ::Khaos::destruct1(ptr); ::Khaos::Memory::freeBytes(ptr); }

    #define KHAOS_NEW_ARRAY_T(T, n)         ::Khaos::constructN(static_cast<T*>(::Khaos::Memory::allocBytes(sizeof(T)*(n))), n)
    #define KHAOS_DELETE_ARRAY_T(ptr, n)    { ::Khaos::destructN(ptr, n); ::Khaos::Memory::freeBytes(ptr); }

    #define KHAOS_NEW       new
    #define KHAOS_DELETE    delete
#endif

// 智能指针释放行为
enum SmartPtrFreeMethod
{
    SPFM_FREE,      // 平数据
    SPFM_DELETE_T,  // 普通类
    SPFM_DELETE,    // 继承自AllocatedObject的类
};

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class MemoryTracker
    {
    public:
        KHAOS_OBJECT_MUTEX

    private:
        struct BlockData
        {
            BlockData() : line(0), bytes(0) {}
            BlockData( const char* file_, int line_, const char* func_, int bytes_ ) :
            file(file_), func(func_), line(line_), bytes(bytes_) {}

            std::string file;
            std::string func;
            int         line;
            int         bytes;
        };

        typedef unordered_map_<void*, BlockData> BlockDataMap;

    public:
        MemoryTracker();
        ~MemoryTracker();

        static MemoryTracker& instance();

    public:
        void setReportFile( const std::string& file );
        void report();

    public:
        void registerBytes( void* p, size_t size, const char* file, int line, const char* func );
        void unregisterBytes( void* p );

    public:
        std::string  m_logFile;
        BlockDataMap m_blkDataMap;
        //size_t       m_maxAllocBytes;
        //size_t       m_currAllocBytes;
    };

#define g_memoryTracker MemoryTracker::instance()
#define MemoryTrackerLock__ Khaos::LockGuard mt_lg_(g_memoryTracker.k_mutex_);

    //////////////////////////////////////////////////////////////////////////
    class Memory
    {
    public:
        static void*  allocBytes( size_t size );
        static void*  allocBytes( size_t size, const char* file, int line, const char* func );
        static void   freeBytes( void* p );
        static size_t getMaxAllocBytes();
    };

    //////////////////////////////////////////////////////////////////////////
    class AllocatedObject
    {
    public:
        void* operator new( size_t sz )
        {
            return Memory::allocBytes( sz );
        }

        void* operator new( size_t sz, const char* file, int line, const char* func )
        {
            return Memory::allocBytes( sz, file, line, func );
        }

        void* operator new( size_t, void* ptr )
        {
            return ptr;
        }

        void* operator new[]( size_t sz )
        {
            return Memory::allocBytes( sz );
        }

        void* operator new[]( size_t sz, const char* file, int line, const char* func )
        {
            return Memory::allocBytes( sz, file, line, func );
        }

        void operator delete( void* ptr )
        {
            Memory::freeBytes( ptr );
        }

        void operator delete( void* ptr, void* )
        {
            Memory::freeBytes( ptr );
        }

        void operator delete( void* ptr, const char*, int, const char* )
        {
            Memory::freeBytes( ptr );
        }

        void operator delete[]( void* ptr )
        {
            Memory::freeBytes( ptr );
        }

        void operator delete[]( void* ptr, const char*, int, const char* )
        {
            Memory::freeBytes( ptr );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    struct STLAllocatorBase
    {
        typedef T value_type;
    };

    template<class T>
    struct STLAllocatorBase<const T>
    {
        typedef T value_type;
    };

    template<class T>
    class STLAllocator : public STLAllocatorBase<T>
    {
    public:
        typedef STLAllocatorBase<T>			Base;
        typedef typename Base::value_type	value_type;
        typedef value_type*					pointer;
        typedef const value_type*			const_pointer;
        typedef value_type&					reference;
        typedef const value_type&			const_reference;
        typedef std::size_t					size_type;
        typedef std::ptrdiff_t				difference_type;

        template<typename U>
        struct rebind
        {
            typedef STLAllocator<U> other;
        };

    public:
        STLAllocator() {}
        
        STLAllocator( const STLAllocator& ) {}

        template <class U>
        STLAllocator( const STLAllocator<U>& ) {}
        
        virtual ~STLAllocator(){}

    public:
        pointer allocate( size_type count, typename std::allocator<void>::const_pointer ptr = 0 )
        {
            size_type sz = count * sizeof(T);
            return static_cast<pointer>(Memory::allocBytes(sz)); // stl不跟踪内存泄露
        }

        void deallocate( pointer ptr, size_type )
        {
            Memory::freeBytes(ptr);
        }

        pointer address( reference x ) const
        {
            return &x;
        }

        const_pointer address( const_reference x ) const
        {
            return &x;
        }

        size_type max_size() const throw()
        {
            return Memory::getMaxAllocBytes();
        }

        void construct( pointer p )
        {
            new (p)T;
        }

        void construct( pointer p, const T& val )
        {
            new (p)T(val);
        }

        void destroy( pointer p )
        {
            p->~T();
        }
    };

    template<class T, class T2>
    bool operator==( const STLAllocator<T>&, const STLAllocator<T2>& )
    {
        return true;
    }

    template<class T, class OtherAllocator>
    bool operator==( const STLAllocator<T>&, const OtherAllocator& )
    {
        return false;
    }

    template<class T, class T2>
    bool operator!=( const STLAllocator<T>&, const STLAllocator<T2>& )
    {
        return false;
    }

    template<class T, class OtherAllocator>
    bool operator!=( const STLAllocator<T>&, const OtherAllocator& )
    {
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    T* constructN( T* p, size_t n )
    {
        for ( size_t i = 0; i < n; ++i )
        {
            new (p+i) T();
        }
        return p;
    }

    template<class T>
    void destruct1( T* p )
    {
        if ( p )
        {
            p->~T();
        }
    }

    template<class T>
    void destructN( T* p, size_t n )
    {
        if ( p )
        {
            for ( size_t i = 0; i < n; ++i )
            {
                p[i].~T();
            }
        }
    }
}

