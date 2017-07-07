#pragma once
#include "KhaosOSHeaders.h"
#include "KhaosNoncopyable.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class Mutex : public Noncopyable
    {
    public:
        Mutex();
        ~Mutex();

    public:
        void lock();
        void unlock();

    private:
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        CRITICAL_SECTION handle_;
#endif
    };

    //////////////////////////////////////////////////////////////////////////
    class LockGuard : public Noncopyable
    {
    public:
        typedef Mutex mutex_type;

    public:
        LockGuard( mutex_type& m ) : m_(m)
        {
            m_.lock();
        }

        ~LockGuard()
        {
            m_.unlock();
        }

    private:
        mutex_type& m_;
    };

    //////////////////////////////////////////////////////////////////////////
    class Thread : public Noncopyable
    {
    public:
        typedef void (*callback_function)(void*);

    public:
        Thread();
        ~Thread();

    public:
        void bind_func( callback_function fn, void* para );
        void run();
        void join();

    public:
        void _thread_callback();

    private:
        void*               handle_;
        callback_function   fn_;
        void*               para_;
    };

    //////////////////////////////////////////////////////////////////////////
    class Signal : public Noncopyable
    {
    public:
        Signal();
        ~Signal();

    public:
        void set();
        void wait();

    private:
        void* event_;
    };

    //////////////////////////////////////////////////////////////////////////
    class ThreadArray : public Noncopyable
    {
        typedef std::vector<Thread*> ThreadList;

    public:
        ThreadArray();
        ~ThreadArray();

    public:
        void addThread( Thread::callback_function func, void* para );
        void run();
        void join();

    private:
        ThreadList m_threads;
    };

    //////////////////////////////////////////////////////////////////////////
    class BatchTasks : public Noncopyable
    {
    public:
        typedef void (*TaskFunc)( int, void*, int );

    private:
        struct TaskPara
        {
            BatchTasks* owner;
            int   threadID;
            int   low;
            int   high;
            int   step;
        };

    public:
        BatchTasks() : m_callback(0), m_userData(0) {}

        void planTask( int taskCnt, int threads );

        void setTask( TaskFunc func ) { m_callback = func; }

        void setUserData( void* ud ) { m_userData = ud; }

    private:
        static void _taskSatic( void* para );

    private:
        TaskFunc m_callback;
        void*    m_userData;
    };
}

#define KHAOS_CLASS_MUTEX   static Khaos::Mutex k_mutex_;
#define KHAOS_OBJECT_MUTEX  Khaos::Mutex k_mutex_;
#define KHAOS_LOCK_MUTEX    Khaos::LockGuard k_lg_(k_mutex_);

