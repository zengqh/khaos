#pragma once
#include "KhaosVector3.h"
#include "KhaosVector2.h"
#include "KhaosSHUtil.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class RandSamples : public AllocatedObject
    {
    public:
        enum RandType
        {
            RT_Jitter,
            RT_MultiJitter,
            RT_Hammersley
        };

    private:
        typedef vector<Vector2>::type Vec2List;

    public:
        RandSamples();

        void general( int numSamples, RandType rType, FLMRandomStream* rs = 0 );

        int  getNumSamples() const { return (int)m_samples.size(); }

        const Vector2& getSample( int i ) const { return m_samples[i]; }

    private:
        void _generalJitter( int numSamples, FLMRandomStream* rs );
        void _generalMultiJitter( int numSamples, FLMRandomStream* rs );
        void _generalHammersley( int numSamples );

    private:
        Vec2List m_samples;
    };


    //////////////////////////////////////////////////////////////////////////
    class SphereSampleMapper : public AllocatedObject
    {
    public:
        enum DistributionType
        {
            EntireSphereUniform,
            HemiSphereUniform,
            HemiSphereCos,
            HemiSphereGGX
        };

        struct Sample
        {
            Sample() : idx(0), theta(0), phi(0), pdf(0) {}

            int     idx;
            Vector3 dir;
            float   theta, phi;
            float   pdf;
            float   coeff[SHMath::MAX_COEFFS];
        };

    private:
        typedef vector<Sample>::type SampleList;

    public:
        SphereSampleMapper();

    public:
        void general();

        void setRandsDistribution( RandSamples* rands, DistributionType dtype, float limitTheta = Math::POS_INFINITY );
        void setDiffuseBRDFParas( const Vector3& norm );
        void setSpecularBRDFParas( const Vector3& norm, const Vector3& wo, float roughness );
        void setCommConfig( bool enPDF, bool enSH );

        const Vector3& getNormal() const { return m_normal; }
        const Vector3& getWo() const { return m_wo; }
        float getRoughness() const { return m_roughness; }

        void resetNormalFrom( const Vector3& newNorm, const SphereSampleMapper& source );

    public:
        int getNumSamples() const { return (int)m_samples.size(); }

        const Sample& getSample( int idx ) const { return m_samples[idx]; }

    public:
        static float getGGXProbability( float roughness, float NdotH, float VdotH );

    private:
        bool _isHemiSphereSample() const;

        void _buildOneSample( int idx, int i, float x, float y );
        void _entireSphereUniformSample( float x, float y, float& theta, float& phi );
        void _hemiSphereUniformSample( float x, float y, float& theta, float& phi );
        void _hemiSphereCosSample( float x, float y, float& theta, float& phi );
        void _hemiSphereGGXSample( float x, float y, float& theta, float& phi );

        void  _tangentToWorld( const Vector3& yDir );
        float _getProbabilityInv( const Sample& smpl );
        void  _calcSHCoeff( Sample& sample );

    private:
        RandSamples*        m_rands;
        DistributionType    m_distType;

        Vector3             m_normal;
        Vector3             m_wo;
        float               m_roughness;

        float               m_limitTheta;
        bool                m_usePDF;
        bool                m_useSH;

        SampleList          m_samples;

        Vector3 m_tangentX;
        Vector3 m_tangentY;
        Vector3 m_tangentZ;
    };


    //////////////////////////////////////////////////////////////////////////
    class SphereSamples : public AllocatedObject
    {
    public:
        typedef SphereSampleMapper::Sample Sample;

        typedef void (*SphereFunc)( SphereSamples* theSS, const Sample& smpl, 
            void* context, float* vals, int groupCnt );

    public:
        SphereSamples();

        void setSamples( SphereSampleMapper* mapper );

        const Sample& getSample( int i ) { return _getSample(i); }

    public:
        void integrate( SphereFunc func, void* context, float* results, int resultsCnt );

        void projectSH( SphereFunc func, void* context, float* coeffs, int coeffsCnt, int groupCnt );

        void prepareZHDir( SphereFunc func, void* context, float* zcoeffs, int LCount, int groupCnt );
        void projectZHDir( const Vector3& dir, const float* zcoeffs, float* coeffs, int LCount, int groupCnt );

    public:
        SphereSampleMapper* getMapper() const { return m_mapper; }

    private:
        const Sample& _getSample( int i );

    private:
        SphereSampleMapper* m_mapper;
    };


    //////////////////////////////////////////////////////////////////////////
    class PoissonDiskGen
    {
        typedef std::vector<Vector2> SampleArray; // not use memory ver.

    public:
        static void setKernelSize( int num );
        static int  getKernelSize();
        static const Vector2& getSample( int ind );

    private:
        static void _initSamples();
        static void _randomPoint( Vector2& p );

    private:
        static SampleArray m_pvSamples;
        static int         m_numSamples;
    };
}

