#pragma once
#include <KhaosRoot.h>
#include <KhaosSampleUtil.h>

class CCubeMapProcessor;
class CImageSurface;

namespace Khaos
{
    class EnvMapBuilder
    {
    public:
        enum 
        {
            QUALITY_METHOD,
            FAST_METHOD
        };

    public:
        EnvMapBuilder();
        ~EnvMapBuilder();

    public:
        void init( int buildMethod, const String& cubeFile, int outSize, int maxMips );
        void buildEnvDiffuseIBL();
        void buildEnvSpecularIBL();
        void save( const String& outFile );

    private:
        void _testSave( const String& outFile );

        void _initInputData();
        void _buildSpecIBL_EachTexel( int u, int v );
        void _buildDiffuseIBL_EachTexel( int u, int v );
        void _fixupEdge();

        void _calcSpecularGetImportant( const Vector3& vR, Vector3& totalClr, float& totalWeight );

        static bool _perSpecularGet( CCubeMapProcessor* proc, void* context, int face, int u, int v, 
            const float* vDir, float* tmpSampleVal, double& deltaArea );

        static bool _perDiffuseGet( CCubeMapProcessor* proc, void* context, int face, int u, int v, 
            const float* vDir, float* tmpSampleVal, double& deltaArea );

        static void _perDiffuseGetImportant( SphereSamples* theSS, 
            const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt );

    private:
        Texture* m_obj;
        CCubeMapProcessor* m_process;
        int m_processMethod;

        // temp data
        RandSamples m_rands;
        SphereSampleMapper m_mapper;
        SphereSamples m_theSS;

        int   m_currLevel;
        int   m_currMapSize;
        int   m_currFace;
        float m_currRoughness;
        
        CImageSurface* m_currSurfaceDest;
    };
}

