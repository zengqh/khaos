#include "KhaosPreHeaders.h"
#include "KhaosGlossyTool.h"
#include "KhaosEffectID.h"
#include "KhaosRenderDevice.h"
#include "KhaosImageProcess.h"
#include "KhaosRenderTarget.h"
#include "KhaosEffectContext.h"
#include "KhaosEffectBuildStrategy.h"
#include "KhaosEffectSetters.h"
#include "KhaosRenderable.h"
#include "KhaosMaterialManager.h"
#include "KhaosMaterialFile.h"
#include "KhaosTexCfgParser.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class SpecularAABuildStrategy : public DefaultBuildStrategy
    {
    public:
        virtual void calcEffectID( Renderable* ra, EffectID& id )
        {
            Material* mtr = ra->getImmMaterial();
            id.setValue( _makeMtrShaderID(mtr, false, false) );
        }

        virtual void calcInferID( const EffectID& id, EffectID& inferId  ) {}
        virtual String makeDynamicDefine( const EffectID& id, const EffectID& inferId );
        virtual void installEffectSetter( EffectContext* ec, const EffectID& id, const EffectID& inferId );
    };

    KHAOS_DECL_SINGLE_SETTER(EffectCurrMipSetter, "mipInfo")

    KHAOS_BEGIN_DYNAMIC_DEFINE(SpecularAABuildStrategy)
        KHAOS_TEST_EFFECT_FLAG(EN_SPECULAR)
        KHAOS_TEST_EFFECT_FLAG(EN_SPECULARMAP)
        KHAOS_TEST_EFFECT_FLAG(EN_NORMMAP)
    KHAOS_END_DYNAMIC_DEFINE()

    KHAOS_BEGIN_INSTALL_SETTER(SpecularAABuildStrategy)
        KHAOS_ALWAYS_BIND_SETTER(PT_MATERIAL, EffectSurfaceParamsSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_SPECULARMAP, PT_MATERIAL, EffectSpecularMapSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_SPECULARMAP, PT_MATERIAL, EffectSpecularMaskParamsSetter)
        KHAOS_TEST_EFFECT_SETTER(EN_NORMMAP, PT_MATERIAL, EffectNormMapSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_MATERIAL, EffectCurrMipSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_MATERIAL, EffectTargetInfoSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_MATERIAL, EffectTargetInfoExSetter)
        KHAOS_ALWAYS_BIND_SETTER(PT_STATE, EffectPostStateSetter)
    KHAOS_END_INSTALL_SETTER()

    //////////////////////////////////////////////////////////////////////////
    class ImageSpecularAAProcess : public ImageProcess
    {
    public:
        ImageSpecularAAProcess() : m_currMip(0), m_totalMip(0)
        {
            m_pin = _createImagePin( ET_SPECULAR_AA );
            m_pin->setOwnOutput();
            _setRoot( m_pin );
        }

        void setMaterial( Material* mtr )
        {
            m_pin->setMaterial( mtr );
        }

        void setOutput( TextureObj* texOut )
        {
            m_pin->getOutput()->linkRTT( texOut );
        }

        void setCurrMip( int curr, int total )
        {
            m_currMip = curr;
            m_totalMip = total;
            m_pin->setOutRTTLevel( curr );
        }

        int getCurrMip() const { return m_currMip; }
        int getTotalMip() const { return m_totalMip; }

    private:
        ImagePin*  m_pin;
        int m_currMip;
        int m_totalMip;
    };

    void EffectCurrMipSetter::doSet( EffSetterParams* params )
    {
        FullScreenDSRenderable* ra = static_cast<FullScreenDSRenderable*>(params->ra);
        ImagePin* pin = (ImagePin*) ra->getData();
        ImageSpecularAAProcess* process = static_cast<ImageSpecularAAProcess*>(pin->getOwner());
        float mipInfo[2] = { (float)process->getCurrMip(), (float)process->getTotalMip() };
        m_param->setFloat2( mipInfo );
    }

    //////////////////////////////////////////////////////////////////////////
    GlossyAATool::GlossyAATool() : m_mtr(0), m_specularOutRTT(0), m_normalMap(0)
    {
        init();
    }

    void GlossyAATool::init()
    {
        static bool s_inited = false;
        if ( s_inited )
            return;

        s_inited = true;

        _registerEffectVSPS2H( ET_SPECULAR_AA, "commDrawScreen", "specularAA", 
            "materialUtil", SpecularAABuildStrategy );
    }

    void GlossyAATool::build( Material* mtr )
    {
        m_mtr = mtr;

        if ( !_needProcess() )
            return;

        if ( !_prepareSpecularMap() )
            return;

        _modifyAllMips();
        _saveResults();
        _clean();
    }

    bool GlossyAATool::_needProcess()
    {
        EffectID id;
        id.setValue( m_mtr->getShaderFlag() );
        
        // 没有高频的法线贴图，不用处理
        if ( !id.testFlag(EffectID::EN_NORMMAP) )
            return false;

        // 没有高光不用处理
        if ( !id.testFlag( EffectID::EN_SPECULAR ) )
            return false;

        // 需要处理
        return true;
    }

    void GlossyAATool::_clean()
    {
        if ( m_specularOutRTT )
        {
            KHAOS_DELETE m_specularOutRTT;
            m_specularOutRTT = 0;
        }
    }

    bool GlossyAATool::_prepareSpecularMap()
    {
        // 准备一张和法线贴图一样大的，mip层次一样的rtt图
        m_normalMap = m_mtr->getAttrib<NormalMapAttrib>();
        khaosAssert( m_normalMap );

        int destLevels = _getDestMipLevels();
        if ( destLevels <= 0 ) // 原图太小不需要
            return false;

        Texture* texNormalMap = m_normalMap->getTexture();

        TexObjCreateParas context;
        context.type   = TEXTYPE_2D;
        context.format = PIXFMT_A8R8G8B8;
        context.width  = texNormalMap->getWidth();
        context.height = texNormalMap->getHeight();
        context.levels = destLevels;
        context.usage  = TEXUSA_RENDERTARGET;

        m_specularOutRTT = g_renderDevice->createTextureObj();

        if ( !m_specularOutRTT->create( context ) )
        {
            khaosLogLn( KHL_L1, "_prepareSpecularMap failed" );
            return false;
        }

        return true;
    }

    int GlossyAATool::_getDestMipLevels()
    {
        const int MinMipSize = 16;

        Texture* texNormalMap = m_normalMap->getTexture();

        for ( int i = 0; i < texNormalMap->getLevels(); ++i )
        {
            if ( texNormalMap->getLevelWidth(i) < MinMipSize || texNormalMap->getLevelHeight(i) < MinMipSize )
                return i; // 就到这里为止
        }

        return texNormalMap->getLevels();
    }

    void GlossyAATool::_modifyAllMips()
    {
        // 修改roughness对应每一mip的法线贴图
        Texture* texNormalMap = m_normalMap->getTexture();

        EffSetterParams params;

        ImageSpecularAAProcess aaProc;

        aaProc.setMaterial( m_mtr );
        aaProc.setOutput( m_specularOutRTT );

        for ( int i = 0; i < m_specularOutRTT->getLevels(); ++i ) // 只需做这些层级
        {
            aaProc.setCurrMip( i, texNormalMap->getLevels() ); // normal-map的mip信息
            aaProc.process( params );
        }
    }

    void GlossyAATool::_saveResults()
    {
        String newFileName;

        // 如果有specularmap的取它作为名字依据
        if ( SpecularMapAttrib* mapSpec = m_mtr->getAttrib<SpecularMapAttrib>() )
        {
            newFileName = mapSpec->getTextureName();
            newFileName = getFileTitleName(newFileName); // 去除扩展名
            newFileName = newFileName + "_mr.dds"; // _mr means modify roughness
        }
        else // 没有我们以normalmap和roughness值为依据
        {
            float roughness = m_mtr->_getCurrRoughness();
            int roughness_i = (int)( roughness * 255.0f );

            newFileName = m_normalMap->getTextureName();
            newFileName = getFileTitleName(newFileName); // 去除扩展名
            newFileName = newFileName + "_r" + intToString(roughness_i) + "_mr.dds"; // _r means roughness
        }

        String fullname = g_fileSystem->getFullFileName( newFileName );
        m_specularOutRTT->save( fullname.c_str() );

        TexCfgSaver::saveSimple( newFileName, TEXTYPE_2D, PIXFMT_A8R8G8B8, TEXADDR_WRAP, TEXF_LINEAR, 
            TEXF_LINEAR, false, TexObjLoadParas::MIP_FROMFILE ); // 保存纹理描述文件

        // 数值混合到图上去了，这里不用了
        if ( RoughnessAttrib* ra = m_mtr->getAttrib<RoughnessAttrib>() )
            ra->setValue( 1.0f );

        // 使用高光贴图的roughness通道
        SpecularMapAttrib* attSpecMap = m_mtr->useAttrib<SpecularMapAttrib>();
        attSpecMap->setTexture( newFileName );
        attSpecMap->setRoughnessEnabled( true );

        // 保存文件
        MaterialExporter mtrExp;
        mtrExp.exportMaterial( m_mtr->getResFileName(), m_mtr );
    }


    //////////////////////////////////////////////////////////////////////////
    void GlossyAABatchCmd::doCmd()
    {
        const ResourceMap& allRes = MaterialManager::getAllMaterials();

        KHAOS_FOR_EACH_CONST( ResourceMap, allRes, it )
        {
            Material* mtr = static_cast<Material*>(it->second);
            GlossyAATool tool;
            tool.build( mtr );
        }
    }
}

