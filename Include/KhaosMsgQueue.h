#pragma once
#include "KhaosThread.h"
#include "KhaosObjectRecycle.h"
#include "KhaosRTTI.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    // 消息结构和事件
    struct Message : public AllocatedObject
    {
        Message() : lparam(0), wparam(0), msg(0) {}
        void*   lparam;
        void*   wparam;
        String  sparam;
        uint32  msg;
    };

    class MsgQueueBase;
    struct IMsgQueueListener
    {
        virtual void onSetMsgUserData( MsgQueueBase* mq, Message* msg, void* lparam, void* wparam )
        {
            msg->lparam = lparam;
            msg->wparam = wparam;
        }

        virtual void onWorkImpl( MsgQueueBase* mq, Message* msg ) = 0;
        virtual void onCancelMsg( MsgQueueBase* mq, Message* msg ) {}
        virtual void onClearMsg( MsgQueueBase* mq, Message* msg ) {}
    };

    //////////////////////////////////////////////////////////////////////////
    // 消息队列基本
    class MsgQueueBase : public AllocatedObject
    {
        KHAOS_DECLARE_RTTI(MsgQueueBase)

    protected:
        KHAOS_OBJECT_MUTEX

    public:
        typedef ObjectRecycle<Message>  MsgPool;
        typedef list<Message*>::type    MsgList;

    public:
        MsgQueueBase() : m_listener(0), m_id(0) {}
        virtual ~MsgQueueBase() {}

    public:
        void setListener( IMsgQueueListener* l ) { m_listener = l; }

        void setId( uint id ) { m_id = id; }
        uint getId() const { return m_id; }

    protected:
        void _postMessageNoLock( uint32 msg, void* lparam, void* wparam, const String* sparam );
        void _clearMessageListNoLock();

    protected:
        MsgPool             m_msgPool;
        MsgList             m_msgList;
        IMsgQueueListener*  m_listener;
        uint                m_id;
    };

    //////////////////////////////////////////////////////////////////////////
    // 工作线程消息队列
    class WorkMsgQueue : public MsgQueueBase
    {
        KHAOS_DECLARE_RTTI(WorkMsgQueue)

    protected:
        static const int MAX_MSG_CACHE = 3;

    public:
        WorkMsgQueue() : m_userData(0), m_run(false)
        {
            m_thread.bind_func( _workThread, this );
        }

        virtual ~WorkMsgQueue()
        {
            shutdown();
        }

        void  setUserData( void* userData ) { m_userData = userData; }
        void* getUserData() const { return m_userData; }

        void start();
        void shutdown();
        void postMessage( uint32 msg, void* lparam, void* wparam, const String* sparam );

    protected:
        void _onWorkThread();

        static void _workThread( void* para )
        {
            static_cast<WorkMsgQueue*>(para)->_onWorkThread();
        }

    protected:
        Thread          m_thread;
        Signal          m_signal;
        void*           m_userData;
        volatile bool   m_run;
    };

    //////////////////////////////////////////////////////////////////////////
    // ui消息队列
    class UIMsgQueue : public MsgQueueBase
    {
        KHAOS_DECLARE_RTTI(UIMsgQueue)

    public:
        UIMsgQueue() : m_deadlineTime(0), m_run(false) {}
        virtual ~UIMsgQueue() { shutdown(); }

    public:
        void start();
        void shutdown();

        void postMessage( uint32 msg, void* lparam, void* wparam, const String* sparam );
        void update( float timePiece );

        bool  hasTimePiece() const;
        float getTimePieceLeft() const;
 
    protected:
        float m_deadlineTime;
        bool  m_run;
    };

    //////////////////////////////////////////////////////////////////////////
    // 消息队列管理
    class MsgQueueManager : public AllocatedObject, public IMsgQueueListener
    {
    private:
        typedef map<uint32, IMsgQueueListener*>::type ListenerMap;
        
        struct WorkQueue : public AllocatedObject
        {
            WorkMsgQueue    queue;
            ListenerMap     listeners;
        };

        typedef map<uint, WorkQueue*>::type WorkQueueMap;

    public:
        MsgQueueManager();
        ~MsgQueueManager();

    public:
        void registerWorkQueue( uint queueId );
        void registerWorkMsg( uint queueId, uint32 msg, IMsgQueueListener* listener );
        void registerUIMsg( uint32 msg, IMsgQueueListener* listener );

        void start();
        void shutdown();
        void update( float timePiece );

        void postWorkMessage( uint queueId, uint32 msg, void* lparam, void* wparam, const String* sparam );
        void postUIMessage( uint32 msg, void* lparam, void* wparam, const String* sparam );

    private:
        virtual void onSetMsgUserData( MsgQueueBase* mq, Message* msg, void* lparam, void* wparam );
        virtual void onWorkImpl( MsgQueueBase* mq, Message* msg );
        virtual void onCancelMsg( MsgQueueBase* mq, Message* msg );
        virtual void onClearMsg( MsgQueueBase* mq, Message* msg );

    private:
        // 1个ui线程
        UIMsgQueue   m_uiQueue;
        ListenerMap  m_uiListeners;

        // n个工作线程
        WorkQueueMap m_workQueueMap;

        // 状态
        bool m_run;
    };

    extern MsgQueueManager* g_msgQueueManager;
}

