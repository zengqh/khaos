#include "KhaosPreHeaders.h"
#include "KhaosImageProcessUtil.h"
#include "KhaosTextureObj.h"
#include "KhaosEffectID.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    ImageScaleProcess::ImageScaleProcess() : 
        m_bigDownsample(false), m_dirty(true)
    {
        m_pin = _createImagePin( ET_COMM_SCALE );
        m_pin->setOwnOutput();
        m_pin->setRttFormat( PIXFMT_INVALID );
        _setRoot( m_pin );
    }

    bool ImageScaleProcess::_isInputPointFilter() const
    {
        bool canLinear = m_bigDownsample;

#if 0
        if ( m_coarseDepth )
            canLinear = false;

        if ( m_customPoint )
            canLinear = false;
#endif
        return !canLinear; 
    }

    void ImageScaleProcess::setBigDownsample( bool en )
    {
        m_bigDownsample = en;
        m_dirty = true;

        m_pin->setInputFilter( _isInputPointFilter() );
    }

    void ImageScaleProcess::setInput( TextureObj* texIn )
    {
        m_pin->setInput( texIn );
        m_dirty = true;
    }

    void ImageScaleProcess::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
        m_dirty = true;
    }

    void ImageScaleProcess::_checkSettings()
    {
        if ( !m_dirty )
            return;

        m_dirty = false;

        TextureObj* offsetTex = m_bigDownsample ? m_pin->getOutput()->getRTT() : m_pin->getInput();

        float s1 = 0.5f / offsetTex->getWidth();  // 2.0 better results on lower res images resizing        
        float t1 = 0.5f / offsetTex->getHeight();       

        if ( m_bigDownsample )
        {
            // Use rotated grid + middle sample (~quincunx)
            m_params0 = Vector4(s1*0.96f, t1*0.25f, -s1*0.25f, t1*0.96f); 
            m_params1 = Vector4(-s1*0.96f, -t1*0.25f, s1*0.25f, -t1*0.96f);  
        }
        else
        {
            // Use box filtering (faster - can skip bilinear filtering, only 4 taps)
            m_params0 = Vector4(-s1, -t1, s1, -t1); 
            m_params1 = Vector4(s1, t1, -s1, t1);    
        }
    }

    //////////////////////////////////////////////////////////////////////////
    ImageBlurProcess::ImageBlurProcess() : m_pinBlurH(0), m_pinBlurV(0), m_dirty(true)
    {
        m_pinBlurH = _createImagePin( ET_COMM_BLUR );
        m_pinBlurH->setInputFilter( false ); // 线性过滤
        m_pinBlurH->setOwnOutput(); // 使用内部rtt，不用clear
        m_pinBlurH->setFlag((void*)0);

        m_pinBlurV = _createImagePin( ET_COMM_BLUR );
        m_pinBlurV->setInputFilter( false ); // 线性过滤
        m_pinBlurV->setOwnOutput(); // 使用内部rtt，不用clear
        m_pinBlurV->setRttFormat( PIXFMT_INVALID ); // 不用再创建tex了，结果返回给原始图
        m_pinBlurV->setFlag((void*)1);

        // 链接：blurH -> blurV
        m_pinBlurV->addChild( m_pinBlurH );
        _setRoot( m_pinBlurV );
    }

    void ImageBlurProcess::setInput( TextureObj* texIn )
    {
        m_pinBlurH->setInput( texIn );
        m_pinBlurH->setRequestRttSize( IntVector2(texIn->getWidth(), texIn->getHeight()) );
        m_pinBlurH->setRttFormat( texIn->getFormat() );

        m_pinBlurV->getOutput()->linkRTT( texIn );

        m_dirty = true;
    }

    void ImageBlurProcess::setDistribution( float d )
    {
        m_distribution = d;
        m_dirty = true;
    }

    void ImageBlurProcess::setScale( float s )
    {
        m_scale = s;
        m_dirty = true;
    }

    void ImageBlurProcess::_checkSettings()
    {
        if ( !m_dirty )
            return;

        m_dirty = false;

        // setup texture offsets, for texture sampling
        float s1 = 1.0f / (float) m_pinBlurH->getInput()->getWidth();     
        float t1 = 1.0f / (float) m_pinBlurH->getInput()->getHeight();    

        // Horizontal/Vertical pass params
        Vector4Array& pHParams   = m_offsetsH;
        Vector4Array& pVParams   = m_offsetsV;
        Vector4Array& pWeightsPS = m_weights;

        vector<float>::type pWeights;
        float fWeightSum = 0;

        pHParams.resize(HALF_SAMPLES_COUNT);
        pVParams.resize(HALF_SAMPLES_COUNT);
        pWeightsPS.resize(HALF_SAMPLES_COUNT);
        pWeights.resize(SAMPLES_COUNT);

        // 计算高斯分布权重
        for ( int s = 0; s < SAMPLES_COUNT; ++s )
        {
            if ( m_distribution != 0.0f )
                pWeights[s] = Math::gaussianDistribution1D( float(s - HALF_SAMPLES_COUNT), m_distribution );      
            else
                pWeights[s] = 0.0f;

            fWeightSum += pWeights[s];
        }

        // 单位化权重
        for ( int s = 0; s < SAMPLES_COUNT; ++s )
        {
            pWeights[s] /= fWeightSum;  
        }

        // set bilinear offsets
        const Vector4 vWhite( 1.0f, 1.0f, 1.0f, 1.0f );

        for ( int s = 0; s < HALF_SAMPLES_COUNT; ++s )
        {
            // 取偶数权重
            float off_a = pWeights[s*2]; // 0, 2, 4, 14

            // 取单数权重
            khaosAssert( (s*2+1) <= SAMPLES_COUNT-1 ); // 1, 3, 5, 15
            float off_b = pWeights[s*2+1]; 

            // offset = 单数权重 / (偶数权重 + 单数权重);
            float a_plus_b = (off_a + off_b);
            if (a_plus_b == 0)
                a_plus_b = 1.0f;
            float offset = off_b / a_plus_b;

            // 像素权重 = (偶数权重 + 单数权重) * 缩放系数
            pWeights[s] = off_a + off_b;
            pWeights[s] *= m_scale;
            pWeightsPS[s] = vWhite * pWeights[s];

            // 水平和垂直的采样偏移
            float fCurrOffset = (s * 2 - HALF_SAMPLES_COUNT) + offset ; // +offset由权重决定
            pHParams[s] = Vector4(s1 * fCurrOffset , 0, 0, 0);  
            pVParams[s] = Vector4(0, t1 * fCurrOffset , 0, 0);       
        }
    }

    //////////////////////////////////////////////////////////////////////////
    ImageCopyProcess::ImageCopyProcess() : m_writeLuma(false)
    {
        m_pin = _createImagePin( ET_COMM_COPY );
        m_pin->setInputFilterEx( TextureFilterSet::NEAREST );
        _setRoot( m_pin );
    }

    void ImageCopyProcess::resetParams()
    {
        writeLuma( false );
    }

    void ImageCopyProcess::setInput( TextureObj* texIn )
    {
        m_pin->setInput( texIn );
    }

    void ImageCopyProcess::setOutput( TextureObj* texOut )
    {
        if ( texOut )
        {
            m_pin->setOwnOutput();
            m_pin->getOutput()->linkRTT( texOut );
            m_pin->useMainRTT( false );
        }
        else
        {
            m_pin->clearOwnOutput();
            m_pin->useMainRTT( true );
        }
    }
}

