#include "KhaosPreHeaders.h"
#include "KhaosSampleUtil.h"
#include "KhaosMath.h"
#include "KhaosBRDF.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    RandSamples::RandSamples()
    {
    }

    void RandSamples::general( int numSamples, RandType rType, FLMRandomStream* rs )
    {
        // �����������
        switch ( rType )
        {
        case RT_Jitter:
            _generalJitter( numSamples, rs );
            break;

        case RT_MultiJitter:
            _generalMultiJitter( numSamples, rs );
            break;

        case RT_Hammersley:
            _generalHammersley( numSamples );
            break;

        }
    }

    void RandSamples::_generalJitter( int numSamples, FLMRandomStream* rs )
    {
        // ����������
        int sqrtNum = (int)Math::sqrt( (float)numSamples );
        numSamples = sqrtNum * sqrtNum;
        m_samples.resize( numSamples );

        const float invNumSamples = 1.0f / sqrtNum;

        // ����
        int k = 0;
    
        for ( int a = 0; a < sqrtNum; ++a )
        {
            for ( int b = 0; b < sqrtNum; ++b )
            {
                Vector2& sample = m_samples[k];

                sample.x = (a + rs->getFraction()) * invNumSamples; // [0, 1)
                sample.y = (b + rs->getFraction()) * invNumSamples; // [0, 1)

                // next
                ++k;
            }
        }
    }

    void RandSamples::_generalMultiJitter( int numSamples, FLMRandomStream* rs )
    {
        // ����������
        int sqrtNum = (int)Math::sqrt( (float)numSamples );
        numSamples = sqrtNum * sqrtNum;
        m_samples.resize( numSamples );

        const float subCellWidth = 1.0f / numSamples;

        // ��ʼ�������������߸����㷨����P85
        for ( int j = 0; j < sqrtNum; ++j )
        {
            for ( int i = 0; i < sqrtNum; ++i )
            {
                Vector2& sample = m_samples[j * sqrtNum + i];

                sample.x = (j * sqrtNum + i) * subCellWidth + rs->getFraction() * subCellWidth;
                sample.y = (i * sqrtNum + j) * subCellWidth + rs->getFraction() * subCellWidth;
            }
        }

        // ��ϴx����
        for ( int j = 0; j < sqrtNum; ++j )
        {
            for ( int i = 0; i < sqrtNum; ++i )
            {
                int k = rs->getRandRange(i, sqrtNum - 1);
                swapVal( m_samples[j * sqrtNum + i].x, m_samples[j * sqrtNum + k].x );
            }
        }
       
        // ��ϴy����
        for ( int j = 0; j < sqrtNum; ++j )
        {
            for ( int i = 0; i < sqrtNum; ++i )
            {
                int k = rs->getRandRange(i, sqrtNum - 1);
                swapVal( m_samples[i * sqrtNum + j].y, m_samples[k * sqrtNum + j].y );
            }
        }
    }

    void RandSamples::_generalHammersley( int numSamples )
    {
        m_samples.resize( numSamples );

        const float invNumSamples = 1.0f / numSamples;

        for ( uint32 i = 0; i < (uint32)numSamples; ++i )
        {
            Vector2& sample = m_samples[i];

            sample.x = (float)i * invNumSamples;
            sample.y = Math::reverseBase2FltFast( i );
        }
    }

    //////////////////////////////////////////////////////////////////////////
    SphereSampleMapper::SphereSampleMapper() : 
        m_rands(0), m_distType(EntireSphereUniform),
        m_roughness(0),
        m_limitTheta(Math::POS_INFINITY), m_usePDF(false), m_useSH(false)
    {
    }

    void SphereSampleMapper::setRandsDistribution( RandSamples* rands, DistributionType dtype, float limitTheta )
    {
        m_rands = rands;
        m_distType = dtype;
        m_limitTheta = limitTheta;
    }

    void SphereSampleMapper::setDiffuseBRDFParas( const Vector3& norm ) 
    {
        m_normal = norm.normalisedCopy(); 
    } 

    void SphereSampleMapper::setSpecularBRDFParas( const Vector3& norm, const Vector3& wo, float roughness )
    {
        m_normal = norm.normalisedCopy(); 
        m_wo = wo.normalisedCopy();
        m_roughness = roughness;
    }

    void SphereSampleMapper::setCommConfig( bool enPDF, bool enSH )
    {
        m_usePDF = enPDF; 
        m_useSH = enSH;
    }

    void SphereSampleMapper::general()
    {
        int totalSampleCount = m_rands->getNumSamples();
        m_samples.resize( totalSampleCount );

        // �����пռ�
        if ( _isHemiSphereSample() )
        {
            // ����ģʽ�ӱ��ؿռ�ת��ȫ�ֿռ�
            _tangentToWorld( m_normal );
        }

        // ����
        int idx = 0;

        for ( int i = 0; i < m_rands->getNumSamples(); ++i )
        {
            const Vector2& sample = m_rands->getSample(i);
            _buildOneSample( idx, i, sample.x, sample.y );
            ++idx;
        }
    }

    void SphereSampleMapper::resetNormalFrom( const Vector3& newNorm1, const SphereSampleMapper& source )
    {
        // �����ڰ���ģ��
        if ( !_isHemiSphereSample() )
            return;

        // �жϷ����Ƿ��Ѿ�һ��
        Vector3 newNorm = newNorm1.normalisedCopy();

        const float e = 1e-5f; // �����ǵ�λ����������ֻ��Ƚ�2������
        if ( m_normal.positionEquals(newNorm, e) )
            return;

        // ת������
        m_normal = newNorm;
        _tangentToWorld( newNorm );

        // ����ֻҪ�������򣬲��ٶ��ֲ�����
        khaosAssert( this->getNumSamples() == source.getNumSamples() );
        for ( int i = 0; i < source.getNumSamples(); ++i )
        {
            const Sample& smpl = source.getSample(i);
            this->m_samples[i].dir = m_tangentX * smpl.dir.x + m_tangentY * smpl.dir.y + m_tangentZ * smpl.dir.z;
        }
    }

    bool SphereSampleMapper::_isHemiSphereSample() const
    {
        return m_distType != EntireSphereUniform;
    }
    
    void SphereSampleMapper::_buildOneSample( int idx, int i, float x, float y )
    {
        // �����ӳ�䵽��γ
        float theta = 0, phi = 0;

        if ( m_distType == EntireSphereUniform )
            _entireSphereUniformSample( x, y, theta, phi );
        else if ( m_distType == HemiSphereUniform )
            _hemiSphereUniformSample( x, y, theta, phi );
        else if ( m_distType == HemiSphereCos )
            _hemiSphereCosSample( x, y, theta, phi );
        else if ( m_distType == HemiSphereGGX )
            _hemiSphereGGXSample( x, y, theta, phi );
        else
            khaosAssert(0);

        // ������ֵ
        Sample& smpl = m_samples[idx];

        smpl.idx   = i;

        smpl.theta = Math::minVal(theta, m_limitTheta);
        smpl.phi   = phi;

        float sin_theta = Math::sin(smpl.theta);
        smpl.dir.x = sin_theta * Math::cos(smpl.phi);
        smpl.dir.z = sin_theta * Math::sin(smpl.phi);
        smpl.dir.y = Math::cos(smpl.theta);

        if ( _isHemiSphereSample() )
        {
            // ����ģʽ�ӱ��ؿռ�ת��ȫ�ֿռ�
            smpl.dir = m_tangentX * smpl.dir.x + m_tangentY * smpl.dir.y + m_tangentZ * smpl.dir.z;
        }

        // ÿ��������
        if ( m_usePDF )
            smpl.pdf = _getProbabilityInv( smpl );
        else
            smpl.pdf = 1;

        // ˳����һ��ϵ��
        if ( m_useSH )
            _calcSHCoeff( smpl );
    }

    void SphereSampleMapper::_entireSphereUniformSample( float x, float y, float& theta, float& phi )
    {
        // ��������Ȳ���
        theta = Math::acos(1.0f - 2.0f * x); // [0, PI]
        phi   = Math::TWO_PI * y; // [0, 2PI]
    }

    void SphereSampleMapper::_hemiSphereUniformSample( float x, float y, float& theta, float& phi )
    {
        // �������Ȳ���
        theta = Math::acos(x); // [0, PI]
        phi   = Math::TWO_PI * y; // [0, 2PI]
    }

    void SphereSampleMapper::_hemiSphereCosSample( float x, float y, float& theta, float& phi )
    {
        // �������ҷֲ�����
        // �ο������߸����㷨������������˵���ػ��ֽ���= =
        float nx = Math::maxVal(1.0f - x, 0.0f); // ԭ���ϲ��Ḻ����ֻΪ��ȫ
        nx = Math::sqrt( nx );

        theta = Math::acos(nx); // [0, PI/2]
        phi   = Math::TWO_PI * y; // [0, 2PI]
    }

    void SphereSampleMapper::_hemiSphereGGXSample( float x, float y, float& theta, float& phi )
    {
        // ggx�ķֲ�����Microfacet Models for Refraction through Rough Surfaces
        // ue4����ͬ������Ҫ�Բ���
        // ע�⣬������h�ķֲ�
        float nx = Math::maxVal( x / (1.0f - x), 0.0f ); // safe here
        float m  = M_FromR( m_roughness ); // ͬue4�ٶ��巽��
        float t  = m * Math::sqrt( nx );

        theta = Math::atan( t );  // [0, PI/2]
        phi   = Math::TWO_PI * y; // [0, 2PI]
    }

    float SphereSampleMapper::getGGXProbability( float roughness, float NdotH, float VdotH )
    {
        // ggx�ֲ�
        // ��H��pdf
        // PDF_h = D_GGX(h) * dot(n,h)
        float m = M_FromR( roughness );
        float pdf_h = D_GGX(m, NdotH) * NdotH;

        // ����ת��ΪL��pdf
        // PDF_i = PDF_h / (4 * V.H)
        float pdf_i = pdf_h / (4.0f * VdotH);
        return pdf_i;
    }

    float SphereSampleMapper::_getProbabilityInv( const Sample& smpl )
    {
        // ����wi�ĸ��ʵ���
        if ( m_distType == EntireSphereUniform )
        {
            // ���ȷֲ�������һ��Ϊ����浹��
            return 4 * Math::PI;
        }

        if ( m_distType == HemiSphereUniform )
        {
            return 2 * Math::PI;
        }

        if ( m_distType == HemiSphereCos )
        {
            // ���ҷֲ�
            // pdf = cos(t) / pi
            float cos_t = Math::cos(smpl.theta);

            float pdfinv;
            if ( cos_t > 1e-5f )
                pdfinv = Math::PI / cos_t;
            else
                pdfinv = 0; // ���Ե�һЩ����ϵĵ�

            return pdfinv;
        }

        if ( m_distType == HemiSphereGGX )
        {
            // ggx�ֲ�
            // ��H��pdf
            // PDF_h = D_GGX(h) * dot(n,h)
            float NdotH = Math::saturateCos(smpl.theta);
            float m = M_FromR( m_roughness );
            float pdf_h = D_GGX(m, NdotH) * NdotH;

            // ����ת��ΪL��pdf
            // PDF_i = PDF_h / (4 * V.H)
            float VdotH = Math::saturate(m_wo.dotProduct(smpl.dir)); // ����ԭ����pdf��H�ķֲ��������dirҲ����H
            float pdf_i = pdf_h / (4.0f * VdotH);

            khaosAssert( pdf_i >= 0 );

            float pdfinv;

            if ( pdf_i > 1e-5f )
                pdfinv = 1 / pdf_i;
            else
                pdfinv = 0;

            return pdfinv;
        }

        khaosAssert(0);
        return 0;
    }

    void SphereSampleMapper::_tangentToWorld( const Vector3& yDir )
    {
        // yDirLocal(0,1,0)  =>  yDir

        m_tangentY = yDir;

        m_tangentX = Math::fabs( yDir.x ) < 0.999f ? Vector3::UNIT_X : Vector3::UNIT_Z;
        m_tangentZ = m_tangentX.crossProduct( m_tangentY ).normalisedCopy();
        m_tangentX = m_tangentY.crossProduct( m_tangentZ );
    }

    void SphereSampleMapper::_calcSHCoeff( Sample& sample )
    {
        float x = sample.dir.x;
        float y = sample.dir.y;
        float z = sample.dir.z;

#define _CL_COEF(i) sample.coeff[i] = SHMath::sh_func_##i##( x, y, z )

        _CL_COEF(0);  _CL_COEF(1);  _CL_COEF(2);  _CL_COEF(3);
        _CL_COEF(4);  _CL_COEF(5);  _CL_COEF(6);  _CL_COEF(7);
        _CL_COEF(8);  _CL_COEF(9);  _CL_COEF(10); _CL_COEF(11);
        _CL_COEF(12); _CL_COEF(13); _CL_COEF(14); _CL_COEF(15);
    }


    //////////////////////////////////////////////////////////////////////////
    SphereSamples::SphereSamples() : m_mapper(0)
    {
    }

    void SphereSamples::setSamples( SphereSampleMapper* mapper )
    {
        m_mapper = mapper;
    }

    const SphereSamples::Sample& SphereSamples::_getSample( int i )
    {
        return m_mapper->getSample( i );
    }

    void SphereSamples::integrate( SphereFunc func, void* context, float* results, int resultsCnt )
    {
        // ��һ�����������ؿ������

        const int MAX_SAMPLES = m_mapper->getNumSamples();
        
        khaosAssert( resultsCnt >= 1 );

        KHAOS_CLEAR_ZERO( results, resultsCnt * sizeof(float) );

        vector<float>::type vals(resultsCnt, 0);

        for ( int i = 0; i < MAX_SAMPLES; ++i )
        {
            const Sample& sample = _getSample(i);

            func( this, sample, context, &vals[0], resultsCnt ); // �����õ�һ������

            for ( int j = 0; j < resultsCnt; ++j )
            {
                results[j] += vals[j] * sample.pdf;
            }
        }

        const float factor = 1.0f / MAX_SAMPLES;

        for ( int i = 0; i < resultsCnt; ++i )
        {
            results[i] *= factor;
        }
    }

    void SphereSamples::projectSH( SphereFunc func, void* context, float* coeffs, int coeffsCnt, int groupCnt )
    {
        // �����ǻ������ؿ�����֣����ͶӰ��sh��
        // �ܶ�p = 1 / (4 * PI)
        // ���ÿ��ϵ����
        // c(i) = �ҡ�f(��,��)y(��,��)sin��d��d��
        //      = 1 / N * ��(f(x)/p(x))
        //      = (4 * PI / N) * ��(f(j) * y(j))

        const int MAX_SAMPLES = m_mapper->getNumSamples();

        khaosAssert( coeffsCnt == 1 || coeffsCnt == 4 || coeffsCnt == 9 || coeffsCnt == 16 );
        khaosAssert( groupCnt >= 1 );

        const int totalCoeffsCnt = groupCnt * coeffsCnt;
        KHAOS_CLEAR_ZERO( coeffs, totalCoeffsCnt * sizeof(float) );

        vector<float>::type vals(groupCnt, 0);

        for ( int i = 0; i < MAX_SAMPLES; ++i )
        {
            const Sample& sample = _getSample(i);

            func( this, sample, context, &vals[0], groupCnt ); // �����õ�һ������

            for ( int g = 0; g < groupCnt; ++g ) // ÿ��
            {
                float val     = vals[g] * sample.pdf;
                int  startIdx = g * coeffsCnt;

                for ( int j = 0; j < coeffsCnt; ++j ) // ����һ��ϵ��
                {
                    coeffs[startIdx + j] += val * sample.coeff[j];
                }
            }
        }

        const float factor = 1.0f / MAX_SAMPLES;

        for ( int i = 0; i < totalCoeffsCnt; ++i )
        {
            coeffs[i] *= factor;
        }
    }

    void SphereSamples::prepareZHDir( SphereFunc func, void* context, float* zcoeffs, int LCount, int groupCnt )
    {
        // �����ǻ������ؿ�����֣����ͶӰ��zh��
        // G(l,m) = sqrt(4pi/(2*l+1)) * f(l,0) * y(l,m)(d)
        // ���������ǰ�벿��

        const int MAX_SAMPLES = m_mapper->getNumSamples();

        khaosAssert( 1 <= LCount && LCount <= 4 );
        khaosAssert( groupCnt >= 1 );

        const int totalCoeffsCnt = groupCnt * LCount;
        KHAOS_CLEAR_ZERO( zcoeffs, totalCoeffsCnt * sizeof(float) );

        vector<float>::type vals(groupCnt, 0);

        // ��f(l,0)
        for ( int i = 0; i < MAX_SAMPLES; ++i )
        {
            const Sample& sample = _getSample(i);

            func( this, sample, context, &vals[0], groupCnt ); // �����õ�һ������

            for ( int g = 0; g < groupCnt; ++g ) // ÿ��
            {
                float val     = vals[g] * sample.pdf;
                int  startIdx = g * LCount;

                // ͶӰ��zh��,zh��m=0������ֵ
                for ( int L = 0; L < LCount; ++L )
                {
                    int baseIdx = SHMath::getBasisIndex( L, 0 );
                    zcoeffs[startIdx+L] += val * sample.coeff[baseIdx];
                }
            }
        }

        // ����G(l,m)ǰ�벿
        #define factor_(i) (Math::sqrt(4 * Math::PI / (2 * i + 1)) / MAX_SAMPLES)

        static const float s_preScale[4] =
        {
            factor_(0), factor_(1), factor_(2), factor_(3)
        };

        #undef factor_

        for ( int g = 0; g < groupCnt; ++g ) // ÿ��
        {
            int  startIdx = g * LCount;

            for ( int L = 0; L < LCount; ++L )
                zcoeffs[startIdx+L] *= s_preScale[L];
        }
    }

    void SphereSamples::projectZHDir( const Vector3& dir, const float* zcoeffs, float* coeffs, int LCount, int groupCnt )
    {
        const int coeffsCount = SHMath::getTotalCoeffsCount(LCount);

        int i = 0;

        for ( int L = 0; L < LCount; ++L )
        {
            for ( int m = -L; m <= L; ++m )
            {
                float y = SHMath::sh_func( i, dir.x, dir.y, dir.z );
                
                for ( int g = 0; g < groupCnt; ++g ) // ÿ��
                {
                    float zh = zcoeffs[g * LCount + L];
                    coeffs[g * coeffsCount + i] = zh * y;
                }

                ++i;
            }
        }
    }


    //////////////////////////////////////////////////////////////////////////
    PoissonDiskGen::SampleArray PoissonDiskGen::m_pvSamples;
    int PoissonDiskGen::m_numSamples = 0;

    void PoissonDiskGen::setKernelSize( int num )
    {
        if ( m_numSamples != num && num > 0 )
        {
            m_numSamples = num;

            m_pvSamples.resize( m_numSamples );

            _initSamples();
        }
    }

    int PoissonDiskGen::getKernelSize()
    {
        return m_numSamples;
    }

    const Vector2& PoissonDiskGen::getSample( int ind )
    {
        return m_pvSamples[ind];
    }

    // samples distance-based sorting
    struct _SamplesDistaceSort
    {
        bool operator()( const Vector2& samplA, const Vector2& samplB ) const
        {
            float R2sampleA = samplA.x*samplA.x + samplA.y*samplA.y;
            float R2sampleB = samplB.x*samplB.x + samplB.y*samplB.y;

            return (R2sampleA < R2sampleB);
        }
    };

    void PoissonDiskGen::_initSamples()
    {
        const int nQ = 1000;

        _randomPoint( m_pvSamples[0] );

        for ( int i = 1; i < m_numSamples; ++i )
        {
            float dmax = -1.0f;

            for ( int c = 0; c < i * nQ; ++c )
            {
                Vector2 curSample;
                _randomPoint (curSample);

                float dc = 2.0f;

                for ( int j = 0; j < i; ++j )
                {
                    float dj =
                        (m_pvSamples[j].x - curSample.x) * (m_pvSamples[j].x - curSample.x) +
                        (m_pvSamples[j].y - curSample.y) * (m_pvSamples[j].y - curSample.y);

                    if ( dc > dj )
                        dc = dj;
                }

                if ( dc > dmax )
                {
                    m_pvSamples[i] = curSample;
                    dmax = dc;
                }
            }
        }

        for ( int i = 0; i < m_numSamples; ++i )
        {
            m_pvSamples[i] *= 2.0f;
        }

        // samples sorting
        std::stable_sort( m_pvSamples.begin(), m_pvSamples.end(), _SamplesDistaceSort() );
    }

    void PoissonDiskGen::_randomPoint( Vector2& p )
    {
        // generate random point inside circle
        do
        {
            p.x = Math::unitRandom() - 0.5f;
            p.y = Math::unitRandom() - 0.5f;
        }
        while ( p.x*p.x + p.y*p.y > 0.25f );
    }
}

