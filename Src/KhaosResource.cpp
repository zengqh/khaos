#include "KhaosPreHeaders.h"
#include "KhaosResource.h"
#include "KhaosResourceManager.h"


namespace Khaos
{
    Resource::~Resource()
    {
        // ���ﲻ����_destructResImpl()��һ���������

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
            if ( m_status == S_LOADED ) // ��Դ�Ѿ������ˣ��������������
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
        // ��������Ѿ��ɹ�ֱ�ӷ���
        if ( m_status == S_LOADED )
            return true;

        g_resourceManager->_checkLoad( this );
        return false;
    }

    void Resource::_notifyUpdate()
    {
        // ֪ͨ��Դ�и���
        khaosAssert( m_status == S_LOADED );
        for ( ListenerList::iterator it = m_listeners.begin(), ite = m_listeners.end(); it != ite; ++it )
            (*it)->onResourceUpdate( this );
    }

    bool Resource::setLoadingStatus()
    {
        // ���ü�����״̬
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
        // ���õ�ǰ״̬Ϊ׼��
        // loading => prepared ok
        // loading => prepared failed
        if ( m_status == S_LOADING )
        {
            m_status = ok ? S_PREPARED_OK : S_PREPARED_FAILED;
        }
    }

    void Resource::setCompletedStatus( bool ok )
    { 
        // ����������״̬
        // prepared ok => loaded
        // prepared ok => badres
        // prepared failed => badres

        if ( m_status != S_PREPARED_OK && m_status != S_PREPARED_FAILED )
            return;

        State finalState = S_BADRES;
        if ( (m_status == S_PREPARED_OK) && ok ) // loadedΨһ������prepare ok�����Ҳok
            finalState = S_LOADED;

        // ��������״̬
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
        // ����ж����״̬
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
        // ����δ����״̬
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

