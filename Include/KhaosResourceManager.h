#pragma once
#include "KhaosResource.h"
#include "KhaosFileSystem.h"
#include "KhaosAutoNameManager.h"

namespace Khaos
{
    class ResourceManager;

    //////////////////////////////////////////////////////////////////////////
    class ResAutoCreatorBase : public AllocatedObject, public IResourceCreator
    {
    public:
        virtual ~ResAutoCreatorBase() {}

        virtual void release();
        virtual bool prepareResource( Resource* res, void* context );

    public:
        void setBasePath( const String& basePath ) { m_basePath = basePath; }
        const String& getBasePath() const { return m_basePath; }

        virtual String getLocateFileName( const String& name ) const;

    protected:
        virtual bool _prepareResourceImpl( Resource* res, DataBuffer& buff ) = 0;

    protected:
        String m_basePath;
    };

    //////////////////////////////////////////////////////////////////////////
    struct IResourceScheduler
    {
        virtual void release() = 0;
        virtual void load( Resource* res, bool async ) = 0;
        virtual void unload( Resource* res, bool async ) = 0;
        virtual void checkLoad( Resource* res ) = 0;
        virtual void releaseRes( Resource* res ) = 0;
    };

    class DefaultResourceScheduler : public AllocatedObject, public IResourceScheduler, public IFileSystemListener, public IMsgQueueListener
    {
    public:
        DefaultResourceScheduler();
        virtual ~DefaultResourceScheduler();

    public:
        void setCheckLoadAsync( bool b ) { m_checkLoadAsync = b; }

    public:
        // IResourceScheduler
        virtual void release();
        virtual void load( Resource* res, bool async );
        virtual void unload( Resource* res, bool async );
        virtual void checkLoad( Resource* res );
        virtual void releaseRes( Resource* res );

    protected:
        // IFileSystemListener
        virtual void onGetFileResult( const String& fileName, void* userData, DataBuffer& buff, IFileSystem::GetResult result );
        virtual void onCancelGetFile( const String& fileName, void* userData );

        // IMsgQueueListener
        virtual void onWorkImpl( MsgQueueBase* mq, Message* msg );
        virtual void onCancelMsg( MsgQueueBase* mq, Message* msg );

    protected:
        bool m_checkLoadAsync;
    };

    extern DefaultResourceScheduler* g_defaultResourceScheduler;

    //////////////////////////////////////////////////////////////////////////
    // 资源管理器
    typedef unordered_map<String, Resource*>::type ResourceMap;
    typedef Resource* (*CreateResourceFunc)();

    class ResourceGroup
    {
    public:
        ResourceGroup() : m_createFunc(0), m_autoCreator(0), m_scheduler(0) {}
        ~ResourceGroup();

        void setCreateFunc( CreateResourceFunc func );
        void setAutoCreator( ResAutoCreatorBase* creator );
        void setScheduler( IResourceScheduler* scheduler );
        void clearResources();

        CreateResourceFunc  getCreateFunc()  const { return m_createFunc; }
        ResAutoCreatorBase* getAutoCreator() const { return m_autoCreator; }
        IResourceScheduler* getScheduler()   const { return m_scheduler; }

        const ResourceMap& getResources() const { return m_resources; }
        ResourceMap& getResources() { return m_resources; }

    private:
        CreateResourceFunc  m_createFunc;
        ResAutoCreatorBase* m_autoCreator;
        IResourceScheduler* m_scheduler;
        ResourceMap         m_resources;
    };

    class ResourceManager : public AllocatedObject, public AutoNameManager
    {
    public:
        static const int SYSTEM_REFERENCE_COUNT;

    public:
        typedef unordered_map<ClassType, ResourceGroup>::type ResourceGroupMap;

    public:
        ResourceManager();
        virtual ~ResourceManager();

    public:
        bool registerGroup( ClassType type, CreateResourceFunc createFunc, 
            ResAutoCreatorBase* autoCreator, IResourceScheduler* scheduler );

