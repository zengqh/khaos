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

        // 注册自己需要的消息
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
        // 检查可加载状态
        if ( !res->setLoadingStatus() )
            return;

        // 向文件系统请求资源数据
        if ( async ) // 异步请求
        {
            res->addRef(); // 增加引用

            IResourceCreator* creator = res->getResCreator();
            if ( creator ) // 人工加载类型
            {
                g_msgQueueManager->postWorkMessage( MQQUEUE_HIGH, MQMSG_CREATE_MANUAL_RESOURCE, res, 0, 0 );
            }
            else // 文件自动加载类型
            {
                String fileName = res->_getGroup()->getAutoCreator()->getLocateFileName( res->getName() );
                g_fileSystem->getAsyncFileData( fileName, res );
            }
        }
        else // 同步请求（必定在ui线程）
        {
            IResourceCreator* creator = res->getResCreator();

            if ( creator ) // 人工加载类型
            {
                bool workOk = creator->prepareResource( res, 0 );
                res->setPreparedStatus( workOk );

                if ( workOk )
                    workOk = creator->buildResource( res );
                res->setCompletedStatus( workOk );
            }
            else  // 文件自动加载类型
            {
                ResAutoCreatorBase* creator = res->_getGroup()->getAutoCreator();

                // 定位
                bool workOk = false;
                String fileName = creator->getLocateFileName( res->getName() );

                // 获取文件数据并准备
                DataBuffer db;
                if ( g_fileSystem->getFileData( fileName, db ) )
                    workOk = creator->prepareResource( res, &db );
                res->setPreparedStatus( workOk );

                // 最后创建工作
                if ( workOk )
                    workOk = creator->buildResource( res );
                res->setCompletedStatus( workOk );
            }
        }
    }

    void DefaultResourceScheduler::unload( Resource* res, bool async )
    {
        // 检查可卸载状态
        if ( !res->setUnloadingStatus() )
            return;

        if ( async ) // 异步在ui线程处理
        {
            res->addRef(); // 增加1次引用
            g_msgQueueManager->postUIMessage( MQMSG_UI_DESTRUCT_RESOURCE, res, 0, 0 );
        }
        else
        {
            // 同步撤销资源
            res->_destructResImpl();

            // 设置卸载状态
            res->setUnloadedStatus();
        }
    }

    void DefaultResourceScheduler::checkLoad( Resource* res )
    {
        // 默认执行加载
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
            // 异步在ui线程处理释放
            g_msgQueueManager->postUIMessage( MQMSG_UI_RELEASE_RESOURCE, res, 0, 0 );
        }
        else
        {
            res->_deleteSelf();
        }
    }

    void DefaultResourceScheduler::onGetFileResult( const String& fileName, void* userData, DataBuffer& buff, IFileSystem::GetResult result )
    {
        // 异步加载请求得到的文件通知
        // 对资源原始数据做解析之类处理
        Resource* res = static_cast<Resource*>(userData);
        bool ok = false;

        if ( result == IFileSystem::GR_OK )
            ok = res->_getGroup()->getAutoCreator()->prepareResource( res, &buff );
        res->setPreparedStatus( ok );

        // 通知ui线程完成处理
        g_msgQueueManager->postUIMessage( MQMSG_UI_BUILD_RESOURCE, res, 0, 0 );
    }

    void DefaultResourceScheduler::onWorkImpl( MsgQueueBase* mq, Message* msg )
    {
        Resource* res = static_cast<Resource*>(msg->lparam);

        switch ( msg->msg )
        {
        case MQMSG_CREATE_MANUAL_RESOURCE: // 异步线程准备人工资源
            {
                bool ok = res->getResCreator()->prepareResource( res, 0 );
                res->setPreparedStatus( ok );
                g_msgQueueManager->postUIMessage( MQMSG_UI_BUILD_MANUAL_RESOURCE, res, 0, 0 );
            }
            break;

        case MQMSG_UI_BUILD_MANUAL_RESOURCE: // ui线程完成创建人工资源
            {
                bool ok = false;
                if ( res->getStatus() == Resource::S_PREPARED_OK )
                    ok = res->getResCreator()->buildResource( res );
                res->setCompletedStatus( ok );
                res->release(); // 撤销1次引用
            }
            break;

        case MQMSG_UI_BUILD_RESOURCE: // 在ui线程完成创建自动资源
            {
                bool ok = false;
                if ( res->getStatus() == Resource::S_PREPARED_OK )
                    ok = res->_getGroup()->getAutoCreator()->buildResource( res );
                res->setCompletedStatus( ok );
                res->release(); // 撤销1次引用
            }
            break;

        case MQMSG_UI_DESTRUCT_RESOURCE: // 在ui线程完成卸载
            {
                res->_destructResImpl();
                res->setUnloadedStatus();
                res->release(); // 撤销1次引用
            }
            break;

        case MQMSG_UI_RELEASE_RESOURCE: // 在ui线程删除
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
        // 处理尚未加载命令就中断（关闭应用程序的时候）
    }

    void DefaultResourceScheduler::onCancelMsg( MsgQueueBase* mq, Message* msg )
    {
        // 处理尚未加载命令就中断（关闭应用程序的时候）
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

        const String& name = _checkNextName(name1); // NB: l-value reference指向r-value是合法的

        // 是否已经存在
        if ( resMap.find(name) != resMap.end() )
        {
            khaosLogLn( KHL_L2, "Already exist resource: %s", name.c_str() );
            return 0;
        }

        // 调用子类实现，并加入
        Resource* ptr = group.getCreateFunc()();
        ptr->_setName( name );
        ptr->_setGroup( &group );
        resMap.insert( ResourceMap::value_type(ptr->getName(), ptr) );

        // 一次引用
        ptr->addRef();
        return ptr;
    }

    Resource* ResourceManager::getResource( ClassType type, const String& name ) const
    {
        // 通过名字获取
        if ( name.empty() )
            return 0;

        const ResourceMap& resMap = _getGroup(type).getResources();

        ResourceMap::const_iterator it = resMap.find( name );
        if ( it != resMap.end() )
        {
            Resource* ptr = it->second;
            ptr->addRef(); // 一次引用
            return ptr;
        }
        return 0;
    }

    Resource* ResourceManager::getOrCreateResource( ClassType type, const String& name )
    {
        // 先获取，没有则创建
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
        // SYSTEM_REFERENCE_COUNT是manager自己掌管的引用计数量，
        // 所以等于SYSTEM_REFERENCE_COUNT就是没有别的外部引用了
        khaosAssert( res->getRefCount() == SYSTEM_REFERENCE_COUNT );

        // 从注册表移除
        ResourceGroup* group = res->_getGroup();
        ResourceMap& resMap = group->getResources();

        ResourceMap::iterator it = resMap.find( res->getName() );
        khaosAssert( it != resMap.end() );
        resMap.erase( it );

        if ( s_isSystemClosing ) // 系统即将关闭时刻不走异步直接删除
        {
            res->_deleteSelf();
        }
        else
        {
            // 交给调度来删除
            group->getScheduler()->releaseRes(res);
        }
    }
}

