#pragma once
#include "KhaosMaterial.h"
#include "KhaosLight.h"
#include "KhaosShader.h"
#include "KhaosRTTI.h"
#include "KhaosSharedPtr.h"
#include "KhaosEffectID.h"
#include "KhaosRIDMgr.h"

namespace Khaos
{
    class LightsInfo;
    class Renderable;
    class EffectContext;

    //////////////////////////////////////////////////////////////////////////
    // EffectSetterBase
    struct EffSetterParams
    {
    public:
        EffSetterParams()
        {
            clear();
        }

        void clear();

        void stampGlobal() { ++_globalStamp; }
        void stampExtern() { ++_externStamp; }

        void clearAndStampGlobal()
        {
            clear();
            stampGlobal();
        }

    public:
        Renderable* ra;
        Material*   _mtr;
        LightsInfo* _litsInfo;
        LightsInfo::LightListInfo* _litListInfo[LT_MAX];

        static uint32 _globalStamp;
        static uint32 _externStamp;
    };

    class EffectSetterBase : public AllocatedObject
    {
        KHAOS_DECLARE_RTTI(EffectSetterBase)

    public:
        enum ParamType
        {
            PT_MATERIAL,
            PT_GEOM,
            PT_LIGHT,
            PT_GLOBAL,
            PT_EXTERN,
            PT_MAX,
            PT_STATE
        };

    public:
        EffectSetterBase() : m_context(0), m_sharedHandle(0) {}
        virtual ~EffectSetterBase() {}

        void setSharedHandle( uint32* sharedHandle ) { m_sharedHandle = sharedHandle; }
        
        bool checkUpdate( uint32 stamp )
        {
            khaosAssert( m_sharedHandle );

            if ( *m_sharedHandle < stamp )
            {
                *m_sharedHandle = stamp;
                return true;
            }

            return false;
        }

        virtual void init( EffectContext* context ) { m_context = context; }
        virtual void doSet( EffSetterParams* params ) = 0;

    protected:
        EffectContext* m_context;
        uint32*        m_sharedHandle;
    };

    //////////////////////////////////////////////////////////////////////////
    // EffectContext
    class EffectContext : public AllocatedObject
    {
        typedef vector<EffectSetterBase*>::type             EffectSetterList;
        typedef SharedPtr<uint32, SPFM_FREE>                UInt32Ptr;
        typedef unordered_map<ClassType, UInt32Ptr>::type   SharedHandleMap;

    public:
        EffectContext() : m_rid(0), m_effect(0), m_setterState(0) {}
        ~EffectContext();

    public:
        void addSetter( EffectSetterBase::ParamType type, EffectSetterBase* effSetter );
        void replaceSetter( EffectSetterBase::ParamType type, EffectSetterBase* effSetter );
        bool _hasStateSetter() const { return m_setterState != 0; }

        void clear();
        void doSet( EffSetterParams* params );

        int getRID() const { return m_rid->getRID(); }

    public:
        void bindEffect( Effect* eff, const EffectID& id );

        Effect* getEffect() const { return m_effect; }
        const EffectID& getEffectID() const { return m_effectID; }

    private:
        void _addSetter( EffectSetterBase::ParamType type, EffectSetterBase* effSetter, bool replaceMode );

        EffectSetterBase** _findSetter( EffectSetterBase::ParamType type, ClassType clsType );

        void _callDoSet( EffectSetterList& setters, EffSetterParams* params );
        void _callDoSetShared( EffectSetterList& setters, EffSetterParams* params, uint32 stamp );

        static uint32* _getSharedHandle( ClassType cls, bool isExtern );

    private:
        RIDObject*        m_rid;
        Effect*           m_effect;
        EffectID          m_effectID;
        EffectSetterBase* m_setterState;
        EffectSetterList  m_setterList[EffectSetterBase::PT_MAX];

        static SharedHandleMap s_sharedHandles[2];
    };

