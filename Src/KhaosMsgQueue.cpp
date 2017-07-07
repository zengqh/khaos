#include "KhaosPreHeaders.h"
#include "KhaosMsgQueue.h"
#include "KhaosMsgQueueIds.h"
#include "KhaosTimer.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void MsgQueueBase::_postMessageNoLock( uint32 msg, void* lparam, void* wparam, const String* sparam )
    {
        // ������Ϣ�����������
        Message* m = m_msgPool.requestObject();

        m->msg = msg;
        if ( sparam )
            m->sparam = *sparam;

        m_listener->onSetMsgUserData( this, m, lparam, wparam );

        // ���뵽��Ϣ����
        m_msgList.push_back(m);
    }

    void MsgQueueBase::_clearMessageListNoLock()
    {
        for ( MsgList::iterator it = m_msgList.begin(), ite = m_msgList.end(); it != ite; ++it )
        {
            Message* msg = *it;

            // δִ�е������ȳ���
            m_listener->onCancelMsg( this, msg );

            // δִ�е�����Ҫ�����
            m_listener->onClearMsg( this, msg );

            // ����
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

        // ֪ͨ�����߳�����Ϣ
        m_signal.set();
    }

    void WorkMsgQueue::_onWorkThread()
    {
        Message* msgCache[MAX_MSG_CACHE] = {};
        int cmdCnt = 0;

        while ( m_run )
        {
            // �ϴ�û���κ�����ȴ�����
            if ( cmdCnt == 0 )
                m_signal.wait();

            // ���Ҫͬ������
            {
                KHAOS_LOCK_MUTEX
                // ����֮ǰ������
                for ( int i = 0; i < cmdCnt; ++i )
                {
                    m_msgPool.freeObject( msgCache[i] ); 
                }

                // ȡһ�����������
                cmdCnt = 0;
                while ( (cmdCnt < MAX_MSG_CACHE) && m_msgList.size() )
                {
                    msgCache[cmdCnt] = m_msgList.front();
                    m_msgList.pop_front();
                    ++cmdCnt;
                }
            }

            // ִ������
            for ( int i = 0; i < cmdCnt; ++i )
            {
                Message* m = msgCache[i];
                m_listener->onWorkImpl( this, m );
                m_listener->onClearMsg( this, m );
                
            }
        }

        // �������һ�ε�����
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
        // ��֡��ʹ�õ�ʱ��Ƭ��������
        m_deadlineTime = g_timerSystem.getCurImmTime() + timePiece;

        KHAOS_LOCK_MUTEX
        // ����Ϣ����ʱ��Ƭ����
        while ( m_msgList.size() && hasTimePiece() )
        {
            // ȡ��һ����Ϣ
            Message* msg = m_msgList.front();
            m_msgList.pop_front();

            // ����ִ��
            m_listener->onWorkImpl( this, msg );
            m_listener->onClearMsg( this, msg );

            // �ӻ�������
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

        // һ��ui�߳�
        m_uiQueue.setId( MQQUEUE_UI );
        m_uiQueue.setListener( this );

        // Ĭ��ע��һ��high�������߳�
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
        // ע��һ�������߳�
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
        // ע��һ�������̵߳�һ����Ϣ����
        khaosAssert( !m_run );
        WorkQueueMap::iterator it = m_workQueueMap.find(queueId);
        khaosAssert( it != m_workQueueMap.end() );
        it->second->listeners.insert( ListenerMap::value_type( msg, listener ) );
    }

    void MsgQueueManager::registerUIMsg( uint32 msg, IMsgQueueListener* listener )
    {
        // ע��һ��ui��Ϣ����
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

