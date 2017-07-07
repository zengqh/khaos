#pragma once
#include "KhaosStdTypes.h"
#include "KhaosMatrix3.h"
#include "KhaosMatrix4.h"
#include "KhaosIterator.h"
#include "KhaosStringUtil.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    struct CodeContext
    {
        CodeContext() : src(0), size(0) {}
        const char* src;
        int         size;
    };

    typedef map<const char*, CodeContext>::type CodeContextMap;

    //////////////////////////////////////////////////////////////////////////
    // ShaderParam
    class Shader;
    class TextureObjUnit;

    class ShaderParam : public AllocatedObject
    {
    public:
        ShaderParam() : m_shader(0), m_register(0) {}

    public:
        void _setShader( Shader* shader ) { m_shader = shader; }
        void _setRegister( int r ) { m_register = r; }

        int  getRegister() const { return m_register; }

#define KHAOS_MAKE_SHADERPARAM_METHOD_(name, type) \
        void set##name( type v ); \
        void set##name##2( const type* v ); \
        void set##name##3( const type* v ); \
        void set##name##4( const type* v ); \
        void set##name##Array( const type* v, int count ); \
        void set##name##2Array( const type* v, int count ); \
        void set##name##3Array( const type* v, int count ); \
        void set##name##4Array( const type* v, int count ); \
        void set##name##Array( int startIndex, const type* v, int count ); \
        void set##name##2Array( int startIndex, const type* v, int count ); \
        void set##name##3Array( int startIndex, const type* v, int count ); \
        void set##name##4Array( int startIndex, const type* v, int count ); \
        void set##name##Pack( const void* v, int bytes );

        KHAOS_MAKE_SHADERPARAM_METHOD_(Float, float)
        KHAOS_MAKE_SHADERPARAM_METHOD_(Int, int)

        void setMatrix3( const Matrix3& mat3 );
        void setMatrix4( const Matrix4& mat4 );
        void setMatrix4Array( const Matrix4* mat4, int count );

    private:
        Shader* m_shader;
        int     m_register;
    };

    //////////////////////////////////////////////////////////////////////////
    // ShaderParamEx
    enum ShaderProfile
    {
        SP_VERTEXSHADER,
        SP_PIXELSHADER,
        SP_MAXCOUNT
    };

    class ShaderParamEx : public AllocatedObject
    {
    public:
        ShaderParamEx();

    public:
        void setParam( ShaderProfile sp, ShaderParam* param );

    public:
        KHAOS_MAKE_SHADERPARAM_METHOD_(Float, float)
        KHAOS_MAKE_SHADERPARAM_METHOD_(Int, int)

        void setMatrix3( const Matrix3& mat3 );
        void setMatrix4( const Matrix4& mat4 );
        void setMatrix4Array( const Matrix4* mat4, int count );
        void setTextureSamplerState( TextureObjUnit* texUnit );

    private:
        ShaderParam* m_params[SP_MAXCOUNT];
    };

    //////////////////////////////////////////////////////////////////////////
    // ShaderConstantTable
    class ShaderConstantTable : public AllocatedObject
    {
    public:
        typedef unordered_map<String, ShaderParam*>::type ParamMap;
        typedef ParamMap::value_type ValueType;
        typedef RangeIterator<ParamMap> Iterator;

    public:
        ShaderConstantTable() : m_shader(0) {}
        ~ShaderConstantTable();

    public:
        void _setShader( Shader* shader ) { m_shader = shader; }

        void addParam( const String& name, int r );
        ShaderParam* getParam( const String& name ) const;
        Iterator getAllParams() const { return Iterator(m_params); }

    private:
        Shader*  m_shader;
        ParamMap m_params;
    };

    //////////////////////////////////////////////////////////////////////////
    // ShaderParser
    class Shader;

    class ShaderParser : public AllocatedObject
    {
    public:
        enum RegisterType
        {
            RT_FLOAT, RT_INT, RT_SAMPLER
        };

        struct UniformVar : public AllocatedObject
        {
            UniformVar() : regType(RT_FLOAT), arraySize(0), regBegin(0) {}

            bool isArray() const { return arraySize > 0; }
            int  getElementCount() const { return arraySize < 1 ? 1 : arraySize; }

            String          typeName;
            String          varName;
            RegisterType    regType;
            int             arraySize;
            int             regBegin;
        };

        struct InOutVar : public AllocatedObject
        {
            String typeName;
            String varName;
            String semantics;
        };

        struct UniformVarItem : public AllocatedObject
        {
            UniformVarItem() : line(0), var(0) {}
            UniformVarItem( String* l, UniformVar* v ) : line(l), var(v) {}

            String*     line;
            UniformVar* var;
        };

        struct InOutVarItem : public AllocatedObject
        {
            InOutVarItem() : line(0), var(0) {}
            InOutVarItem( String* l, InOutVar* v ) : line(l), var(v) {}

            String*   line;
            InOutVar* var;
        };

        typedef vector<UniformVarItem>::type    UniformVarItemList;
        typedef vector<InOutVarItem>::type      InOutVarItemList;

        struct UniformPack : public AllocatedObject
        {
            RegisterType        regType;
            String              packName;
            UniformVarItemList  itemList;
            int                 regUsed;
        };
        
        typedef unordered_map<String, UniformPack*>::type UniformPackMap;

    public:
        ShaderParser();
        virtual ~ShaderParser();

    public:
        bool parseSource( const char* code, int len, const CodeContextMap& headCodes );

    public:
        const StringMap&            getDefineMap()       const { return m_defMap; }
        const UniformPackMap&       getUniformPackMap()  const { return m_uniformPackMap; }
        const StringVector&         getSectionHead()     const { return m_sectHead; }
        const UniformVarItemList&   getSectionUniform()  const { return m_sectUnifromVar; }
        const InOutVarItemList&     getSectionInVar()    const { return m_sectInVar; }
        const InOutVarItemList&     getSectionOutVar()   const { return m_sectOutVar; }
        const StringVector&         getSectionComm()     const { return m_sectComm; }
        const StringVector&         getSectionMainFunc() const { return m_sectMainFunc; }

        const UniformPack* findUniformPack( const String& typeName ) const;

    protected:
        void _replaceHeaders( String& outFile, const CodeContextMap& headCodes );

        bool _parseDefine( const String& lineData, int& stat );
        bool _parseVarUniform( const String& lineData, int& stat );
        bool _parseVarIn( const String& lineData, int& stat );
        bool _parseVarOut( const String& lineData, int& stat );
        bool _parseCommSection( const String& lineData, int& stat );
        bool _parseMainFunction( const String& lineData, int& stat );
        bool _parseCanDo( const String& strLineData, int& stat, pcstr strBeg, pcstr strEnd, int startStat, bool& ret );

        bool _checkDefine( String& lineData, int stat, list<bool>::type& condiStack );
        bool _getSourceMacroParams( const char* str, StringVector& sl );
        bool _isIfDef( const String& str );
        bool _isCondiOk( const list<bool>::type& condiStack );

        template<class T>
        void _clear( T& itemList );
        void _clear( UniformPackMap& upm );

    protected:
        static RegisterType _getRegisterType( const String& typeName );
        static int _getTypeNeedRegisterCount( const String& typeName );

    protected:
        StringMap           m_defMap;
        UniformPackMap      m_uniformPackMap;
        StringVector        m_sectHead;
        UniformVarItemList  m_sectUnifromVar;
        InOutVarItemList    m_sectInVar;
        InOutVarItemList    m_sectOutVar;
        StringVector        m_sectComm;
        StringVector        m_sectMainFunc;
        int                 m_uniformRegUsed[3];
        UniformPack*        m_currUniformPack;
    };

    //////////////////////////////////////////////////////////////////////////
    // Shader
    class Shader : public AllocatedObject
    {
    public:
        Shader();
        virtual ~Shader() {}

    public:
        bool create( uint64 id, const CodeContext& mainCode, const CodeContextMap& headCodes, const CodeContext& binCode );

#define KHAOS_MAKE_SHADER_SET_METHOD_(name, type) \
        virtual void set##name( int registerIdx, type v ) = 0; \
        virtual void set##name##2( int registerIdx, const type* v ) = 0; \
        virtual void set##name##3( int registerIdx, const type* v ) = 0; \
        virtual void set##name##4( int registerIdx, const type* v ) = 0; \
        virtual void set##name##Array( int registerIdx, const type* v, int count ) = 0; \
        virtual void set##name##2Array( int registerIdx, const type* v, int count ) = 0; \
        virtual void set##name##3Array( int registerIdx, const type* v, int count ) = 0; \
        virtual void set##name##4Array( int registerIdx, const type* v, int count ) = 0; \
        virtual void set##name##Array( int registerIdx, int startIndex, const type* v, int count ) = 0; \
        virtual void set##name##2Array( int registerIdx, int startIndex, const type* v, int count ) = 0; \
        virtual void set##name##3Array( int registerIdx, int startIndex, const type* v, int count ) = 0; \
        virtual void set##name##4Array( int registerIdx, int startIndex, const type* v, int count ) = 0; \
        virtual void set##name##Pack( int registerIdx, const void* v, int bytes ) = 0;
        
        KHAOS_MAKE_SHADER_SET_METHOD_(Float, float)
        KHAOS_MAKE_SHADER_SET_METHOD_(Int, int)

        virtual void setMatrix3( int registerIdx, const Matrix3& mat3 ) = 0;
        virtual void setMatrix4( int registerIdx, const Matrix4& mat4 ) = 0;
        virtual void setMatrix4Array( int registerIdx, const Matrix4* mat4, int count ) = 0;

    public:
        ShaderParam* getParam( const String& name ) const;
        ShaderConstantTable::Iterator getAllParams() const { return m_constTable.getAllParams(); }

    protected:
        void _buildParams( const ShaderParser& parser );

    protected:
        virtual bool _createImpl( uint64 id, const ShaderParser& parser, const CodeContext& binCode ) = 0;

    protected:
        ShaderConstantTable m_constTable;
    };

    //////////////////////////////////////////////////////////////////////////
    // Effect
    struct EffectCreateContext
    {
        CodeContextMap heads;
        CodeContext codes[SP_MAXCOUNT];
        CodeContext binCodes[SP_MAXCOUNT];
    };

    class Effect : public AllocatedObject
    {
    public:
        typedef unordered_map<String, ShaderParamEx*>::type ParamExMap;

    public:
        Effect();
        virtual ~Effect();

    public:
        virtual bool create( uint64 id, const EffectCreateContext& context ) = 0;

        Shader* getVertexShader() const { return m_vs; }
        Shader* getPixelShader() const { return m_ps; }

        ShaderParamEx* getParam( const String& name ) const;

    protected:
        void _buildParamEx();
        void _appendToParamEx( ShaderProfile sp, const String& name, ShaderParam* param );

    protected:
        union
        {
            struct 
            {
                Shader* m_vs;
                Shader* m_ps;
            };

            Shader* m_shaders[SP_MAXCOUNT];
        };
        
        ParamExMap m_paramExs;
    };
}