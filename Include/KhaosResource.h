#pragma once
#include "KhaosStdTypes.h"
#include "KhaosRTTI.h"
#include "KhaosResPtr.h"
#include "KhaosFileSystem.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class ResourceGroup;
    class Resource;

    //////////////////////////////////////////////////////////////////////////
    struct IResourceCreator
    {
        virtual void release() = 0;
        virtual bool prepareResource( Resource* res, void* context ) = 0;
        virtual bool buildResource( Resource* res ) = 0;
    };

    struct IResourceListener
    {
        virtual void onResourceInitialized( Resource* res ) { onResourceLoaded(res); }
        virtual void onResourceLoaded( Resource* res ) {}
        virtual void onResourceBad( Resource* res ) {}
        virtual void onResourceUpdate( Resource* res ) {}
    };

    class ResourceNullCreator : public AllocatedObject, public IResourceCreator
    {
    public:
        virtual void release(){}
        virtual bool prepareResource( Resource* res, void* context ) { return true; }
        virtual bool buildResource( Resource* res ){ return true; }
    };

    //////////////////////////////////////////////////////////////////////////
    class Resource : public AllocatedObject, public Noncopyable
    {
        KHAOS_DECLARE_RTTI(Resource)

    public:
        enum State
        {
            S_UNLOADED,
            S_LOADING,
            S_PREPARED_OK,
            S_PREPARED_FAILED,
            S_LOADED,
            S_BADRES,
            S_UNLOADING
        };

    protected:
        typedef set<IResourceListener*>::type ListenerList;

    public:
        Resource() : m_group(0), m_resCreator(0), m_refCount(1), m_loadAsync(0), m_status(S_UNLOADED) {}
        virtual ~Resource();

    public:
        void _setName( const String& name ) { m_name = name; }
        const String& getName() const { return m_name; }
        
        String getResFileName() const;
        String getResFullFileName() const;

        void _setGroup( ResourceGroup* gp ) { m_group = gp; }
        ResourceGroup* _getGroup() const { return m_group; }

        void setResCreator( IResourceCreator* creator );
        void setNullCreator( bool loadNow = true );
        IResourceCreator* getResCreator() const { return m_resCreator; }

    public:
        void addListener( IResourceListener* listener );
        void removeListener( IResourceListener* listener );

    public:
        void  setLoadAsync( bool b );
        void  clearLoadAsync();
        bool* _getLoadAsync() const { return m_loadAsync; }

        void load( bool async );
        void unload( bool async );
        bool checkLoad();

    public:
        bool setLoadingStatus();
        void setPreparedStatus( bool ok );
        void setCompletedStatus( bool ok );

        bool setUnloadingStatus();
        void setUnloadedStatus();

        uint8 getStatus() const { return m_status; }
        bool  isLoaded() const { return m_status == S_LOADED; }

    public:
        void addRef() { ++m_refCount; }
        void release();
        uint getRefCount() const { return m_refCount; }

    public:
        virtual void copyFrom( const Resource* rhs ) {}

    public:
        virtual void _deleteSelf();
        virtual void _destructResImpl() {}

        DataBuffer& _getBufferTmp() { return m_dbTmp; }

    protected:
        void _notifyUpdate();

    protected:
        ResourceGroup*              m_group;
        IResourceCreator*           m_resCreator;
        String                      m_name;
        ListenerList                m_listeners;
        uint                        m_refCount;
        bool*                       m_loadAsync;
        volatile uint8              m_status;
        DataBuffer                  m_dbTmp;
    };

    typedef ResPtr<Resource> ResourcePtr;
}

