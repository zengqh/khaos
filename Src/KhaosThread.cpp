#include "KhaosPreHeaders.h"
#include "KhaosThread.h"
#include "KhaosMath.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    Mutex::Mutex()
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        InitializeCriticalSection(&handle_);
#endif
    }

    Mutex::~Mutex()
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        DeleteCriticalSection(&handle_);
#endif
    }

    void Mutex::lock()
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        EnterCriticalSection(&handle_);
#endif
    }

    void Mutex::unlock()
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        LeaveCriticalSection(&handle_);
#endif
    }

    //////////////////////////////////////////////////////////////////////////
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
    unsigned __stdcall thread_callback( void* para )
    {
        static_cast<Thread*>(para)->_thread_callback();
        return 0;
    }
#endif

    Thread::Thread() : handle_(0), fn_(0), para_(0)
    {
    }

    Thread::~Thread()
    {
        join();
    }

    void Thread::bind_func( callback_function fn, void* para )
    {
        fn_ = fn;
        para_ = para;
    }

    void Thread::run()
    {
        if ( handle_ )
            return;

#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        handle_ = (void*)_beginthreadex( 0, 0, thread_callback, this, 0, 0 );
#endif
    }

    void Thread::join()
    {
        if ( handle_ )
        {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
            WaitForSingleObject( (HANDLE)handle_, INFINITE );
            CloseHandle( (HANDLE)handle_ );
            handle_ = 0;
#endif
        }
    }

    void Thread::_thread_callback()
    {
        fn_( para_ );
    }

    //////////////////////////////////////////////////////////////////////////
    Signal::Signal() : event_(0)
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        event_ = CreateEvent( 0, FALSE, FALSE, 0 );
#endif
    }

    Signal::~Signal()
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        CloseHandle( (HANDLE)event_ );
#endif
    }

    void Signal::set()
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        SetEvent( (HANDLE)event_ );
#endif
    }

    void Signal::wait()
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        WaitForSingleObject( (HANDLE)event_, INFINITE );
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    ThreadArray::ThreadArray()
    {
    }

    ThreadArray::~ThreadArray()
    {
        for ( size_t i = 0; i < m_threads.size(); ++i )
        {
            KHAOS_DELETE_T(m_threads[i]);
        }
    }

    void ThreadArray::addThread( Thread::callback_function func, void* para )
    {
        Thread* thread = KHAOS_NEW_T(Thread);
        thread->bind_func( func, para );
        m_threads.push_back( thread );
    }

    void ThreadArray::run()
    {
        for ( size_t i = 0; i < m_threads.size(); ++i )
        {
            m_threads[i]->run();
        }
    }

    void ThreadArray::join()
    {
        for ( size_t i = 0; i < m_threads.size(); ++i )
        {
            m_threads[i]->join();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void BatchTasks::planTask( int taskCnt, int threads )
    {
        // 主线程
        if ( threads <= 0 )
        {
            TaskPara para;

            para.owner = this;
            para.threadID = 0; // id = 0, main thread
            para.low   = 0;
            para.high  = taskCnt;
            para.step  = 1;

            _taskSatic( &para );
            return;
        }

        // 依据线程分配每任务数量
        ThreadArray thrTask;
        vector<TaskPara>::type paras(threads);

        for ( int t = 0; t < threads; ++t )
        {
            TaskPara& para = paras[t];

            para.owner = this;
            para.threadID = t; // [0, threads), work threads
            para.low   = t;
            para.high  = taskCnt;
            para.step  = threads;

            thrTask.addThread( _taskSatic, &para );
        }

        thrTask.run();
        thrTask.join();
    }

    void BatchTasks::_taskSatic( void* para )
    {
        const TaskPara* taskPara = static_cast<TaskPara*>( para );
        BatchTasks* pThis = taskPara->owner;
        int threadID = taskPara->threadID;

        for ( int t = taskPara->low; t < taskPara->high; t += taskPara->step )
        {
            pThis->m_callback( threadID, pThis->m_userData, t );
        }
    }
}

