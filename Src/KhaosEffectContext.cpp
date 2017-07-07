#include "KhaosPreHeaders.h"
#include "KhaosEffectContext.h"
#include "KhaosEffectSetters.h"
#include "KhaosRenderDevice.h"
#include "KhaosRenderable.h"

#if KHAOS_DEBUG
    #define TEST_DEBUG_SHADER_ 1
#else
    #define TEST_DEBUG_SHADER_ 0
#endif

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    uint32 EffSetterParams::_globalStamp = 0;
    uint32 EffSetterParams::_externStamp = 0;

    void EffSetterParams::clear()
    {
        ra = 0;
        _mtr = 0;
        _litsInfo = 0;

        for ( int i = 0; i < LT_MAX; ++i )
            _litListInfo[i] = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    EffectContext::SharedHandleMap EffectContext::s_sharedHandles[2];

    EffectContext::~EffectContext() 
    {
        clear(); 
        g_effectTemplateManager->_freeRID( m_rid );
    }

    void EffectContext::bindEffect( Effect* eff, const EffectID& id )
    {
        khaosAssert( !m_effect && !m_rid );

        m_effect = eff;
        m_effectID = id;

        m_rid = g_effectTemplateManager->_requestRID( this );
    }

    uint32* EffectContext::_getSharedHandle( ClassType cls, bool isExtern )
    {
        int idx = isExtern ? 0 : 1;
        UInt32Ptr& ptr = s_sharedHandles[idx][cls];
        if ( ptr )
            return ptr.get();

        uint32* val = KHAOS_MALLOC_T(uint32);
        *val = 0;
        ptr.attach( val );
        return ptr.get();
    }

    EffectSetterBase** EffectContext::_findSetter( EffectSetterBase::ParamType type, ClassType clsType )
    {
        if ( type != EffectSetterBase::PT_STATE )
        {
            EffectSetterList& setterList = m_setterList[type];

            for ( size_t i = 0; i < setterList.size(); ++i )
            {
                EffectSetterBase*& currsetter = setterList[i];

                if ( KHAOS_OBJECT_TYPE(currsetter) == clsType )
                    return &currsetter;
            }
        }
        else
        {
            if ( m_setterState )
                return &m_setterState;
        }

        return 0;
    }

    void EffectContext::_addSetter( EffectSetterBase::ParamType type, EffectSetterBase* effSetter, bool replaceMode )
    {
        // 检查是否已经存在
        EffectSetterBase** setterReplace = 0;

        if ( replaceMode )
        {
            setterReplace = _findSetter( type, KHAOS_OBJECT_TYPE(effSetter) );

            if ( setterReplace )
            {
                KHAOS_DELETE (*setterReplace);
                *setterReplace = 0;
            }
        }
        else
        {
#if KHAOS_DEBUG
            // debug下我们仅仅断言不存在
            khaosAssert( !_findSetter(type, KHAOS_OBJECT_TYPE(effSetter)) );
#endif
        }

        // 开始初始化和添加
        effSetter->init( this );

        if ( type != EffectSetterBase::PT_STATE )
        {
            if ( type == EffectSetterBase::PT_GLOBAL || type == EffectSetterBase::PT_EXTERN )
            {
                // 共享
                effSetter->setSharedHandle( _getSharedHandle( KHAOS_OBJECT_TYPE(effSetter),
                    type == EffectSetterBase::PT_EXTERN ) );
            }

            if ( setterReplace )
                *setterReplace = effSetter;
            else
                m_setterList[type].push_back( effSetter );
        }
        else
        {
            m_setterState = effSetter;
        }
    }

    void EffectContext::addSetter( EffectSetterBase::ParamType type, EffectSetterBase* effSetter )
    {
        _addSetter( type, effSetter, false );
    }

    void EffectContext::replaceSetter( EffectSetterBase::ParamType type, EffectSetterBase* effSetter )
    {
        _addSetter( type, effSetter, true );
    }

    void EffectContext::clear()
    {
        for ( int i = 0; i < EffectSetterBase::PT_MAX; ++i )
        {
            EffectSetterList& setters = m_setterList[i];

            KHAOS_FOR_EACH( EffectSetterList, setters, it )
            {
                EffectSetterBase* e = *it;
                KHAOS_DELETE e;
            }

            setters.clear();
        }

        KHAOS_SAFE_DELETE(m_setterState);
    }

    void EffectContext::doSet( EffSetterParams* params )
    {
        // 检查material，不同才设置
        Material* curMtr = params->ra->getImmMaterial();
        if ( curMtr != params->_mtr )
        {
            params->_mtr = curMtr;
            _callDoSet( m_setterList[EffectSetterBase::PT_MATERIAL], params );
        }
        
        // 检查lits info，不同才设置
        LightsInfo* curLitInfo = params->ra->getImmLightInfo();
        if ( curLitInfo != params->_litsInfo )
        {
            params->_litsInfo = curLitInfo;
            _callDoSet( m_setterList[EffectSetterBase::PT_LIGHT], params );
        }

        // 几何信息数据总是每次更新
        _callDoSet( m_setterList[EffectSetterBase::PT_GEOM], params );

        // 检查外部设置，第一次才设置
        _callDoSetShared( m_setterList[EffectSetterBase::PT_EXTERN], params, params->_externStamp );

        // 检查全局设置，第一次才设置
        _callDoSetShared( m_setterList[EffectSetterBase::PT_GLOBAL], params, params->_globalStamp );

        // 状态数据总是每次更新
        m_setterState->doSet( params );
    }

    void EffectContext::_callDoSet( EffectSetterList& setters, EffSetterParams* params )
    {
        KHAOS_FOR_EACH( EffectSetterList, setters, it )
        {
            EffectSetterBase* e = *it;
            e->doSet( params );
        }
    }

    void EffectContext::_callDoSetShared( EffectSetterList& setters, EffSetterParams* params, uint32 stamp )
    {
        KHAOS_FOR_EACH( EffectSetterList, setters, it )
        {
            EffectSetterBase* e = *it;
            if ( e->checkUpdate(stamp) )
                e->doSet( params );
        }
    }

    //////////////////////////////////////////////////////////////////////////
    EffectTemplate::~EffectTemplate() 
    {
        clear();
        m_buildStrategy->release();
    }

    bool EffectTemplate::init( const StringVector& names, const StringVector& includeFiles, IEffectBuildStrategy* buildStrategy )
    {
        m_buildStrategy = buildStrategy;
        return _loadEffectCode( names, includeFiles );
    }

    void EffectTemplate::clear()
    {
        for ( EffectContextMap::iterator it = m_effContextMap.begin(), ite = m_effContextMap.end(); it != ite; ++it )
        {
            EffectContext* eff = it->second;
            KHAOS_DELETE eff->getEffect();
            KHAOS_DELETE eff;
        }

        m_effContextMap.clear();
    }

    bool EffectTemplate::_loadEffectCode( const StringVector& names, const StringVector& includeFiles )
    {

#define _LOAD_FILE_DATA(name, buf) \
        if ( !g_fileSystem->getFileData( name, buf ) ) \
        { \
            khaosLogLn( KHL_L1, ("Can not load " + name).c_str() ); \
            return false; \
        }

#define STR_APPEND_(str, db) str.append( (const char*)db.data, (const char*)db.data + db.dataLen )

        // shader公共头
        String strPre;

        {
            FileSystem::DataBufferAuto buffComm;
            String strCommPath = "System/comm.sl";
            _LOAD_FILE_DATA( strCommPath, buffComm );
            STR_APPEND_(strPre, buffComm);
        }

        // 自定义头
        for ( size_t i = 0; i < includeFiles.size(); ++i )
        {
            FileSystem::DataBufferAuto buffTmp;
            String strPath = "System/" + includeFiles[i] + ".inc";
            _LOAD_FILE_DATA( strPath, buffTmp );

            String& headCode = m_effCode.headCodes[ includeFiles[i] ];
            STR_APPEND_(headCode, buffTmp);
        }

        // vs/ps shader
        static const char* extNames[] = { "vs", "ps" };

        for ( size_t i = 0; i < names.size(); ++i )
        {
            // 读取源码
            FileSystem::DataBufferAuto buffTmp;
            String strPath = "System/" + names[i] + "." + extNames[i];
            _LOAD_FILE_DATA( strPath, buffTmp );

            m_effCode.strCode[i] = strPre;
            STR_APPEND_(m_effCode.strCode[i], buffTmp);
        }

        return true;
    }

    void EffectTemplate::calcEffectID( Renderable* ra, EffectID& id )
    {
        m_buildStrategy->calcEffectID( ra, id );

        // 56 - 63 tempid
        id.setFlag( (uint64)m_tempID << 56 );
    }

    EffectContext* EffectTemplate::getEffectContext( const EffectID& id )
    {
        // 是否已经存在
        EffectContextMap::iterator it = m_effContextMap.find( id.getValue() );
        if ( it != m_effContextMap.end() )
            return it->second;

        // 根据id计算推断id，得到一组宏
        EffectID inferId;
        m_buildStrategy->calcInferID( id, inferId );

        String strDefine = m_buildStrategy->makeSystemDefine(); // 得到系统宏
        strDefine += m_buildStrategy->makeDynamicDefine( id, inferId ); // 得到相关的宏

        // 拼接代码
        String tmpCodes[SP_MAXCOUNT];
        String tmpBinCodes[SP_MAXCOUNT];
        EffectCreateContext ecc;

        // 文件头表
        for ( EffectCode::HeadMap::iterator it = m_effCode.headCodes.begin(), 
            ite = m_effCode.headCodes.end(); it != ite; ++it )
        {
            CodeContext& cc = ecc.heads[it->first.c_str()];
            cc.src = it->second.c_str();
            cc.size = (int) it->second.size();
        }
      
        // 主文件
        static const char* s_pre_names[SP_MAXCOUNT] = { "vs", "ps" };

        for ( int i = 0; i < SP_MAXCOUNT; ++i )
        {
            if ( m_effCode.strCode[i].size() )
            {
                tmpCodes[i] = strDefine + m_effCode.strCode[i];
                ecc.codes[i].src = tmpCodes[i].c_str();
                ecc.codes[i].size = (int) tmpCodes[i].size();
            }
            else
            {
                ecc.codes[i].src = "";
                ecc.codes[i].size = 0;
            }

            #if TEST_DEBUG_SHADER_
                String strID = uint64ToHexStr( id.getValue() );
                char file[1024] = {};
                sprintf( file, "E:/download/%s_src_%s.txt", s_pre_names[i], strID.c_str() );
                //writeStringToFile( tmpCodes[i].c_str(), file );
            #endif

            // 尝试读取二进制
            {
                FileSystem::DataBufferAuto buffTmp;
                String strPath = String("Shader/") + s_pre_names[i] + "_" + 
                    uint64ToHexStr( id.getValue() ) +
                    "." + s_pre_names[i] + "b";

                if ( g_fileSystem->isExist( strPath ) )
                {
                    g_fileSystem->getFileData( strPath, buffTmp );

                    tmpBinCodes[i].assign( (const char*)buffTmp.data, buffTmp.dataLen );
                    ecc.binCodes[i].src  = tmpBinCodes[i].data();
                    ecc.binCodes[i].size = (int) tmpBinCodes[i].size();
                }
            }
        }

        // 渲染设备创建effect对象
        Effect* eff = g_renderDevice->createEffect();
        if ( !eff->create( id.getValue(), ecc ) )
        {
            khaosLogLn( KHL_L1, "RenderBase::_getObjectEffect failed" );
            return 0;
        }

        // 创建effect context
        EffectContext* ec = KHAOS_NEW EffectContext;
        ec->bindEffect( eff, id );

        // 根据id安装setter
        m_buildStrategy->installEffectSetter( ec, id, inferId );

        if ( !ec->_hasStateSetter() )
            ec->addSetter( EffectSetterBase::PT_STATE, KHAOS_NEW EffectEmptyStateSetter );

        // 加入到表
        m_effContextMap.insert( EffectContextMap::value_type(id.getValue(), ec) );
        return ec;
    }

    EffectContext* EffectTemplate::getEffectContext( Renderable* ra )
    {
        EffectID effId;
        calcEffectID( ra, effId );
        return getEffectContext( effId );
    }

    //////////////////////////////////////////////////////////////////////////
    EffectTemplateManager* g_effectTemplateManager = 0;

    EffectTemplateManager::EffectTemplateManager()
    {
        khaosAssert( !g_effectTemplateManager );
        g_effectTemplateManager = this;

        m_ridMgr.setSortFunc( _compareEffect );
    }

    EffectTemplateManager::~EffectTemplateManager()
    {
        clear();
        g_effectTemplateManager = 0;
    }

    void EffectTemplateManager::registerEffectTemplate( int tempId, const StringVector& names, const StringVector& includeFiles, IEffectBuildStrategy* bs )
    {
        if ( (int)m_effTempMap.size() < tempId+1 )
            m_effTempMap.resize(tempId+1, 0);

        EffectTemplate*& et = m_effTempMap[tempId];
        khaosAssert( !et );

        et = KHAOS_NEW EffectTemplate(tempId);
        if ( !et->init( names, includeFiles, bs ) )
            khaosLogLn( KHL_L1, "EffectTemplateManager::registerEffectTemplate failed: %d", tempId );
    }

    void EffectTemplateManager::registerEffectTemplateVSPS( int tempId, const String& name, IEffectBuildStrategy* bs )
    {
        registerEffectTemplate( tempId, makeStringVector(name, name), StringVector(), bs );
    }

    void EffectTemplateManager::registerEffectTemplateVSPS( int tempId, const String& vsName, const String& psName, IEffectBuildStrategy* bs )
    {
        registerEffectTemplate( tempId, makeStringVector(vsName, psName), StringVector(), bs );
    }

    void EffectTemplateManager::registerEffectTemplateVSPS( int tempId, const String& vsName, const String& psName, const String& headFile, IEffectBuildStrategy* bs )
    {
        registerEffectTemplate( tempId, makeStringVector(vsName, psName), makeStringVector(headFile), bs );
    }

    void EffectTemplateManager::registerEffectTemplateVSPS( int tempId, const String& vsName, const String& psName, const StringVector& headFiles, IEffectBuildStrategy* bs )
    {
        registerEffectTemplate( tempId, makeStringVector(vsName, psName), headFiles, bs );
    }

    EffectTemplate* EffectTemplateManager::getEffectTemplate( int id )
    {
        return m_effTempMap[id];
    }

    void EffectTemplateManager::clear()
    {
        KHAOS_FOR_EACH( EffTempMap, m_effTempMap, it )
        {
            EffectTemplate* et = *it;
            KHAOS_DELETE et;
        }

        m_effTempMap.clear();
    }

    RIDObject* EffectTemplateManager::_requestRID( void* context )
    {
        return m_ridMgr.requestRIDObj( context );
    }

    void EffectTemplateManager::_freeRID( RIDObject* rid )
    {
        m_ridMgr.freeRIDObj( rid );
    }

    void EffectTemplateManager::update()
    {
        m_ridMgr.update();
    }

    bool EffectTemplateManager::_compareEffect( const RIDObject* lhs, const RIDObject* rhs )
    {
        Effect* effLeft  = ((EffectContext*)lhs->getOwner())->getEffect();
        Effect* effRight = ((EffectContext*)rhs->getOwner())->getEffect();

        // 先比较vs
        if ( effLeft->getVertexShader() != effRight->getVertexShader() )
            return effLeft->getVertexShader() < effRight->getVertexShader();

        // 然后比较ps
        return effLeft->getPixelShader() < effRight->getPixelShader();        
    }
}

