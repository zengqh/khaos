#include "KhaosPreHeaders.h"
#include "KhaosResourceManager.h"
#include "KhaosFileSystem.h"
#include "KhaosLogger.h"
#include "KhaosMsgQueueIds.h"
#include "KhaosResourceMQIds.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void ResAutoCreatorBase::release()
    {
        KHAOS_DELETE this;
    }

    bool ResAutoCreatorBase::prepareResource( Resource* res, void* context )
    {
        DataBuffer* buff = (DataBuffer*)context;
        return _prepareResourceImpl( res, *buff );
    }

    String ResAutoCreatorBase::getLocateFileName( const String& name ) const
    {
        if ( name.size() && (name[0] == '/' || name[0] == '\\') )
            return  name;

        return m_basePath + name; 
    }

    //////////////////////////////////////////////////////////////////////////
    DefaultResourceScheduler* g_defaultResourceScheduler = 0;

    DefaultResourceScheduler::DefaultResourceScheduler() : m_checkLoadAsync(false)
    {
        khaosAssert( !g_defaultResourceScheduler );
        g_defaultResourceScheduler = this;

        // ע���Լ���Ҫ����Ϣ
        g_msgQueueManager->registerUIMsg( MQMSG_UI_BUILD_RESOURCE, this );

        g_msgQueueManager->registerWorkMsg( MQQUEUE_HIGH, MQMSG_CREATE_MANUAL_RESOURCE, this );
        g_msgQueueManager->registerUIMsg( MQMSG_UI_BUILD_MANUAL_RESOURCE, this );

        g_msgQueueManager->registerUIMsg( MQMSG_UI_DESTRUCT_RESOURCE, this );
        g_msgQueueManager->registerUIMsg( MQMSG_UI_RELEASE_RESOURCE, this );
    }

    DefaultResourceScheduler::~DefaultResourceScheduler()
    {
        g_defaultResourceScheduler = 0;
    }

    void DefaultResourceScheduler::release()
    {
    }

    void DefaultResourceScheduler::load( Resource* res, bool async )
    {
        // ���ɼ���״̬
        if ( !res->setLoadingStatus() )
            return;

        // ���ļ�ϵͳ������Դ����
        if ( async ) // �첽����
        {
            res->addRef(); // ��������

            IResourceCreator* creator = res->getResCreator();
            if ( creator ) // �˹���������
            {
                g_msgQueueManager->postWorkMessage( MQQUEUE_HIGH, MQMSG_CREATE_MANUAL_RESOURCE, res, 0, 0 );
            }
            else // �ļ��Զ���������
            {
                String fileName = res->_getGroup()->getAutoCreator()->getLocateFileName( res->getName() );
                g_fileSystem->getAsyncFileData( fileName, res );
            }
        }
        else // ͬ�����󣨱ض���ui�̣߳�
        {
            IResourceCreator* creator = res->getResCreator();

            if ( creator ) // �˹���������
            {
                bool workOk = creator->prepareResource( res, 0 );
                res->setPreparedStatus( workOk );

                if ( workOk )
                    workOk = creator->buildResource( res );
                res->setCompletedStatus( workOk );
            }
            else  // �ļ��Զ���������
            {
                ResAutoCreatorBase* creator = res->_getGroup()->getAutoCreator();

                // ��λ
                bool workOk = false;
                String fileName = creator->getLocateFileName( res->getName() );

                // ��ȡ�ļ����ݲ�׼��
                DataBuffer db;
                if ( g_fileSystem->getFileData( fileName, db ) )
                    workOk = creator->prepareResource( res, &db );
                res->setPreparedStatus( workOk );

                // ��󴴽�����
                if ( workOk )
                    workOk = creator->buildResource( res );
                res->setCompletedStatus( workOk );
            }
        }
    }

    void DefaultResourceScheduler::unload( Resource* res, bool async )
    {
        // ����ж��״̬
        if ( !res->setUnloadingStatus() )
            return;

        if ( async ) // �첽��ui�̴߳���
        {
            res->addRef(); // ����1������
            g_msgQueueManager->postUIMessage( MQMSG_UI_DESTRUCT_RESOURCE, res, 0, 0 );
        }
        else
        {
            // ͬ��������Դ
            res->_destructResImpl();

            // ����ж��״̬
            res->setUnloadedStatus();
        }
    }

    void DefaultResourceScheduler::checkLoad( Resource* res )
    {
        // Ĭ��ִ�м���
        bool needAsync = m_checkLoadAsync;
        if ( bool* async = res->_getLoadAsync() )
            needAsync = *async;

        load( res, needAsync );
    }

    void DefaultResourceScheduler::releaseRes( Resource* res )
    {
        bool needAsync = m_checkLoadAsync;
        if ( bool* async = res->_getLoadAsync() )
            needAsync = *async;

        if ( needAsync )
        {
            // �첽��ui�̴߳����ͷ�
            g_msgQueueManager->postUIMessage( MQMSG_UI_RELEASE_RESOURCE, res, 0, 0 );
        }
        else
        {
            res->_deleteSelf();
        }
    }

    void DefaultResourceScheduler::onGetFileResult( const String& fileName, void* userData, DataBuffer& buff, IFileSystem::GetResult result )
    {
        // �첽��������õ����ļ�֪ͨ
        // ����Դԭʼ����������֮�ദ��
        Resource* res = static_cast<Resource*>(userData);
        bool ok = false;

        if ( result == IFileSystem::GR_OK )
            ok = res->_getGroup()->getAutoCreator()->prepareResource( res, &buff );
        res->setPreparedStatus( ok );

        // ֪ͨui�߳���ɴ���
        g_msgQueueManager->postUIMessage( MQMSG_UI_BUILD_RESOURCE, res, 0, 0 );
    }

    void DefaultResourceScheduler::onWorkImpl( MsgQueueBase* mq, Message* msg )
    {
        Resource* res = static_cast<Resource*>(msg->lparam);

        switch ( msg->msg )
        {
        case MQMSG_CREATE_MANUAL_RESOURCE: // �첽�߳�׼���˹���Դ
            {
                bool ok = res->getResCreator()->prepareResource( res, 0 );
                res->setPreparedStatus( ok );
                g_msgQueueManager->postUIMessage( MQMSG_UI_BUILD_MANUAL_RESOURCE, res, 0, 0 );
            }
            break;

        case MQMSG_UI_BUILD_MANUAL_RESOURCE: // ui�߳���ɴ����˹���Դ
            {
                bool ok = false;
                if ( res->getStatus() == Resource::S_PREPARED_OK )
                    ok = res->getResCreator()->buildResource( res );
                res->setCompletedStatus( ok );
                res->release(); // ����1������
            }
            break;

        case MQMSG_UI_BUILD_RESOURCE: // ��ui�߳���ɴ����Զ���Դ
            {
                bool ok = false;
                if ( res->getStatus() == Resource::S_PREPARED_OK )
                    ok = res->_getGroup()->getAutoCreator()->buildResource( res );
                res->setCompletedStatus( ok );
                res->release(); // ����1������
            }
            break;

        case MQMSG_UI_DESTRUCT_RESOURCE: // ��ui�߳����ж��
            {
                res->_destructResImpl();
                res->setUnloadedStatus();
                res->release(); // ����1������
            }
            break;

        case MQMSG_UI_RELEASE_RESOURCE: // ��ui�߳�ɾ��
            {
                res->_deleteSelf();
            }
            break;

        default:
            khaosAssert(0);
            break;
        }
    }

    void DefaultResourceScheduler::onCancelGetFile( const String& fileName, void* userData )
    {
        // ������δ����������жϣ��ر�Ӧ�ó����ʱ��
    }

    void DefaultResourceScheduler::onCancelMsg( MsgQueueBase* mq, Message* msg )
    {
        // ������δ����������жϣ��ر�Ӧ�ó����ʱ��
    }


    //////////////////////////////////////////////////////////////////////////
    ResourceGroup::~ResourceGroup()
    {
        clearResources();
        setScheduler(0);
        setAutoCreator(0);
    }

    void ResourceGroup::setCreateFunc( CreateResourceFunc func )
    {
        khaosAssert( func );
        m_createFunc = func;
    }

    void ResourceGroup::setAutoCreator( ResAutoCreatorBase* creator )
    {
        if ( m_autoCreator )
            m_autoCreator->release();

        m_autoCreator = creator;
    }

    void ResourceGroup::setScheduler( IResourceScheduler* scheduler )
    {
        if ( m_scheduler )
            m_scheduler->release();

        m_scheduler = scheduler;
    }

    void ResourceGroup::clearResources()
    {
        KHAOS_FOR_EACH(  ResourceMap, m_resources, it )
        {
            Resource* ptr = it->second;
            ptr->_deleteSelf();
        }

        m_resources.clear();
    }


    //////////////////////////////////////////////////////////////////////////
    const int ResourceManager::SYSTEM_REFERENCE_COUNT = 1;
    bool ResourceManager::s_isSystemClosing = false;
    ResourceManager* g_resourceManager = 0;

    ResourceManager::ResourceManager()
    {
        khaosAssert( !g_resourceManager );
        g_resourceManager = this;
    }

    ResourceManager::~ResourceManager()
    {
        g_resourceManager = 0;
    }

    bool ResourceManager::registerGroup( ClassType type, CreateResourceFunc createFunc, 
        ResAutoCreatorBase* autoCreator, IResourceScheduler* scheduler )
    {
        if ( m_resGroupMap.find(type) != m_resGroupMap.end() )
        {
            khaosLogLn( KHL_L2, "Already exist resource group: %s", KHAOS_TYPE_TO_NAME(type) );
            return false;
        }

        ResourceGroup& group = _getGroup(type);
        group.setCreateFunc( createFunc );
        group.setAutoCreator( autoCreator );
        group.setScheduler( scheduler );
        return true;
    }

    void ResourceManager::setGroupAutoCreator( ClassType type, ResAutoCreatorBase* autoCreator )
    {
        ResourceGroupMap::iterator it = m_resGroupMap.find(type);
        if ( it == m_resGroupMap.end() )
        {
            khaosLogLn( KHL_L2, "Not found resource group: %s", KHAOS_TYPE_TO_NAME(type) );
            return;
        }

        it->second.setAutoCreator( autoCreator );
    }

    void ResourceManager::setGroupScheduler( ClassType type, IResourceScheduler* scheduler )
    {
        ResourceGroupMap::iterator it = m_resGroupMap.find(type);
        if ( it == m_resGroupMap.end() )
        {
            khaosLogLn( KHL_L2, "Not found resource group: %s", KHAOS_TYPE_TO_NAME(type) );
            return;
        }

        it->second.setScheduler( scheduler );
    }

    ResourceGroup* ResourceManager::getGroup( ClassType type ) const
    {
        return &const_cast<ResourceManager*>(this)->_getGroup( type );
    }

    const ResourceGroup& ResourceManager::_getGroup( ClassType type ) const
    {
        return const_cast<ResourceManager*>(this)->_getGroup(type);
    }

    ResourceGroup& ResourceManager::_getGroup( ClassType type )
    {
        return m_resGroupMap[type];
    }

    Resource* ResourceManager::createResource( ClassType type, const String& name1 )
    {
        ResourceGroup& group = _getGroup(type);
        ResourceMap& resMap = group.getResources();

        const String& name = _checkNextName(name1); // NB: l-value referenceָ��r-value�ǺϷ���

        // �Ƿ��Ѿ�����
        if ( resMap.find(name) != resMap.end() )
        {
            khaosLogLn( KHL_L2, "Already exist resource: %s", name.c_str() );
            return 0;
        }

        // ��������ʵ�֣�������
        Resource* ptr = group.getCreateFunc()();
        ptr->_setName( name );
        ptr->_setGroup( &group );
        resMap.insert( ResourceMap::value_type(ptr->getName(), ptr) );

        // һ������
        ptr->addRef();
        return ptr;
    }

    Resource* ResourceManager::getResource( ClassType type, const String& name ) const
    {
        // ͨ�����ֻ�ȡ
        if ( name.empty() )
            return 0;

        const ResourceMap& resMap = _getGroup(type).getResources();

        ResourceMap::const_iterator it = resMap.find( name );
        if ( it != resMap.end() )
        {
            Resource* ptr = it->second;
            ptr->addRef(); // һ������
            return ptr;
        }
        return 0;
    }

    Resource* ResourceManager::getOrCreateResource( ClassType type, const String& name )
    {
        // �Ȼ�ȡ��û���򴴽�
        Resource* ptr = getResource( type, name );
        if ( ptr )
            return ptr;
        return createResource( type, name );
    }

    void ResourceManager::_load( Resource* res, bool async )
    {
        res->_getGroup()->getScheduler()->load( res, async );
    }

    void ResourceManager::_unload( Resource* res, bool async )
    {
        res->_getGroup()->getScheduler()->unload( res, async );
    }

    void ResourceManager::_checkLoad( Resource* res )
    {
        res->_getGroup()->getScheduler()->checkLoad( res ); 
    }

    void ResourceManager::_releaseRes( Resource* res )
    {
        // SYSTEM_REFERENCE_COUNT��manager�Լ��ƹܵ����ü�������
        // ���Ե���SYSTEM_REFERENCE_COUNT����û�б���ⲿ������
        khaosAssert( res->getRefCount() == SYSTEM_REFERENCE_COUNT );

        // ��ע����Ƴ�
        ResourceGroup* group = res->_getGroup();
        ResourceMap& resMap = group->getResources();

        ResourceMap::iterator it = resMap.find( res->getName() );
        khaosAssert( it != resMap.end() );
        resMap.erase( it );

        if ( s_isSystemClosing ) // ϵͳ�����ر�ʱ�̲����첽ֱ��ɾ��
        {
            res->_deleteSelf();
        }
        else
        {
            // ����������ɾ��
            group->getScheduler()->releaseRes(res);
        }
    }
}

