#include "KhaosPreHeaders.h"
#include "KhaosMaterial.h"
#include "KhaosRIDMgr.h"
#include "KhaosEffectID.h"

namespace Khaos
{
    RIDObject* MaterialManager_requestRID( void* context );
    void MaterialManager_freeRID( RIDObject* rid );
    void MaterialManager_dirtyRIDFlag();

    Material::Material() : 
        m_rid(0),
        m_mtrState(MaterialStateSet::FRONT_DRAW), m_blendState(BlendStateSet::REPLACE), m_blendEnabled(false),
        m_shaderFlagDirty(true),
        m_currBaseColorAttr(0), m_currMetallicAttr(0), m_currDSpecularAttr(0), m_currRoughnessAttr(0),
        m_currEmissiveAttr(0)
    {
        m_rid = MaterialManager_requestRID( this );

        // shader
        m_effTempId[MP_MATERIAL] = ET_DEF_MATERIAL;
        m_effTempId[MP_SHADOW]   = ET_DEF_SHADOW;
        m_effTempId[MP_REFLECT]  = ET_DEF_REFLECT;
        m_effTempId[MP_DEFPRE]   = ET_DEF_DEFPRE;
    }

    Material::~Material()
    {
        _destructResImpl();
        MaterialManager_freeRID( m_rid );
    }

    Material::ShaderID Material::getShaderFlag()
    {
        _updateShaderFlag();
        return m_shaderFlag.getValue();
    }

    bool Material::_isTextureAttribPrepared( MtrAttribType type ) const
    {
        if ( MtrAttrib* attr = this->getEnabledAttrib(type) )
        {
            return static_cast<TextureAttribBase*>(attr)->isTexturePrepared();
        }

        return 0;
    }

    TextureAttribBase* Material::_getEnabledPreparedTexAttr( MtrAttribType type ) const
    {
        if ( MtrAttrib* attr = this->getEnabledAttrib(type) )
        {
            TextureAttribBase* attrtex = static_cast<TextureAttribBase*>(attr);
            if ( attrtex->isTexturePrepared() )
                return attrtex;
        }

        return 0;
    }

    void Material::_updateShaderFlag()
    {
        if ( !m_shaderFlagDirty )
            return;

        // 首先设置普通渲染状态开关
        MtrCommState::StateID stateComm = m_commState.getValue();
        stateComm &= MtrCommState::VTXCLR; // 只有这些会引起shader变化
        m_shaderFlag.setValue( stateComm );

        // 颜色属性图的情况
        bool isBaseMapUsed = _isTextureAttribPrepared(MA_BASEMAP);

        SpecularMapAttrib* attrSpecMap = this->getEnabledAttrib<SpecularMapAttrib>();

        bool isMetalMapUsed = false;
        bool isDSpecMapUsed = false;
        bool isRoughMapUsed = false;

        if ( attrSpecMap && attrSpecMap->isTexturePrepared() )
        {
            isMetalMapUsed = attrSpecMap->isMetallicEnabled();
            isDSpecMapUsed = attrSpecMap->isDSpecularEnabled();
            isRoughMapUsed = attrSpecMap->isRoughnessEnabled();

            m_shaderFlag.setFlag( EffectID::EN_SPECULARMAP );
        }

        bool isEmissMapUsed = _isTextureAttribPrepared(MA_EMISSIVEMAP);

        // 首确定颜色属性的链接情况
        m_currBaseColorAttr = getEnabledAttrib<BaseColorAttrib>();
        if ( !m_currBaseColorAttr ) m_currBaseColorAttr = InnerAttribSet::getBaseColorAttrib( isBaseMapUsed );

        m_currMetallicAttr = getEnabledAttrib<MetallicAttrib>();
        if ( !m_currMetallicAttr ) m_currMetallicAttr = InnerAttribSet::getMetallicAttrib( isMetalMapUsed );

        m_currDSpecularAttr = getEnabledAttrib<DSpecularAttrib>();
        if ( !m_currDSpecularAttr ) m_currDSpecularAttr = InnerAttribSet::getDSpecularAttrib( isDSpecMapUsed );

        m_currRoughnessAttr = getEnabledAttrib<RoughnessAttrib>();
        if ( !m_currRoughnessAttr ) m_currRoughnessAttr = InnerAttribSet::getRoughnessAttrib( isRoughMapUsed );

        m_currEmissiveAttr = getEnabledAttrib<EmissiveAttrib>();
        if ( !m_currEmissiveAttr ) m_currEmissiveAttr = InnerAttribSet::getEmissiveAttrib( isEmissMapUsed );

        // 确定高光开启：金属物体或者非金属有高光
        if ( m_currMetallicAttr->getValue() > 1e-5f || m_currDSpecularAttr->getValue() > 1e-5f )
            m_shaderFlag.setFlag( EffectID::EN_SPECULAR );

        if ( !m_shaderFlag.testFlag(EffectID::EN_SPECULAR) ) // 没有specular，那么map也一起被优化掉，反正无用
            m_shaderFlag.unsetFlag( EffectID::EN_SPECULARMAP );

        // 自发光颜色
        if ( isAttribEnabled(MA_EMISSIVE) )
            m_shaderFlag.setFlag( EffectID::EN_EMISSIVE );

        // 颜色图
        if ( isBaseMapUsed )
            m_shaderFlag.setFlag( EffectID::EN_BASEMAP );

        if ( isEmissMapUsed )
            m_shaderFlag.setFlag( EffectID::EN_EMISSIVE | EffectID::EN_EMISSMAP ); // 同时开启emissive颜色

        // 几何图
        if ( _isTextureAttribPrepared(MA_NORMALMAP) )
            m_shaderFlag.setFlag( EffectID::EN_NORMMAP );

        bool isOpacityMapUsed = _isTextureAttribPrepared(MA_OPACITYMAP);
        if ( isOpacityMapUsed ) // 不透明图
            m_shaderFlag.setFlag( EffectID::EN_OPACITYMAP );

        if ( (isBaseMapUsed || isOpacityMapUsed) && isAttribEnabled(MA_ALPHATEST) ) // alpha test
            m_shaderFlag.setFlag( EffectID::EN_ALPHATEST );

        if ( _isTextureAttribPrepared(MA_BAKEDAOMAP) )
            m_shaderFlag.setFlag( EffectID::EN_BAKEDAOMAP );

        // 完成更新
        m_shaderFlagDirty = false;
    }