        void setGroupAutoCreator( ClassType type, ResAutoCreatorBase* autoCreator );
        void setGroupScheduler( ClassType type, IResourceScheduler* scheduler );

        Resource* createResource( ClassType type, const String& name );
        Resource* getOrCreateResource( ClassType type, const String& name );
        Resource* getResource( ClassType type, const String& name ) const;
        
        template<class T>
        T* createResource( const String& name )
        {
            return static_cast<T*>(createResource(KHAOS_CLASS_TYPE(T), name));
        }

        template<class T>
        T* getOrCreateResource( const String& name )
        {
            return static_cast<T*>(getOrCreateResource(KHAOS_CLASS_TYPE(T), name));
        }

        template<class T>
        T* getResource( const String& name )
        {
            return static_cast<T*>(getResource(KHAOS_CLASS_TYPE(T), name));
        }

        ResourceGroup* getGroup( ClassType type ) const;

        template<class T>
        ResourceGroup* getGroup() const
        {
            return getGroup( KHAOS_CLASS_TYPE(T) );
        }

    public:
        void _load( Resource* res, bool async );
        void _unload( Resource* res, bool async );
        void _checkLoad( Resource* res );
        void _releaseRes( Resource* res );

    private:
        const ResourceGroup& _getGroup( ClassType type ) const;
        ResourceGroup& _getGroup( ClassType type );

    public:
        static void _setSystemClosing() { s_isSystemClosing = true; }

    private:
        ResourceGroupMap m_resGroupMap;

        static bool s_isSystemClosing;
    };

    extern ResourceManager* g_resourceManager;

    //////////////////////////////////////////////////////////////////////////
#define KHAOS_RESMAN_COMM_INIT(type, autoCreator) \
    public: \
    static Resource* _createFunc() { return KHAOS_NEW type; } \
    static void init() { g_resourceManager->registerGroup(KHAOS_CLASS_TYPE(type), _createFunc, KHAOS_NEW autoCreator, g_defaultResourceScheduler); }

#define KHAOS_RESMAN_COMM_SHUTDOWN(type) \
    public: \
    static void shutdown() { g_resourceManager->getGroup(KHAOS_CLASS_TYPE(type))->clearResources(); }

#define KHAOS_RESMAN_COMM_OVERRIDE(type) \
    public: \
    static type* create##type( const String& name ) { return g_resourceManager->createResource<type>(name); } \
    static type* getOrCreate##type( const String& name ) { return g_resourceManager->getOrCreateResource<type>(name); } \
    static type* get##type( const String& name ) { return g_resourceManager->getResource<type>(name); } \
    static const ResourceMap& getAll##type##s() { return g_resourceManager->getGroup<type>()->getResources(); }

#define KHAOS_RESMAN_COMM_IMPL(type, autoCreator) \
    KHAOS_RESMAN_COMM_INIT(type, autoCreator) \
    KHAOS_RESMAN_COMM_SHUTDOWN(type) \
    KHAOS_RESMAN_COMM_OVERRIDE(type)

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    void _bindResourceRoutine( ResPtr<T>& ptr, T* obj, IResourceListener* listener )
    {
        if ( ptr.get() == obj )
            return;

        if ( obj )
            obj->addRef();

        if ( ptr && listener )
            ptr->removeListener( listener );

        ptr.reset( obj );

        if ( obj )
        {
            if ( listener )
                obj->addListener( listener );

            obj->checkLoad();
        }
    }

    template<class T>
    void _bindResourceRoutine( ResPtr<T>& ptr, const String& name, IResourceListener* listener )
    {
        ResPtr<T> tmp;

        if ( name.size() )
            tmp.attach( g_resourceManager->getOrCreateResource<T>( name ) );

        _bindResourceRoutine( ptr, tmp.get(), listener );
    }
}

