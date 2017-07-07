#pragma once
#include "KhaosResource.h"
#include "KhaosMtrAttrib.h"
#include "KhaosMaterialDef.h"

namespace Khaos
{
    class RIDObject;

    class Material : public Resource, public MtrAttribMap
    {
        KHAOS_DECLARE_RTTI(Material)

    public:
        typedef MtrCommState::StateID StateID;
        typedef BitSet<StateID> StateSet;

        typedef uint32 ShaderID;
        typedef BitSet<uint32> ShaderFlag;

    public:
        Material();
        virtual ~Material();

    public:
        // effect
        void setEffTempId( MaterialPass pass, int tmpId );
        int  getEffTempId( MaterialPass pass ) const;

    public:
        // comm state
        void setCommState( const StateSet& s ) { m_commState = s; _setUpdateDirty(); }
        const StateSet& getCommState() const { return m_commState; }
        void enableCommState( StateID type, bool en ) { m_commState.enableFlag( type, en ); _setUpdateDirty(); }
        bool isCommStateEnabled( StateID type ) const { return m_commState.testFlag( type ); }

        // material state
        void setCullMode( CullMode cm ) { m_mtrState.cullMode = cm; _setUpdateDirty(); }
        void setWireframe( bool wf ) { m_mtrState.wireframe = wf; _setUpdateDirty(); }
        void setMaterialState( MaterialStateSet state ) { m_mtrState = state; _setUpdateDirty(); }

        CullMode         getCullMode()      const { return (CullMode)m_mtrState.cullMode; }
        bool             isWireframe()      const { return m_mtrState.wireframe != 0; }
        MaterialStateSet getMaterialState() const { return m_mtrState; }

        // blend state
        void setBlendOp( BlendOp op ) { m_blendState.op = op; _setUpdateDirty(); }
        void setBlendSrc( BlendVal val ) { m_blendState.srcVal = val; _setUpdateDirty(); }
        void setBlendDest( BlendVal val ) { m_blendState.destVal = val; _setUpdateDirty(); }
        void setBlendState( BlendStateSet state ) { m_blendState = state; _setUpdateDirty(); }
        void enableBlend( bool en ) { m_blendEnabled = en; _setUpdateDirty(); }

        BlendOp          getBlendOp()       const { return (BlendOp)m_blendState.op; }
        BlendVal         getBlendSrc()      const { return (BlendVal)m_blendState.srcVal; }
        BlendVal         getBlendDest()     const { return (BlendVal)m_blendState.destVal; }
        BlendStateSet    getBlendState()    const { return m_blendState; }
        bool             isBlendEnabled()   const { return m_blendEnabled; }

    public:
        // shader flag
        ShaderID getShaderFlag();

        // rid
        int getRID() const;

        // for bake
        void _prepareRead();
        Color _readDiffuseColor( const Vector2& uv );
        Color _readEmissiveColor( const Vector2& uv );

    public:
        virtual void copyFrom( const Resource* rhs );
        virtual void _destructResImpl();

    private:
        virtual void _notifyNeedUpdateAttr( MtrAttrib* attr ) { _setUpdateDirty(); }
        void _setUpdateDirty();
        void _updateShaderFlag();

        bool _isTextureAttribPrepared( MtrAttribType type ) const;
        TextureAttribBase* _getEnabledPreparedTexAttr( MtrAttribType type ) const;

        void _fetchTextureReadData( MtrAttribType type );

    public:
        const Color& _getCurrBaseColor() const { return m_currBaseColorAttr->getValue(); }
        float        _getCurrMetallic() const { return m_currMetallicAttr->getValue(); }
        float        _getCurrDSpecular() const { return m_currDSpecularAttr->getValue(); }
        float        _getCurrRoughness() const { return m_currRoughnessAttr->getValue(); }
        const Color& _getCurrEmissive() const { return m_currEmissiveAttr->getValue(); }

        float _getCurrAlpha() const;
        float _getCurrAlphaTest() const;
        
    private:
        RIDObject*       m_rid;

        // effect
        int              m_effTempId[MP_MAX];

        // state
        MaterialStateSet m_mtrState;
        BlendStateSet    m_blendState;
        StateSet         m_commState;
        bool             m_blendEnabled;

        // temp
        bool       m_shaderFlagDirty;
        ShaderFlag m_shaderFlag;
        
        const BaseColorAttrib* m_currBaseColorAttr;
        const MetallicAttrib*  m_currMetallicAttr;
        const DSpecularAttrib* m_currDSpecularAttr;
        const RoughnessAttrib* m_currRoughnessAttr;
        const EmissiveAttrib*  m_currEmissiveAttr;
    };

    typedef ResPtr<Material> MaterialPtr;
}