    float Material::_getCurrAlpha() const
    {
        if ( AlphaAttrib* attr = getEnabledAttrib<AlphaAttrib>() )
            return attr->getValue();
        return 1.0f;
    }

    float Material::_getCurrAlphaTest() const
    {
        if ( AlphaTestAttrib* attr = getEnabledAttrib<AlphaTestAttrib>() )
            return attr->getValue();
        return 0.0f;
    }

    int Material::getRID() const
    {
        return m_rid->getRID(); 
    }

    void Material::_setUpdateDirty()
    {
        m_shaderFlagDirty = true;
        MaterialManager_dirtyRIDFlag();
    }

    void Material::setEffTempId( MaterialPass pass, int tmpId ) 
    {
        khaosAssert( 0 <= pass && pass < MP_MAX );
        m_effTempId[pass] = tmpId; 
    }

    int Material::getEffTempId( MaterialPass pass ) const 
    {
        khaosAssert( 0 <= pass && pass < MP_MAX );
        return m_effTempId[pass]; 
    }

    void Material::_destructResImpl()
    {
    }

    void Material::copyFrom( const Resource* rhs )
    {
        const Material* mtrOth = static_cast<const Material*>(rhs);

        // 模版id
        memcpy( this->m_effTempId, mtrOth->m_effTempId, sizeof(this->m_effTempId) );

        // 状态
        this->m_commState    = mtrOth->m_commState;
        this->m_mtrState     = mtrOth->m_mtrState;
        this->m_blendState   = mtrOth->m_blendState;
        this->m_blendEnabled = mtrOth->m_blendEnabled;

        // 属性
        MtrAttribMap::copyFrom( mtrOth );
    }

    void Material::_fetchTextureReadData( MtrAttribType type )
    {
        TextureAttribBase* tex = _getEnabledPreparedTexAttr(type);
        if ( tex )
            tex->getTexture()->_fetchReadData();
    }

    void Material::_prepareRead()
    {
        khaosAssert( this->isLoaded() );

        _updateShaderFlag();

        _fetchTextureReadData(MA_BASEMAP);
        _fetchTextureReadData(MA_SPECULARMAP);
        _fetchTextureReadData(MA_EMISSIVEMAP);
    }

    Color Material::_readDiffuseColor( const Vector2& uv )
    {
        // 获取基本色设置
        Color clrBase = _getCurrBaseColor().getRGB();

        if ( TextureAttribBase* attr = _getEnabledPreparedTexAttr(MA_BASEMAP) )
        {
            clrBase *= attr->getTexture()->_readTex2D( uv ).getRGB();
        }

        // 获取金属程度设置
        float metallic = _getCurrMetallic();

        if ( TextureAttribBase* attr = _getEnabledPreparedTexAttr(MA_SPECULARMAP) )
        {
            SpecularMapAttrib* specmap = static_cast<SpecularMapAttrib*>(attr);
            if ( specmap->isMetallicEnabled() )
            {
                metallic *= specmap->getTexture()->_readTex2D( uv ).r; // rgb = msr
            }
        }

        return clrBase * (1 - metallic);
    }

    Color Material::_readEmissiveColor( const Vector2& uv )
    {
        // 获取emiss
        Color clrEmiss = _getCurrEmissive().getRGB();

        if ( TextureAttribBase* attr = _getEnabledPreparedTexAttr(MA_EMISSIVEMAP) )
        {
            clrEmiss *= attr->getTexture()->_readTex2D( uv ).getRGB();
        }

        return clrEmiss;
    }
}

