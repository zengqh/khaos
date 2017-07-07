#pragma once
#include "KhaosImageProcess.h"
#include "KhaosImageProcessUtil.h"
#include "KhaosMaterial.h"

namespace Khaos
{
    class ImageScaleProcess;

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessInitLumin : public ImageProcess
    {
    public:
        ImageProcessInitLumin();

        void setInput( TextureObj* texIn );
        void setOutput( TextureObj* texOut );

        const Vector4& getParams0() const { return m_sampleLumOffsets0; }
        const Vector4& getParams1() const { return m_sampleLumOffsets1; }

    private:
        void _updateParas();

    private:
        ImagePin*   m_pin;
        Vector4     m_sampleLumOffsets0;
        Vector4     m_sampleLumOffsets1;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessLuminIter : public ImageProcess
    {
    public:
        ImageProcessLuminIter();

        void setInput( TextureObj* texIn );
        void setOutput( TextureObj* texOut );

        void setLastIter( bool isLast );

        const Vector4* getParams() const { return m_sampleLumOffsets; }

    private:
        void _updateParas();

    private:
        ImagePin*   m_pin;
        Vector4     m_sampleLumOffsets[4];
        MaterialPtr m_material;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessAdaptedLum : public ImageProcess
    {
    public:
        ImageProcessAdaptedLum();

        void setPrevLum( TextureObj* texIn );
        void setCurToneMap( TextureObj* texIn );
        void setOutput( TextureObj* texOut );
        void update();

        TextureObj* getLum0() const { return m_texPrevLum; }
        TextureObj* getLum1() const { return m_texCurToneMap; }

        const Vector4& getParam() const { return m_param; }

    private:
        void _updateParas();

    private:
        ImagePin*   m_pin;
        TextureObj* m_texPrevLum;
        TextureObj* m_texCurToneMap;
        Vector4     m_param;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessBrightPass : public ImageProcess
    {
    public:
        ImageProcessBrightPass();

        void setSrcTex( TextureObj* texIn ) { m_texSrc = texIn; }
        void setCurLumTex( TextureObj* texIn ) { m_texCurLum = texIn; }
        void setOutput( TextureObj* texOut );

        TextureObj* getSrcTex() const { return m_texSrc; }
        TextureObj* getCurLumTex() const { return m_texCurLum; }

    private:
        ImagePin*   m_pin;
        TextureObj* m_texSrc;
        TextureObj* m_texCurLum;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessFlaresPass : public ImageProcess
    {
    public:
        ImageProcessFlaresPass();

        void setBlooms( TextureObj* texIn[3] );
        void setOutput( TextureObj* texOut );

        TextureObj* getBlooms( int i ) const { return m_texBlooms[i]; }

    private:
        ImagePin*   m_pin;
        TextureObj* m_texBlooms[3];
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessFinalScene : public ImageProcess
    {
    public:
        ImageProcessFinalScene();
        virtual ~ImageProcessFinalScene();

        float getGammaAdjVal() const;

    private:
        virtual void _checkSettings();

    private:
        ImagePin*   m_pinToMainBuf;
        ImagePin*   m_pinToTmpBuf;
        MaterialPtr m_material;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessHDR : public ImageProcess
    {
    public:
        enum 
        {
            NUM_TONEMAP_TEXTURES = 4,
            NUM_CURRLUMIN_TEXTURES = 8,
            MASK_CURRLUMIN_MAX = NUM_CURRLUMIN_TEXTURES - 1
        };

    public:
        ImageProcessHDR();
        virtual ~ImageProcessHDR();

        void process( EffSetterParams& params, TextureObj* inBuffer, TextureObj* outBuffer, bool isPriorToFXAA );

    public:
        TextureObj* _getInBuf() const { return m_inBuffer; }
        TextureObj* _getOutBuf() const { return m_outBuffer; }

        TextureObj* _getTargetHalf() const { return m_texTargetHalf; }
        TextureObj* _getTargetQuarter() const { return m_texTargetQuarter; }
        TextureObj* _getTargetQuarterPost() const { return m_texTargetQuarterPost; }
        TextureObj* _getTargetEighth() const { return m_texTargetEighth; }
        TextureObj* _getTargetSixteenth() const { return m_texTargetSixteenth; }

        TextureObj* _getFinalBloomQuarter() const { return m_texFinalBloomQuarter; }

        TextureObj* _getToneMap( int idx ) const { return m_texToneMaps[idx]; }
        TextureObj* _getCurrLumin() const { return m_curLumTexture; }

        const Vector4& _getParam( int idx ) const { return m_params[idx]; }

        bool _isWriteLuma() const { return m_isWriteLuma; }

    private:
        void _downsampleHDRTarget( EffSetterParams& params );

        void _measureLuminance( EffSetterParams& params );
        void _eyeAdaptation( EffSetterParams& params );

        void _bloomAndFlares( EffSetterParams& params );

        void _buildToneMaps();
        void _cleanToneMaps();

        void _buildAdaptedLuminMaps();
        void _cleanAdaptedLuminMaps();

        void _updateParams();

    private:
        ImageScaleProcess m_scaleTargetHalf;
        ImageScaleProcess m_scaleTargetQuarter;

        ImageProcessInitLumin  m_initLumin;
        ImageProcessLuminIter  m_luminIter;
        ImageProcessAdaptedLum m_adaptedLum;

        ImageProcessBrightPass m_brightpass;
        ImageBlurProcess       m_blurBloom_4;
        ImageBlurProcess       m_blurBloom_8;
        ImageBlurProcess       m_blurBloom_16;
        ImageScaleProcess      m_scaleBloom_8;
        ImageScaleProcess      m_scaleBloom_16;
        ImageProcessFlaresPass m_flares;

        ImageProcessFinalScene m_finalScene;

        TextureObj* m_inBuffer;
        TextureObj* m_outBuffer;

        TextureObj*        m_texTargetHalf;
        TextureObj*        m_texTargetQuarter;
        TextureObj*        m_texTargetQuarterPost;
        TextureObj*        m_texTargetEighth;
        TextureObj*        m_texTargetSixteenth;
        TextureObj*        m_texFinalBloomQuarter;

        TextureObj*        m_texToneMaps[NUM_TONEMAP_TEXTURES];

        TextureObj*        m_texAdaptedLuminCur[NUM_CURRLUMIN_TEXTURES];
        TextureObj*        m_curLumTexture;
        int                m_curLumTextureTick;

        Vector4 m_params[16];

        bool m_isWriteLuma;
    };
}

