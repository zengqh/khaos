#pragma once
#include "KhaosImageProcess.h"
#include "KhaosRenderTarget.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class ImageScaleProcess : public ImageProcess
    {
    public:
        ImageScaleProcess();

        void setBigDownsample( bool en );

        void setInput( TextureObj* texIn );
        void setOutput( TextureObj* texOut );

        const Vector4& getParams0() const { return m_params0; }
        const Vector4& getParams1() const { return m_params1; }

    private:
        virtual void _checkSettings();

        bool _isInputPointFilter() const;

    private:
        ImagePin*  m_pin;

        Vector4 m_params0;
        Vector4 m_params1;

        bool m_bigDownsample;
        bool m_dirty;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageBlurProcess : public ImageProcess
    {
        typedef vector<Vector4>::type Vector4Array;

    public:
        enum
        {
            SAMPLES_COUNT = 16,
            HALF_SAMPLES_COUNT = SAMPLES_COUNT / 2
        };

    public:
        ImageBlurProcess();

    public:
        void setInput( TextureObj* texIn );
        void setDistribution( float d );
        void setScale( float s );

        const Vector4* getOffsetsH() const { return &m_offsetsH[0]; }
        const Vector4* getOffsetsV() const { return &m_offsetsV[0]; }
        const Vector4* getWeights()  const { return &m_weights[0]; }

    private:
        virtual void _checkSettings();

    private:
        ImagePin*   m_pinBlurH;
        ImagePin*   m_pinBlurV;

        Vector4Array m_offsetsH;
        Vector4Array m_offsetsV;
        Vector4Array m_weights;

        float m_distribution;
        float m_scale;

        bool m_dirty;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageCopyProcess : public ImageProcess
    {
    public:
        ImageCopyProcess();

        void resetParams();

        void setInput( TextureObj* texIn );
        void setOutput( TextureObj* texOut );

        void writeLuma( bool en ) { m_writeLuma = en; }
        bool isWriteLuma() const { return m_writeLuma; }

    private:
        ImagePin*  m_pin;
        bool       m_writeLuma;
    };
}

