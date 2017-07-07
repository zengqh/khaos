#pragma once
#include "KhaosImageProcess.h"
#include "KhaosMaterial.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class ImageProcessDownscaleZ : public ImageProcess
    {
    public:
        ImageProcessDownscaleZ();

        void setInput( TextureObj* texIn );
        void setOutput( TextureObj* texOut );

        const Vector4& getParams0() const { return m_texToTexParams0; }
        const Vector4& getParams1() const { return m_texToTexParams1; }

    private:
        void _updateParas();

    private:
        ImagePin*   m_pin;
        Vector4     m_texToTexParams0;
        Vector4     m_texToTexParams1;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessGeneralAO : public ImageProcess
    {
    public:
        ImageProcessGeneralAO();

        virtual void process( EffSetterParams& params );

        const Vector4& getParam1() const { return m_param1; }
        const Vector4& getParam2() const { return m_param2; }
        const Vector4& getParam3() const { return m_param3; }

    private:
        void _updateParams();

    private:
        ImagePin*   m_pin;
        Vector4     m_param1;
        Vector4     m_param2;
        Vector4     m_param3;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessBlurAO : public ImageProcess
    {
    public:
        ImageProcessBlurAO();

        void setInput( TextureObj* texIn );

        const Vector4& getParam1() const { return m_param1; }
        const Vector4& getParam2( bool isH ) const { return isH ? m_param2H : m_param2V; }

    private:
        virtual void _checkSettings();

    private:
        ImagePin*   m_pinBlurH;
        ImagePin*   m_pinBlurV;
        Vector4     m_param1;
        Vector4     m_param2H;
        Vector4     m_param2V;
        bool        m_dirty;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessSSAO : public ImageProcess
    {
    public:
        ImageProcessSSAO();
        virtual ~ImageProcessSSAO();

        virtual void process( EffSetterParams& params );

    private:
        ImageProcessDownscaleZ m_downScaleZHalf;
        ImageProcessDownscaleZ m_downScaleZQuarter;
        ImageProcessGeneralAO  m_generalAO;
        ImageProcessBlurAO     m_blurAO;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImgProcShadowMask : public ImageProcess
    {
    public:
        ImgProcShadowMask();

        void setOutput( TextureObj* texOut );

    private:
        ImagePin*   m_pin;
    };
}

