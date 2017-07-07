#include "KhaosPreHeaders.h"
#include "KhaosMsgQueue.h"
#include "KhaosMsgQueueIds.h"
#include "KhaosTimer.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void MsgQueueBase::_postMessageNoLock( uint32 msg, void* lparam, void* wparam, const String* sparam )
    {
        // 请求消息对象并填充数据
        Message* m = m_msgPool.requestObject();

        m->msg = msg;
        if ( sparam )
            m->sparam = *sparam;

        m_listener->onSetMsgUserData( this, m, lparam, wparam );

        // 加入到消息队列
        m_msgList.push_back(m);
    }

    void MsgQueueBase::_clearMessageListNoLock()
    {
        for ( MsgList::iterator it = m_msgList.begin(), ite = m_msgList.end(); it != ite; ++it )
        {
            Message* msg = *it;

            // 未执行的命令先撤销
            m_listener->onCancelMsg( this, msg );

            // 未执行的命令要求清除
            m_listener->onClearMsg( this, msg );

            // 返还
            m_msgPool.freeObject( msg );
        }

        m_msgList.clear();
        m_msgPool.clearPool();
    }

    //////////////////////////////////////////////////////////////////////////
    void WorkMsgQueue::start()
    {
        if ( !m_run )
        {
            m_run = true;
            m_thread.run();
        }
    }

    void WorkMsgQueue::shutdown()
    {
        if ( m_run )
        {
            m_run = false;
            m_signal.set();
            m_thread.join();

            KHAOS_LOCK_MUTEX
            _clearMessageListNoLock();
        }
    }

    void WorkMsgQueue::postMessage( uint32 msg, void* lparam, void* wparam, const String* sparam )
    {
        khaosAssert( m_run );

        {
            KHAOS_LOCK_MUTEX
            _postMessageNoLock( msg, lparam, wparam, sparam );
        }

        // 通知工作线程有消息
        m_signal.set();
    }

    void WorkMsgQueue::_onWorkThread()
    {
        Message* msgCache[MAX_MSG_CACHE] = {};
        int cmdCnt = 0;

        while ( m_run )
        {
            // 上次没有任何命令，等待接收
            if ( cmdCnt == 0 )
                m_signal.wait();

            // 这个要同步工作
            {
                KHAOS_LOCK_MUTEX
                // 回收之前的命令
                for ( int i = 0; i < cmdCnt; ++i )
                {
                    m_msgPool.freeObject( msgCache[i] ); 
                }

                // 取一批新命令出来
                cmdCnt = 0;
                while ( (cmdCnt < MAX_MSG_CACHE) && m_msgList.size() )
                {
                    msgCache[cmdCnt] = m_msgList.front();
                    m_msgList.pop_front();
                    ++cmdCnt;
                }
            }

            // 执行命令
            for ( int i = 0; i < cmdCnt; ++i )
            {
                Message* m = msgCache[i];
                m_listener->onWorkImpl( this, m );
                m_listener->onClearMsg( this, m );
                
            }
        }

        // 回收最后一次的命令
        KHAOS_LOCK_MUTEX
        for ( int i = 0; i < cmdCnt; ++i )
        {
            m_msgPool.freeObject( msgCache[i] ); 
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void UIMsgQueue::start()
    {
        m_run = true;
    }

    void UIMsgQueue::shutdown()
    {
        if ( m_run )
        {
            m_run = false;
            KHAOS_LOCK_MUTEX
            _clearMessageListNoLock();
        }
    }

    void UIMsgQueue::postMessage( uint32 msg, void* lparam, void* wparam, const String* sparam )
    {
        khaosAssert( m_run );
        KHAOS_LOCK_MUTEX
        _postMessageNoLock( msg, lparam, wparam, sparam );
    }

    void UIMsgQueue::update( float timePiece )
    {
        // 本帧能使用的时间片到点期限
        m_deadlineTime = g_timerSystem.getCurImmTime() + timePiece;

        KHAOS_LOCK_MUTEX
        // 有消息且有时间片空余
        while ( m_msgList.size() && hasTimePiece() )
        {
            // 取出一个消息
            Message* msg = m_msgList.front();
            m_msgList.pop_front();

            // 交付执行
            m_listener->onWorkImpl( this, msg );
            m_listener->onClearMsg( this, msg );

            // 扔回垃圾堆
            m_msgPool.freeObject( msg );
        }
    }

    bool UIMsgQueue::hasTimePiece() const
    {
        return g_timerSystem.getCurImmTime() < m_deadlineTime;
    }

    float UIMsgQueue::getTimePieceLeft() const
    {
        return m_deadlineTime - g_timerSystem.getCurImmTime();
    }

    //////////////////////////////////////////////////////////////////////////
    MsgQueueManager* g_msgQueueManager = 0;

    MsgQueueManager::MsgQueueManager() : m_run(false)
    {
        khaosAssert( !g_msgQueueManager );
        g_msgQueueManager = this;

        // 一个ui线程
        m_uiQueue.setId( MQQUEUE_UI );
        m_uiQueue.setListener( this );

        // 默认注册一个high级别工作线程
        registerWorkQueue( MQQUEUE_HIGH );
    }

    MsgQueueManager::~MsgQueueManager()
    {
        shutdown();

        for ( WorkQueueMap::iterator it = m_workQueueMap.begin(), ite = m_workQueueMap.end(); it != ite; ++it )
        {
            WorkQueue* wq = it->second;
            KHAOS_DELETE wq;
        }

        g_msgQueueManager = 0;
    }

    void MsgQueueManager::registerWorkQueue( uint queueId )
    {
        // 注册一个工作线程
        khaosAssert( !m_run );
        khaosAssert( m_workQueueMap.find(queueId) == m_workQueueMap.end() );
        WorkQueue* wq = KHAOS_NEW WorkQueue;
        wq->queue.setId( queueId );
        wq->queue.setUserData( wq );
        wq->queue.setListener( this );
        m_workQueueMap.insert( WorkQueueMap::value_type(queueId, wq) );
    }

    void MsgQueueManager::registerWorkMsg( uint queueId, uint32 msg, IMsgQueueListener* listener )
    {
        // 注册一个工作线程的一个消息监听
        khaosAssert( !m_run );
        WorkQueueMap::iterator it = m_workQueueMap.find(queueId);
        khaosAssert( it != m_workQueueMap.end() );
        it->second->listeners.insert( ListenerMap::value_type( msg, listener ) );
    }

    void MsgQueueManager::registerUIMsg( uint32 msg, IMsgQueueListener* listener )
    {
        // 注册一个ui消息监听
        khaosAssert( !m_run );
        khaosAssert( m_uiListeners.find(msg) == m_uiListeners.end() );
        m_uiListeners.insert( ListenerMap::value_type( msg, listener ) );
    }

    void MsgQueueManager::start()
    {
        m_uiQueue.start();

        for ( WorkQueueMap::iterator it = m_workQueueMap.begin(), ite = m_workQueueMap.end(); it != ite; ++it )
        {
            it->second->queue.start();
        }

        m_run = true;
    }

    void MsgQueueManager::shutdown()
    {
        for ( WorkQueueMap::iterator it = m_workQueueMap.begin(), ite = m_workQueueMap.end(); it != ite; ++it )
        {
            it->second->queue.shutdown();
        }

        m_uiQueue.shutdown();

        m_run = false;
    }

    void MsgQueueManager::onSetMsgUserData( MsgQueueBase* mq, Message* msg, void* lparam, void* wparam )
    {
        if ( KHAOS_OBJECT_IS(mq, WorkMsgQueue) ) // work
        {
            WorkQueue* wq = (WorkQueue*)static_cast<WorkMsgQueue*>(mq)->getUserData();
            ListenerMap::iterator it = wq->listeners.find( msg->msg );
            khaosAssert( it != wq->listeners.end() );
            it->second->onSetMsgUserData( mq, msg, lparam, wparam );
        }
        else // ui
        {
            khaosAssert( KHAOS_OBJECT_IS(mq, UIMsgQueue) );
            ListenerMap::iterator it = m_uiListeners.find( msg->msg );
            khaosAssert( it != m_uiListeners.end() );
            it->second->onSetMsgUserData( mq, msg, lparam, wparam );
        }
    }

    void MsgQueueManager::onWorkImpl( MsgQueueBase* mq, Message* msg )
    {
        if ( KHAOS_OBJECT_IS(mq, WorkMsgQueue) ) // work
        {
            WorkQueue* wq = (WorkQueue*)static_cast<WorkMsgQueue*>(mq)->getUserData();
            ListenerMap::iterator it = wq->listeners.find( msg->msg );
            khaosAssert( it != wq->listeners.end() );
            it->second->onWorkImpl( mq, msg );
        }
        else // ui
        {
            khaosAssert( KHAOS_OBJECT_IS(mq, UIMsgQueue) );
            ListenerMap::iterator it = m_uiListeners.find( msg->msg );
            khaosAssert( it != m_uiListeners.end() );
            it->second->onWorkImpl( mq, msg );
        }
    }

    void MsgQueueManager::onCancelMsg( MsgQueueBase* mq, Message* msg )
    {
        if ( KHAOS_OBJECT_IS(mq, WorkMsgQueue) ) // work
        {
            WorkQueue* wq = (WorkQueue*)static_cast<WorkMsgQueue*>(mq)->getUserData();
            ListenerMap::iterator it = wq->listeners.find( msg->msg );
            khaosAssert( it != wq->listeners.end() );
            it->second->onCancelMsg( mq, msg );
        }
        else // ui
        {
            khaosAssert( KHAOS_OBJECT_IS(mq, UIMsgQueue) );
            ListenerMap::iterator it = m_uiListeners.find( msg->msg );
            khaosAssert( it != m_uiListeners.end() );
            it->second->onCancelMsg( mq, msg );
        }
    }

    void MsgQueueManager::onClearMsg( MsgQueueBase* mq, Message* msg )
    {
        if ( KHAOS_OBJECT_IS(mq, WorkMsgQueue) ) // work
        {
            WorkQueue* wq = (WorkQueue*)static_cast<WorkMsgQueue*>(mq)->getUserData();
            ListenerMap::iterator it = wq->listeners.find( msg->msg );
            khaosAssert( it != wq->listeners.end() );
            it->second->onClearMsg( mq, msg );
        }
        else // ui
        {
            khaosAssert( KHAOS_OBJECT_IS(mq, UIMsgQueue) );
            ListenerMap::iterator it = m_uiListeners.find( msg->msg );
            khaosAssert( it != m_uiListeners.end() );
            it->second->onClearMsg( mq, msg );
        }
    }

    void MsgQueueManager::update( float timePiece )
    {
        if ( m_run )
            m_uiQueue.update( timePiece );
    }

    void MsgQueueManager::postWorkMessage( uint queueId, uint32 msg, void* lparam, void* wparam, const String* sparam )
    {
        khaosAssert( m_run );
        WorkQueueMap::iterator it = m_workQueueMap.find(queueId);
        khaosAssert( it != m_workQueueMap.end() );
        it->second->queue.postMessage( msg, lparam, wparam, sparam );
    }

    void MsgQueueManager::postUIMessage( uint32 msg, void* lparam, void* wparam, const String* sparam )
    {
        khaosAssert( m_run );
        m_uiQueue.postMessage( msg, lparam, wparam, sparam );
    }
}