    //////////////////////////////////////////////////////////////////////////
    // IEffectBuildStrategy
    struct IEffectBuildStrategy
    {
        virtual void   release() = 0;
        virtual void   calcEffectID( Renderable* ra, EffectID& id ) = 0;
        virtual void   calcInferID( const EffectID& id, EffectID& inferId  ) = 0;
        virtual String makeSystemDefine() = 0;
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId ) = 0;
        virtual void   installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId ) = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    // EffectTemplate
    struct EffectCode
    {
        typedef map<String, String>::type HeadMap;
        HeadMap headCodes;
        String strCode[SP_MAXCOUNT];
    };

    class EffectTemplate : public AllocatedObject
    {
        typedef unordered_map<EffectID::ValueType, EffectContext*>::type EffectContextMap;

    public:
        EffectTemplate( int tempID ) : m_tempID(tempID), m_buildStrategy(0) {}
        ~EffectTemplate();

    public:
        bool init( const StringVector& names, const StringVector& includeFiles, IEffectBuildStrategy* buildStrategy );
        void clear();

        void calcEffectID( Renderable* ra, EffectID& id );

        EffectContext* getEffectContext( const EffectID& idOri );
        EffectContext* getEffectContext( Renderable* ra );

    private:
        bool _loadEffectCode( const StringVector& names, const StringVector& includeFiles );

    private:
        int                     m_tempID;
        EffectCode              m_effCode;
        EffectContextMap        m_effContextMap;
        IEffectBuildStrategy*   m_buildStrategy;
    };

    //////////////////////////////////////////////////////////////////////////
    // EffectTemplateManager
    class EffectTemplateManager : public AllocatedObject
    {
        struct RegisterEntry
        {
            RegisterEntry() : buildStrategy(0) {}

            String                name;
            IEffectBuildStrategy* buildStrategy;
        };

        typedef vector<EffectTemplate*>::type EffTempMap;

    public:
        EffectTemplateManager();
        ~EffectTemplateManager();

        void registerEffectTemplate( int tempId, const StringVector& names, const StringVector& includeFiles, IEffectBuildStrategy* bs );
        void registerEffectTemplateVSPS( int tempId, const String& name, IEffectBuildStrategy* bs );
        void registerEffectTemplateVSPS( int tempId, const String& vsName, const String& psName, IEffectBuildStrategy* bs );
        void registerEffectTemplateVSPS( int tempId, const String& vsName, const String& psName, const String& headFile, IEffectBuildStrategy* bs );
        void registerEffectTemplateVSPS( int tempId, const String& vsName, const String& psName, const StringVector& headFiles, IEffectBuildStrategy* bs );

        void update();

        void clear();

        EffectTemplate* getEffectTemplate( int tempId );

    public:
        RIDObject* _requestRID( void* context );
        void _freeRID( RIDObject* rid );

    private:
        static bool _compareEffect( const RIDObject* lhs, const RIDObject* rhs );
        
    private:
        EffTempMap m_effTempMap;
        RIDObjectManager m_ridMgr;
    };

    extern EffectTemplateManager* g_effectTemplateManager;

#define _registerEffectVSPS(id, name, strat) \
    g_effectTemplateManager->registerEffectTemplateVSPS(id, name, KHAOS_NEW strat)

#define _registerEffectVSPS2(id, vsname, psname, strat) \
    g_effectTemplateManager->registerEffectTemplateVSPS(id, vsname, psname, KHAOS_NEW strat)

#define _registerEffectVSPS2H(id, vsname, psname, headfile, strat) \
    g_effectTemplateManager->registerEffectTemplateVSPS(id, vsname, psname, headfile, KHAOS_NEW strat)


#define _registerEffectVSPS2HS(id, vsname, psname, headfile, strat) \
    g_effectTemplateManager->registerEffectTemplateVSPS(id, vsname, psname, makeStringVector##headfile, KHAOS_NEW strat)
}

