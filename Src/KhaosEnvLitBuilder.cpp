#include "KhaosPreHeaders.h"
#include "KhaosEnvLitBuilder.h"
#include "KhaosTextureManager.h"
#include "KhaosRenderDevice.h"
#include "KhaosTexCfgParser.h"
#include "KhaosSampleUtil.h"
#include "KhaosBRDF.h"
//#include "KhaosLight.h"

namespace Khaos
{
    const uint32 NumSamples = 128;

    //////////////////////////////////////////////////////////////////////////
    void _buildEnvLUT_UE4( float* destBuff, int mapSizeX, int mapSizeY )
    {
        // x is NoV, y is roughness
        for ( int y = 0; y < mapSizeY; ++y )
        {
            float Roughness = (float)(y + 0.5f) / mapSizeY;
            float m = Roughness * Roughness;
            float m2 = m*m;

            for ( int x = 0; x < mapSizeX; ++x )
            {
                float NoV = (float)(x + 0.5f) / mapSizeX;

                Vector3 V;
                V.x = Math::sqrt(1.0f - NoV * NoV);	// sin
                V.y = 0.0f;
                V.z = NoV;		                    // cos

                float A = 0.0f;
                float B = 0.0f;

                for ( uint32 i = 0; i < NumSamples; ++i )
                {
                    float E1 = (float)i / NumSamples;
                    float E2 = Math::reverseBase2FltFast(i);

                    // A & B
                    {
                        float Phi = 2.0f * Math::PI * E1;
                        float CosPhi = Math::cos(Phi);
                        float SinPhi = Math::sin(Phi);
                        float CosTheta = Math::sqrt((1.0f - E2) / (1.0f + (m2 - 1.0f) * E2));
                        float SinTheta = Math::sqrt(1.0f - CosTheta * CosTheta);

                        Vector3 H(SinTheta * Math::cos(Phi), SinTheta * Math::sin(Phi), CosTheta);
                        Vector3 L = 2.0f * V.dotProduct(H) * H - V;

                        float NoL = Math::maxVal(L.z, 0.0f);
                        float NoH = Math::maxVal(H.z, 0.0f);
                        float VoH = Math::maxVal(V.dotProduct(H), 0.0f);

                        if ( NoL > 0.0f )
                        {
                            float Vis_SmithV = NoL * ( NoV * ( 1 - m ) + m );
                            float Vis_SmithL = NoV * ( NoL * ( 1 - m ) + m );
                            float Vis = 0.5f / ( Vis_SmithV + Vis_SmithL );

                            float NoL_Vis_PDF = NoL * Vis * (4.0f * VoH / NoH);
                            float Fc = 1.0f - VoH;
                            Fc *= Math::sqr(Fc*Fc); // Fc^5
                            A += NoL_Vis_PDF * (1.0f - Fc);
                            B += NoL_Vis_PDF * Fc;
                        }
                    } // end A & B
                } // end for ( uint32 i = 0; i < NumSamples; ++i )

                A /= NumSamples;
                B /= NumSamples;

                float* Dest = &destBuff[ (mapSizeX * y + x) * 2 ];
                Dest[0] = Math::saturate(A);
                Dest[1] = Math::saturate(B);
            } // end for ( int x = 0; x < mapSizeX; ++x )
        } // for ( int y = 0; y < mapSizeY; ++y )
    }

    //////////////////////////////////////////////////////////////////////////
    struct EnvBuildContext
    {
        float roughness;
        float m;
        float NdotV;
        Vector3 N;
        Vector3 V;
    };

    void _testSample();

    void _onEachEnvSample( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt )
    {
        // D   = D_GGX
        // Vis = Vis_SmithJointApprox
        // Fc  = Fc_Schlick

        // F = (1 - Fc).F0 + Fc

        // Y = ∫BRDF(l,v)(n.l)dw
        //   = ∫D.Vis.F.(n.l)dw
        //   = ∫D.Vis.((1 - Fc).F0 + Fc).(n.l)dw
        //   = F0.∫D.Vis.(1 - Fc)(n.l)dw + ∫D.Vis.Fc.(n.l)dw

        // Incident light = NdotL
        // pdf_h = D * NdotH
        // pdf_i = pdf_h / (4 * VdotH) = D * NdotH / (4 * VdotH)
        // D.Vis.(n.l) / pdf_i = D * Vis * NdotL / (D * NdotH / (4 * VdotH))
        //                     = Vis * NdotL * (4 * VdotH / NdotH)

        EnvBuildContext* envData = (EnvBuildContext*)context;
        
        // 我们已经把pdf算进去了，所以不用外部的pdf
        //SphereSamples::setCurrSamplePDF( smpl, 1.0f );

        const Vector3& H = smpl.dir;
        Vector3 L = Math::reflectDir( envData->V, H, true );

        float NdotL = envData->N.dotProduct( L );
        if ( NdotL <= 0 )
        {
            vals[0] = vals[1] = 0;
            return;
        }

        // Vis * NdotL * (4 * VdotH / NdotH)

        NdotL = Math::saturate( NdotL );
        
        float NdotH = envData->N.dotProduct( H );
        NdotH = Math::saturate( NdotH );

        float VdotH = envData->V.dotProduct( H );
        VdotH = Math::saturate( VdotH );

        float Vis = Vis_SmithJointApprox( envData->m, envData->NdotV, NdotL );
        float NoL_Vis_PDF = NdotL * Vis * (4 * VdotH / NdotH);
        float Fc = Fc_Schlick(VdotH);

        vals[0] = (1 - Fc) * NoL_Vis_PDF;
        vals[1] = Fc * NoL_Vis_PDF;
    }

