#include "KhaosPreHeaders.h"
#include "KhaosShader.h"
#include "KhaosNameDef.h"
#include "KhaosRenderDevice.h"


namespace Khaos
{
    const int MAX_LINE_CHARS = 4096;

    //////////////////////////////////////////////////////////////////////////
#define KHAOS_MAKE_SHADERPARAM_METHOD_IMPL_(name, type) \
    void ShaderParam::set##name( type v ) { m_shader->set##name( m_register, v ); } \
    void ShaderParam::set##name##2( const type* v ) { m_shader->set##name##2( m_register, v ); } \
    void ShaderParam::set##name##3( const type* v ) { m_shader->set##name##3( m_register, v ); } \
    void ShaderParam::set##name##4( const type* v ) { m_shader->set##name##4( m_register, v ); } \
    void ShaderParam::set##name##Array( const type* v, int count ) { m_shader->set##name##Array( m_register, v, count ); } \
    void ShaderParam::set##name##2Array( const type* v, int count ) { m_shader->set##name##2Array( m_register, v, count ); } \
    void ShaderParam::set##name##3Array( const type* v, int count ) { m_shader->set##name##3Array( m_register, v, count ); } \
    void ShaderParam::set##name##4Array( const type* v, int count ) { m_shader->set##name##4Array( m_register, v, count ); } \
    void ShaderParam::set##name##Array( int startIndex, const type* v, int count ) { m_shader->set##name##Array( m_register, startIndex, v, count ); } \
    void ShaderParam::set##name##2Array( int startIndex, const type* v, int count ) { m_shader->set##name##2Array( m_register, startIndex, v, count ); } \
    void ShaderParam::set##name##3Array( int startIndex, const type* v, int count ) { m_shader->set##name##3Array( m_register, startIndex, v, count ); } \
    void ShaderParam::set##name##4Array( int startIndex, const type* v, int count ) { m_shader->set##name##4Array( m_register, startIndex, v, count ); } \
    void ShaderParam::set##name##Pack( const void* v, int bytes ) { m_shader->set##name##Pack( m_register, v, bytes ); }

    KHAOS_MAKE_SHADERPARAM_METHOD_IMPL_(Float, float)
    KHAOS_MAKE_SHADERPARAM_METHOD_IMPL_(Int, int)

    void ShaderParam::setMatrix3( const Matrix3& mat3 )
    {
        m_shader->setMatrix3( m_register, mat3 );
    }

    void ShaderParam::setMatrix4( const Matrix4& mat4 )
    {
        m_shader->setMatrix4( m_register, mat4 );
    }

    void ShaderParam::setMatrix4Array( const Matrix4* mat4, int count )
    {
        m_shader->setMatrix4Array( m_register, mat4, count );
    }

    //////////////////////////////////////////////////////////////////////////
    void ShaderParamEx::setParam( ShaderProfile sp, ShaderParam* param )
    {
        khaosAssert( 0 <= sp && sp < SP_MAXCOUNT );
        m_params[sp] = param;
    }

#define _for_each_sp(x) \
    for ( int i = 0; i < SP_MAXCOUNT; ++i ) \
    { \
        if ( ShaderParam* sp = m_params[i] ) \
        { \
            x; \
        } \
    }

#define KHAOS_MAKE_SHADERPARAMEX_METHOD_IMPL_(name, type) \
    void ShaderParamEx::set##name( type v ) { _for_each_sp((sp->set##name(v))) } \
    void ShaderParamEx::set##name##2( const type* v ) { _for_each_sp((sp->set##name##2(v))) } \
    void ShaderParamEx::set##name##3( const type* v ) { _for_each_sp((sp->set##name##3(v))) } \
    void ShaderParamEx::set##name##4( const type* v ) { _for_each_sp((sp->set##name##4(v))) } \
    void ShaderParamEx::set##name##Array( const type* v, int size ) { _for_each_sp((sp->set##name##Array(v, size))) } \
    void ShaderParamEx::set##name##2Array( const type* v, int size ) { _for_each_sp((sp->set##name##2Array(v, size))) } \
    void ShaderParamEx::set##name##3Array( const type* v, int size ) { _for_each_sp((sp->set##name##3Array(v, size))) } \
    void ShaderParamEx::set##name##4Array( const type* v, int size ) { _for_each_sp((sp->set##name##4Array(v, size))) } \
    void ShaderParamEx::set##name##Array( int startIndex, const type* v, int size ) { _for_each_sp((sp->set##name##Array(startIndex, v, size))) } \
    void ShaderParamEx::set##name##2Array( int startIndex, const type* v, int size ) { _for_each_sp((sp->set##name##2Array(startIndex, v, size))) } \
    void ShaderParamEx::set##name##3Array( int startIndex, const type* v, int size ) { _for_each_sp((sp->set##name##3Array(startIndex, v, size))) } \
    void ShaderParamEx::set##name##4Array( int startIndex, const type* v, int size ) { _for_each_sp((sp->set##name##4Array(startIndex, v, size))) } \
    void ShaderParamEx::set##name##Pack( const void* v, int bytes ) { _for_each_sp((sp->set##name##Pack(v, bytes))) }

