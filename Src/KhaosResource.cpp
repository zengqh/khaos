#include "KhaosPreHeaders.h"
#include "KhaosResource.h"
#include "KhaosResourceManager.h"


namespace Khaos
{
    Resource::~Resource()
    {
        // 这里不调用_destructResImpl()，一律子类调用

        if ( m_resCreator )
            m_resCreator->release();

        if ( m_loadAsync )
            KHAOS_FREE(m_loadAsync);
    }

    String Resource::getResFileName() const
    {
        return m_group->getAutoCreator()->getLocateFileName( m_name );
    }

    String Resource::getResFullFileName() const
    {
        return g_fileSystem->getFullFileName( getResFileName() );
    }

    void Resource::setResCreator( IResourceCreator* creator )
    {
        if ( m_resCreator )
            m_resCreator->release();
        m_resCreator = creator; 
    }

    void Resource::setNullCreator( bool loadNow )
    {
        static ResourceNullCreator s_creator;
        setResCreator( &s_creator );

        if ( loadNow )
            load( false );
    }

    void Resource::addListener( IResourceListener* listener )
    {
        if ( m_listeners.insert( listener ).second )
        {
            if ( m_status == S_LOADED ) // 资源已经加载了，告诉这个新来的
                listener->onResourceInitialized( this );
        }
    }

    void Resource::removeListener( IResourceListener* listener )
    {
        m_listeners.erase( listener );
    }

    void Resource::setLoadAsync( bool b )
    {
        if ( !m_loadAsync )
            m_loadAsync = KHAOS_MALLOC_T(bool);
        *m_loadAsync = b;
    }

    void Resource::clearLoadAsync()
    {
        if ( m_loadAsync )
        {
            KHAOS_FREE(m_loadAsync);
            m_loadAsync = 0;
        }
    }

    void Resource::load( bool async )
    {
        g_resourceManager->_load( this, async );
    }

    void Resource::unload( bool async )
    {
        g_resourceManager->_unload( this, async ); 
    }

    bool Resource::checkLoad()
    {
        // 如果加载已经成功直接返回
        if ( m_status == S_LOADED )
            return true;

        g_resourceManager->_checkLoad( this );
        return false;
    }

    void Resource::_notifyUpdate()
    {
        // 通知资源有更新
        khaosAssert( m_status == S_LOADED );
        for ( ListenerList::iterator it = m_listeners.begin(), ite = m_listeners.end(); it != ite; ++it )
            (*it)->onResourceUpdate( this );
    }

    bool Resource::setLoadingStatus()
    {
        // 设置加载中状态
        // unloaded => loading
        if ( m_status == S_UNLOADED )
        {
            m_status = S_LOADING;
            return true;
        }

        return false;
    }

    void Resource::setPreparedStatus( bool ok )
    {
        // 设置当前状态为准备
        // loading => prepared ok
        // loading => prepared failed
        if ( m_status == S_LOADING )
        {
            m_status = ok ? S_PREPARED_OK : S_PREPARED_FAILED;
        }
    }

    void Resource::setCompletedStatus( bool ok )
    { 
        // 设置最后完成状态
        // prepared ok => loaded
        // prepared ok => badres
        // prepared failed => badres

        if ( m_status != S_PREPARED_OK && m_status != S_PREPARED_FAILED )
            return;

        State finalState = S_BADRES;
        if ( (m_status == S_PREPARED_OK) && ok ) // loaded唯一条件是prepare ok且最后也ok
            finalState = S_LOADED;

        // 设置最后的状态
        m_status = finalState;

        if ( finalState == S_LOADED )
        {
            for ( ListenerList::iterator it = m_listeners.begin(), ite = m_listeners.end(); it != ite; ++it )
                (*it)->onResourceLoaded( this );
        }
        else
        {
            for ( ListenerList::iterator it = m_listeners.begin(), ite = m_listeners.end(); it != ite; ++it )
                (*it)->onResourceBad( this );
        }
    }

    bool Resource::setUnloadingStatus()
    {
        // 设置卸载中状态
        // loaded => unloading
        // badres => unloading
        if ( m_status == S_LOADED || m_status == S_BADRES )
        {
            m_status = S_UNLOADING;
            return true; 
        }

        return false;
    }

    void Resource::setUnloadedStatus()
    {
        // 设置未加载状态
        // unloading => unloaded
        if ( m_status == S_UNLOADING )
        {
            m_status = S_UNLOADED;
        }
    }

    void Resource::release()
    {
        --m_refCount;

        if ( m_refCount == ResourceManager::SYSTEM_REFERENCE_COUNT )
        {
            g_resourceManager->_releaseRes( this );
        }
    }

    void Resource::_deleteSelf()
    {
        KHAOS_DELETE this;
    }
}