    void buildEnvLUTMap( const String& name, int mapSize )
    {
        //_testSample();
        return;

        vector<float>::type datas;
        datas.resize( mapSize * mapSize * 2, 0.0f ); // r16g16

#if 1
        RandSamples rands;
        rands.general( NumSamples, RandSamples::RT_Hammersley );

        SphereSampleMapper mapper;
        mapper.setRandsDistribution( &rands, SphereSampleMapper::HemiSphereGGX );

        SphereSamples theSS;
        theSS.setSamples( &mapper );

        EnvBuildContext context;

        context.N = Vector3::UNIT_Y;

        for ( int i = 0; i < mapSize; ++i )
        {
            // each roughness
            context.roughness = (float)(i + 0.5f) / mapSize;
            context.m = M_FromR( context.roughness );

            khaosOutputStr( "buildEnvLUTMap: %d, %f\n", i, context.roughness );

            for ( int j = 0; j < mapSize; ++j )
            {
                // each n.v
                context.NdotV = (float)(j + 0.5f) / mapSize; // cos
                float sin_nv = Math::sqrt(1 - context.NdotV * context.NdotV);
                khaosAssert( 0 <= sin_nv && sin_nv <= 1 );
                context.V = Vector3(sin_nv, context.NdotV, 0); // 因为对称，只考虑x-y正半平面

                mapper.setSpecularBRDFParas( context.N, context.V, context.roughness );
                mapper.general();

                Vector2 results;
                theSS.integrate( _onEachEnvSample, &context, results.ptr(), 2 );

                int pixIdx = (i * mapSize + j) * 2;
                datas[pixIdx] = Math::saturate(results.x);
                datas[pixIdx+1] = Math::saturate(results.y);  // n.v, roughness => results

                if ( i == mapSize - 1 )
                    khaosOutputStr( "results: %f, %f\n", results.x, results.y );
            }
        }

#else
            _buildEnvLUT_UE4( &datas[0], mapSize, mapSize );
#endif

        // save
        String fileName = g_fileSystem->getFullFileName( name );
        TextureObj* tempTex = g_renderDevice->createTextureObj();

        TexObjCreateParas paras;
        paras.type   = TEXTYPE_2D;
        paras.usage  = TEXUSA_STATIC;
        paras.format = PIXFMT_G16R16F;
        paras.levels = 1;
        paras.width  = mapSize;
        paras.height = mapSize;

        IntRect rect(0, 0, mapSize, mapSize);
        tempTex->create( paras );
        tempTex->fillData( 0, &rect, &datas[0], 0 );
        tempTex->fetchSurface();
        tempTex->getSurface(0)->save( fileName.c_str() );

        KHAOS_DELETE tempTex;

        TexCfgSaver::saveSimple( name, TEXTYPE_2D, PIXFMT_G16R16F, TEXADDR_CLAMP, TEXF_LINEAR, 
            TEXF_POINT, false, TexObjLoadParas::MIP_AUTO ); // 保存纹理描述文件

    }

    //////////////////////////////////////////////////////////////////////////
    void _onTestFuncSample( SphereSamples* theSS, const SphereSamples::Sample& smpl, void* context, float* vals, int groupCnt )
    {
        const Vector3& norm = theSS->getMapper()->getNormal();

        float ndotl = norm.dotProduct( smpl.dir );
        if ( ndotl < 0 )
            ndotl = 0;

        *vals = ndotl;
    }

    void _testSample()
    {
        const Vector3 norm = Vector3(0.7f, -0.3f, 0.6f);

        FLMRandomStream rs(1234957);

        RandSamples rands;
        rands.general( 20, RandSamples::RT_Hammersley, &rs );

        SphereSampleMapper mapper;
        mapper.setRandsDistribution( &rands, SphereSampleMapper::HemiSphereCos );
        mapper.setDiffuseBRDFParas( norm );
        mapper.setCommConfig( true, false );
        mapper.general();

        SphereSamples theSS;
        theSS.setSamples( &mapper );
        float ret0 = 0;
        theSS.integrate( _onTestFuncSample, 0, &ret0, 1 );

        int a;
        a = 1;
    }

}