    ShaderParamEx::ShaderParamEx()
    {
        memset( m_params, 0, sizeof(m_params) );
    }

    KHAOS_MAKE_SHADERPARAMEX_METHOD_IMPL_(Float, float)
    KHAOS_MAKE_SHADERPARAMEX_METHOD_IMPL_(Int, int)

    void ShaderParamEx::setMatrix3( const Matrix3& mat3 )
    {
        _for_each_sp((sp->setMatrix3(mat3)))
    }

    void ShaderParamEx::setMatrix4( const Matrix4& mat4 )
    {
        _for_each_sp((sp->setMatrix4(mat4)))
    }

    void ShaderParamEx::setMatrix4Array( const Matrix4* mat4, int count )
    {
        _for_each_sp((sp->setMatrix4Array(mat4, count)))
    }

    void ShaderParamEx::setTextureSamplerState( TextureObjUnit* texUnit )
    {
        int lastRegIdx = -999;

        for ( int i = 0; i < SP_MAXCOUNT; ++i )
        {
            if ( ShaderParam* sp = m_params[i] )
            {
                int currRegIdx = sp->getRegister();

                if ( lastRegIdx != currRegIdx )
                {
                    g_renderDevice->setTexture( currRegIdx, texUnit->getTextureObj() );
                    g_renderDevice->setSamplerState( currRegIdx, texUnit->getSamplerState() );
                    lastRegIdx = currRegIdx;
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    ShaderConstantTable::~ShaderConstantTable()
    {
        for ( ParamMap::iterator it = m_params.begin(), ite = m_params.end(); it != ite; ++it )
        {
            ShaderParam* sp = it->second;
            KHAOS_DELETE sp;
        }
    }

    void ShaderConstantTable::addParam( const String& name, int r )
    {
        if ( m_params.find( name ) != m_params.end() )
        {
            khaosLogLn( KHL_L2, "Already exist shader param %s", name.c_str() );
            return;
        }

        ShaderParam* sp = KHAOS_NEW ShaderParam;
        sp->_setShader( m_shader );
        sp->_setRegister( r );

        m_params.insert( ParamMap::value_type(name, sp) );
    }

    ShaderParam* ShaderConstantTable::getParam( const String& name ) const
    {
        ParamMap::const_iterator it = m_params.find( name );
        if ( it != m_params.end() )
            return it->second;
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    ShaderParser::ShaderParser() : m_currUniformPack(0)
    {
        for ( int i = 0; i < KHAOS_ARRAY_SIZE(m_uniformRegUsed); ++i )
            m_uniformRegUsed[i] = 0;
    }

    ShaderParser::~ShaderParser() 
    {
        _clear(m_sectUnifromVar);
        _clear(m_sectInVar);
        _clear(m_sectOutVar);
        _clear(m_uniformPackMap);
    }

    template<class T>
    void ShaderParser::_clear( T& itemList )
    {
        for ( typename T::iterator it = itemList.begin(), ite = itemList.end(); it != ite; ++it )
        {
            typename T::reference item = *it;

            if ( item.var )
                KHAOS_DELETE item.var;
            else
                KHAOS_DELETE_T(item.line);
        }

        itemList.clear();
    }

    void ShaderParser::_clear( UniformPackMap& upm )
    {
        for ( UniformPackMap::iterator it = upm.begin(), ite = upm.end(); it != ite; ++it )
        {
            UniformPack* uvpack = it->second;
            _clear( uvpack->itemList );
            KHAOS_DELETE uvpack;
        }

        upm.clear();
    }

    const ShaderParser::UniformPack* ShaderParser::findUniformPack( const String& typeName ) const
    {
        UniformPackMap::const_iterator it = m_uniformPackMap.find( typeName );
        if ( it != m_uniformPackMap.end() )
            return it->second;
        return 0;
    }

    void ShaderParser::_replaceHeaders( String& outFile, const CodeContextMap& headCodes )
    {
        String includeName = "#include<";

        for ( CodeContextMap::const_iterator it = headCodes.begin(), ite = headCodes.end();
            it != ite; ++it )
        {
            String headLine = includeName + it->first + ">";
            replaceString<String>( outFile, headLine, it->second.src );
        }
    }

    bool ShaderParser::parseSource( const char* code, int len, const CodeContextMap& headCodes )
    {
        String tmp(code);
        _replaceHeaders( tmp, headCodes );

        TxtStreamReader reader(tmp.c_str());
        char buff[MAX_LINE_CHARS];

        int  stat = 0;
        list<bool>::type condiStack;

        while ( reader.getLine(buff) )
        {
            String lineData = buff;
            trim( lineData );

            // define/uniform/varin/varout定义阶段执行
            if ( 0 <= stat && stat <= 6 )
            {
                if ( _checkDefine( lineData, stat, condiStack ) )
                    continue;
            }

            // 先解析khaos自定义宏
            if ( _parseDefine( lineData, stat ) )
                continue;

            if ( !_isCondiOk( condiStack ) ) // IFDEF(x)失败后执行跳过
                continue;

            // 解析uniform
            if ( _parseVarUniform( lineData, stat ) )
                continue;

            // 解析varin
            if ( _parseVarIn( lineData, stat ) )
                continue;

            // 解析varout
            if ( _parseVarOut( lineData, stat ) )
                continue;

            // 解析comm
            if ( _parseCommSection( lineData, stat ) )
                continue;

            // 解析主函数
            if ( _parseMainFunction( lineData, stat ) )
                continue;

            // main结束
            break;
        }

        return true;
    }

    bool ShaderParser::_parseDefine( const String& strLineData, int& stat )
    {
        if ( stat > 0 )
            return false;

        khaosAssert( stat == 0 );
        const char* lineData = strLineData.c_str();

        if ( isStartWith(lineData, "BEGIN_DECLARE_UNIFORMS(") )
        {
            stat = 1;
            return false; // to next stage
        }
        else if ( isStartWith(lineData, "DEFINE_LINE(") )
        {
            // DEFINE_LINE(name, value)
            // DEFINE_LINE(name)
            StringVector paraList;
            _getSourceMacroParams( lineData, paraList );
            khaosAssert( paraList.size() == 1 || paraList.size() == 2 );
            if ( paraList.size() == 1 )
                paraList.push_back( STRING_EMPTY );
            m_defMap[paraList[0]] = paraList[1];
        }
        else
        {
            // 头区段获取行
            m_sectHead.push_back( strLineData );
        }

        return true;
    }

    bool ShaderParser::_parseCanDo( const String& strLineData, int& stat, pcstr strBeg, pcstr strEnd, int startStat, bool& ret )
    {
        ret = false;
        int endStat = startStat + 1;
        if ( stat > endStat )
            return false;

        ret = true;
        khaosAssert( stat == startStat || stat == endStat );
        const char* lineData = strLineData.c_str();

        if ( stat == startStat )
        {
            // 开始
            if ( isStartWith(lineData, strBeg) )
                stat = endStat;

            return false; // 未开始前都忽略
        }

        //if ( stat == endStat  )
        {
            // 结束
            if ( isStartWith(lineData, strEnd) )
            {
                stat = endStat + 1; // 转下个阶段
                return false;
            }
        }

        return true;
    }

    bool ShaderParser::_parseVarUniform( const String& strLineData, int& stat )
    {
        bool ret;
        if ( !_parseCanDo( strLineData, stat, "BEGIN_DECLARE_UNIFORMS(", "END_DECLARE_UNIFORMS(", 1, ret ) )
            return ret;

        const char* lineData = strLineData.c_str();

        if ( isStartWith(lineData, "VAR_UNIFORM(" ) )
        {
            // VAR_UNIFORM(type, name[, size])
            StringVector paraList;
            _getSourceMacroParams( lineData, paraList );

            // 得到变量
            UniformVar* uvar = KHAOS_NEW UniformVar;

            uvar->typeName  = paraList[0];
            uvar->varName   = paraList[1];
            uvar->arraySize = paraList.size() >= 3 ? stringToInt(paraList[2].c_str()) : 0;

            if ( m_currUniformPack ) // 定义包情况
            {
                khaosAssert( _getRegisterType( uvar->typeName ) == m_currUniformPack->regType );

                uvar->regType  = m_currUniformPack->regType;
                uvar->regBegin = m_currUniformPack->regUsed; //m_uniformRegUsed[ uvar->regType ];

                m_currUniformPack->itemList.push_back( UniformVarItem(0, uvar) );
                m_currUniformPack->regUsed += _getTypeNeedRegisterCount( uvar->typeName ) * uvar->getElementCount();
            }
            else // 正常情况
            {
                const UniformPack* pack = findUniformPack( uvar->typeName ); // 是否是包结构

                uvar->regType  = pack ? pack->regType : _getRegisterType( uvar->typeName );
                uvar->regBegin = m_uniformRegUsed[ uvar->regType ];

                m_sectUnifromVar.push_back( UniformVarItem(0, uvar) );
                m_uniformRegUsed[uvar->regType] += 
                    (pack ? pack->regUsed : _getTypeNeedRegisterCount(uvar->typeName)) * uvar->getElementCount();
            }
        }
        else if ( isStartWith(lineData, "VAR_UNIFORM_PACK_BEGIN(") )
        {
            // VAR_UNIFORM_PACK_BEGIN(type, name)
            StringVector paraList;
            _getSourceMacroParams( lineData, paraList );
            khaosAssert( paraList.size() == 2 );

            khaosAssert( !m_currUniformPack );
            m_currUniformPack = KHAOS_NEW UniformPack;
            m_currUniformPack->regType  = _getRegisterType( paraList[0] );
            m_currUniformPack->packName = paraList[1];
            m_currUniformPack->regUsed  = 0;
        }
        else if ( isStartWith(lineData, "VAR_UNIFROM_PACK_END(") )
        {
            khaosAssert( m_currUniformPack );
            m_uniformPackMap[m_currUniformPack->packName] = m_currUniformPack;
            m_currUniformPack = 0;
        }
        else
        {
            // 获取未识别行
            m_sectUnifromVar.push_back( UniformVarItem(KHAOS_NEW_T(String)(strLineData), 0) );
        }

        return true;
    }

    bool ShaderParser::_parseVarIn( const String& strLineData, int& stat )
    {
        bool ret;
        if ( !_parseCanDo( strLineData, stat, "BEGIN_DECLARE_INS(", "END_DECLARE_INS(", 3, ret ) )
            return ret;

        const char* lineData = strLineData.c_str();

        if ( isStartWith(lineData, "VAR_IN(") )
        {
            // VAR_IN(type, name, sema)
            StringVector paraList;
            _getSourceMacroParams( lineData, paraList );
            khaosAssert( paraList.size() == 3 );

            // 获取变量
            InOutVar* iv = KHAOS_NEW InOutVar;
            
            iv->typeName  = paraList[0];
            iv->varName   = paraList[1];
            iv->semantics = paraList[2];

            m_sectInVar.push_back( InOutVarItem(0, iv) );
        }
        else
        {
            // 获取未识别行
            m_sectInVar.push_back( InOutVarItem(KHAOS_NEW_T(String)(strLineData), 0) );
        }

        return true;
    }

    bool ShaderParser::_parseVarOut( const String& strLineData, int& stat )
    {
        bool ret;
        if ( !_parseCanDo( strLineData, stat, "BEGIN_DECLARE_OUTS(", "END_DECLARE_OUTS(", 5, ret ) )
            return ret;

        const char* lineData = strLineData.c_str();

        if ( isStartWith(lineData, "VAR_OUT(") )
        {
            // VAR_OUT(type, name, sema)
            StringVector paraList;
            _getSourceMacroParams( lineData, paraList );
            khaosAssert( paraList.size() == 3 );

            // 获取变量
            InOutVar* iv = KHAOS_NEW InOutVar;

            iv->typeName  = paraList[0];
            iv->varName   = paraList[1];
            iv->semantics = paraList[2];

            m_sectOutVar.push_back( InOutVarItem(0, iv) );
        }
        else
        {
            // 获取未识别行
            m_sectOutVar.push_back( InOutVarItem(KHAOS_NEW_T(String)(strLineData), 0) );
        }

        return true;
    }

    bool ShaderParser::_parseCommSection( const String& strLineData, int& stat )
    {
        bool ret;
        if ( !_parseCanDo( strLineData, stat, "BEGIN_SECTION(", "END_SECTION(", 7, ret ) )
            return ret;

        {
            // 获取代码行
            m_sectComm.push_back( strLineData );
        }

        return true;
    }

    bool ShaderParser::_parseMainFunction( const String& strLineData, int& stat )
    {
        bool ret;
        if ( !_parseCanDo( strLineData, stat, "BEGIN_MAIN_FUNCTION(", "END_MAIN_FUNCTION(", 9, ret ) )
            return ret;

        {
            // 获取代码行
            m_sectMainFunc.push_back( strLineData );
        }

        return true;
    }

    bool ShaderParser::_isCondiOk( const list<bool>::type& condiStack )
    {
        if ( condiStack.size() >= 2 )
        {
            int a;
            a = 1;
        }

        KHAOS_FOR_EACH_CONST( list<bool>::type, condiStack, it )
        {
            if ( !(*it) )
                return false;
        }

        return true;
    }

    bool ShaderParser::_checkDefine( String& lineData, int stat, list<bool>::type& condiStack )
    {
        // 检查IF_DEF(x)
        const char* strLine = lineData.c_str();

        if ( isStartWith(strLine, "IFDEF(") )
        {
            condiStack.push_back( _isIfDef( lineData ) );
            return true;
        }
        else if ( isStartWith(strLine, "ELSE(") )
        {
            // else后状态取反
            condiStack.back() = !condiStack.back();
            return true;
        }
        else if ( isStartWith(strLine, "ENDIF(") )
        {
            condiStack.pop_back();
            return true;
        }

        // 检查替换
        if ( stat > 0 )
        {
            for ( StringMap::iterator it = m_defMap.begin(), ite = m_defMap.end(); it != ite; ++it )
            {
                const String& strOld = it->first;
                const String& strNew = it->second;

                replaceString( lineData, strOld, strNew );
            }
        }

        return false;
    }

    bool ShaderParser::_getSourceMacroParams( const char* str, StringVector& sl )
    {
        // 解析诸如：VAR_UNIFORM(type, name)
        int leftBracket = findString( str, '(' );
        if ( leftBracket < 0 )
            return false;

        ++leftBracket;
        const char* strLeft = str+leftBracket;
        int rightBracket = findString( strLeft, ')' );
        if ( rightBracket < 0 )
            return false;

        splitString( sl, String(strLeft, strLeft+rightBracket), ", ", true );
        return true;
    }

    bool ShaderParser::_isIfDef( const String& str )
    {
        // 解析简单表达式：IF_DEF( m1 && ( m2 || !m3 ) )
        static StringVector keys;

        if ( keys.empty() )
        {
            keys.push_back( "&&" );
            keys.push_back( "||" );
            keys.push_back( "(" );
            keys.push_back( ")" );
        }

        static const String& STR_AND    = keys[0];
        static const String& STR_0R     = keys[1];
        static const String& STR_LEFTB  = keys[2];
        static const String& STR_RIGHTB = keys[3];

        const int VAL_TRUE  = 1;
        const int VAL_FALSE = 0;
        const int OP_AND    = -1;
        const int OP_OR     = -2;
        const int OP_LEFTB  = -3;
        const int OP_RIGHTB = -4;

        // 取出符号和值
        StringVector sl;
        splitStringByKey( sl, str, keys );
        khaosAssert( sl.size() >= 4 );
        
        // 解析
        typedef list<int>::type IntList;
        IntList opExp;

        for ( size_t i = 1; i < sl.size(); ++i )
        {
            const String& str = sl[i];
            if ( str == STR_AND )
            {
                // and操作入栈
                opExp.push_back(OP_AND);
            }
            else if ( str == STR_0R )
            {
                // or操作入栈
                opExp.push_back(OP_OR);
            }
            else if ( str == STR_LEFTB )
            {
                // 左括号入栈
                opExp.push_back(OP_LEFTB);
            }
            else if ( str == STR_RIGHTB )
            {
                // 右括号
                int curVal = opExp.back(); // 取出当前值
                khaosAssert( curVal == VAL_FALSE || curVal == VAL_TRUE );
                opExp.pop_back(); // 弹出

                int lastExp = opExp.back(); // 再弹一次
                opExp.pop_back();

                if ( lastExp != OP_LEFTB ) // 是个符号
                {
                    khaosAssert( lastExp == OP_AND || lastExp == OP_OR );

                    int lastLastExp = opExp.back(); // 再弹个数值
                    khaosAssert( lastLastExp == VAL_FALSE || lastLastExp == VAL_TRUE );
                    opExp.pop_back();

                    khaosAssert( opExp.back() == OP_LEFTB ); // 再弹左括号
                    opExp.pop_back();

                    // 计算
                    if ( lastExp == OP_AND )
                        curVal = curVal & lastLastExp;
                    else
                        curVal = curVal | lastLastExp;
                }

                // 存入当前值
                opExp.push_back( curVal );
            }
            else // 是个数值
            {
                // 获取到一个宏
                int val;

                if ( str.front() != '!' ) 
                    val = m_defMap.find( str ) != m_defMap.end() ? VAL_TRUE : VAL_FALSE;
                else // 简单取反
                    val = m_defMap.find( &str[1] ) == m_defMap.end() ? VAL_TRUE : VAL_FALSE;

                // 取栈顶符号
                int curExp = opExp.back();

                // 是个与或操作则执行计算
                if ( curExp == OP_AND || curExp == OP_OR )
                {
                    opExp.pop_back();
                    int lastVal = opExp.back();
                    khaosAssert( lastVal == VAL_FALSE || lastVal == VAL_TRUE );
                    opExp.pop_back();
                    if ( curExp == OP_AND )
                        val = val & lastVal;
                    else
                        val = val | lastVal;
                }

                // 结果存回
                opExp.push_back( val );
            }
        }

        khaosAssert( opExp.size() == 1 );
        return opExp.back() == VAL_TRUE;
    }

    ShaderParser::RegisterType ShaderParser::_getRegisterType( const String& typeName )
    {
        if ( isStartWith(typeName.c_str(), "float") ||
             isStartWith(typeName.c_str(), "half") )
             return RT_FLOAT;

        if ( isStartWith(typeName.c_str(), "int") )
            return RT_INT;

        if ( isStartWith(typeName.c_str(), "sampler") )
            return RT_SAMPLER;

        khaosLogLn( KHL_L1, "Unknown Type Name %s", typeName.c_str() );
        return RT_FLOAT;
    }

    int ShaderParser::_getTypeNeedRegisterCount( const String& typeName )
    {
        typedef unordered_map<String, int>::type StringMap;
        static StringMap s_typeList;

        if ( s_typeList.empty() )
        {
            s_typeList["float"] = 1;
            s_typeList["half"] = 1;
            s_typeList["int"] = 1;

            for ( int i = 2; i <= 4; ++i )
            {
                String strIdx = intToString(i);
                s_typeList[String("float") + strIdx] = 1;
                s_typeList[String("half") + strIdx] = 1;
                s_typeList[String("int") + strIdx] = 1;
            }

            s_typeList["float4x4"] = 4;
            s_typeList["half4x4"] = 4;
            s_typeList["sampler"] = 1;
        }

        int regCnt;

        StringMap::iterator it = s_typeList.find( typeName );
        if ( it != s_typeList.end() )
        {
            regCnt = it->second;
        }
        else
        {
            khaosLogLn( KHL_L1, "Unknown Type Name %s", typeName.c_str() );
            regCnt = 4;
        }

        return regCnt;
    }

    //////////////////////////////////////////////////////////////////////////
    Shader::Shader()
    {
        m_constTable._setShader(this);
    }

    bool Shader::create( uint64 id, const CodeContext& mainCode, const CodeContextMap& headCodes, const CodeContext& binCode )
    {
        ShaderParser parser;
        if ( !parser.parseSource( mainCode.src, mainCode.size, headCodes ) )
            return false;

        if ( !_createImpl( id, parser, binCode ) )
            return false;

        _buildParams( parser );
        return true;
    }

    void Shader::_buildParams( const ShaderParser& parser )
    {
        const ShaderParser::UniformVarItemList& secUniform = parser.getSectionUniform();

        for ( ShaderParser::UniformVarItemList::const_iterator it = secUniform.begin(), ite = secUniform.end(); it != ite; ++it )
        {
            const ShaderParser::UniformVarItem& uvar = *it;

            if ( uvar.var )
                m_constTable.addParam( uvar.var->varName, uvar.var->regBegin );
        }
    }

    ShaderParam* Shader::getParam( const String& name ) const
    {
        return m_constTable.getParam( name );
    }

    //////////////////////////////////////////////////////////////////////////
    Effect::Effect() : m_vs(0), m_ps(0)
    {
    }

    Effect::~Effect()
    {
        for ( ParamExMap::iterator it = m_paramExs.begin(), ite = m_paramExs.end(); it != ite; ++it )
        {
            ShaderParamEx* spe = it->second;
            KHAOS_DELETE spe;
        }

        for ( int i = 0; i < SP_MAXCOUNT; ++i )
        {
            if ( Shader* shader = m_shaders[i] )
                KHAOS_DELETE shader;
        }
    }

    ShaderParamEx* Effect::getParam( const String& name ) const
    {
        ParamExMap::const_iterator it = m_paramExs.find( name );
        if ( it != m_paramExs.end() )
            return it->second;
        return 0;
    }

    void Effect::_buildParamEx()
    {
        // 遍历每个shader
        for ( int i = 0; i < SP_MAXCOUNT; ++i )
        {
            // 有这个shader
            if ( Shader* shader = m_shaders[i] )
            {
                // 遍历shader每个参数
                ShaderConstantTable::Iterator it = shader->getAllParams();
                
                while ( it.hasNext() )
                {
                    const ShaderConstantTable::ValueType& v = it.get();
                    _appendToParamEx( (ShaderProfile)i, v.first, v.second );
                }
            }
        }
    }

    void Effect::_appendToParamEx( ShaderProfile sp, const String& name, ShaderParam* param )
    {
        ShaderParamEx*& paramEx = m_paramExs[name];
        if ( !paramEx )
            paramEx = KHAOS_NEW ShaderParamEx;
        paramEx->setParam( sp, param );
    }
}

