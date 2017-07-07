#include "EnvMapBuilder.h"
#include "CCubeMapProcessor.h"
#include "KhaosSampleUtil.h"
#include "KhaosBRDF.h"
#include "KhaosTexCfgParser.h"
#include <Windows.h>
#include <tchar.h>


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    const int32 DefaultFixupType  = CP_FIXUP_WARP;
    const int32 DefaultFixupWidth = 1;

    const int SPEC_IBL_SIZE = 256;
    const int DIFF_IBL_SIZE = 64;

    //////////////////////////////////////////////////////////////////////////
    struct EnvBakeContext
    {
        EnvMapBuilder* pThis;
        Vector3        vN;
        double         weightTotal;
    };

    //////////////////////////////////////////////////////////////////////////
    uint32 _convertPixFmt( PixelFormat fmt )
    {
        uint32 cImgSurfFormat = 0;

        switch ( fmt )
        {
        case PIXFMT_A8R8G8B8:
        case PIXFMT_X8R8G8B8:
            cImgSurfFormat = CP_VAL_UNORM8_BGRA;
            break;

        case PIXFMT_A16B16G16R16:
            cImgSurfFormat = CP_VAL_UNORM16;
            break;

        case PIXFMT_A16B16G16R16F:
            cImgSurfFormat = CP_VAL_FLOAT16;
            break;

        case PIXFMT_A32B32G32R32F:
            cImgSurfFormat = CP_VAL_FLOAT32;
        }

        return cImgSurfFormat;
    }

    //////////////////////////////////////////////////////////////////////////
    EnvMapBuilder::EnvMapBuilder() : m_obj(0), m_process(new CCubeMapProcessor), m_processMethod(0)
    {
    }

    EnvMapBuilder::~EnvMapBuilder()
    {
        if ( m_obj )
            m_obj->release();

        delete m_process;
    }

    void EnvMapBuilder::init( int buildMethod, const String& cubeFile, int outSize, int maxMips )
    {
        m_processMethod = buildMethod;

        khaosAssert( !m_obj );
        m_obj = TextureManager::getOrCreateTexture( cubeFile );
        m_obj->load( false );

        if ( maxMips == 0 ) // 自动计算层数
        {
            int n = Math::log2i(outSize); // 原始层数-1
            maxMips = n - 3; // 调节后层数，限制到最低16x16大小
            maxMips = Math::maxVal( maxMips, 2 );
        }

        m_process->Init( m_obj->getWidth(), outSize, maxMips, 4 );

        _initInputData();

        if ( m_processMethod == FAST_METHOD ) // 速度模式使用重要性采样，尽可能需要大的原始图（由于采样为点采样关系）
        {
            // 保证最小的大小
#if 0
            const int INPUTSIZE_MIN = 1024;
            if ( m_process->m_InputSize < INPUTSIZE_MIN )
            {
                m_process->ResizeInput( INPUTSIZE_MIN );
            }
#endif
        }
        else // 质量模式使用完全积分，预处理一个表加快速度
        {
            // build normal angle table
            m_process->BuildNormalizerSolidAngleCubemap( m_process->m_InputSize, DefaultFixupType );
        }
    }

    void EnvMapBuilder::save( const String& outFile )
    {
        //_testSave( outFile );
        //return;

        //////////////////////////////////////////////////////////////////////////
        const float outGamma = 1.0f; // 目前输出为线性

        TextureObj* texDest = g_renderDevice->createTextureObj();

        TexObjCreateParas tcp;
        
        tcp.type   = TEXTYPE_CUBE;
        tcp.usage  = TEXUSA_STATIC;
        tcp.format = PIXFMT_A16B16G16R16F;
        tcp.width  = m_process->m_OutputSize;
        tcp.height = m_process->m_OutputSize;
        tcp.levels = m_process->m_NumMipLevels;
        
        if ( !texDest->create( tcp ) )
        {
            KHAOS_DELETE texDest;
            return;
        }

        texDest->fetchSurface();

        vector<uint8>::type buffers;
        buffers.resize( m_process->m_OutputSize * m_process->m_OutputSize * sizeof(float) * 4 );

        for ( int level = 0; level < texDest->getLevels(); ++ level )
        {
            int dataPitch = m_process->m_OutputSurface[level][0].m_Width * sizeof(float) * 4;

            for ( int face = 0; face < 6; ++face )
            {
                CImageSurface& surSrc = m_process->m_OutputSurface[level][face];
                SurfaceObj* surDest = texDest->getSurface( (CubeMapFace)face, level );

                //surSrc.GetImageData( CP_VAL_FLOAT32, 4, dataPitch, &buffers[0] );
                surSrc.GetImageDataScaleGamma( CP_VAL_FLOAT32, 4, dataPitch, &buffers[0], 1.0f, outGamma );
                surDest->fillData( 0, &buffers[0], dataPitch );
            }
        }
        
        String fullFile = g_fileSystem->getFullFileName( outFile );
        texDest->save( fullFile.c_str() );
        KHAOS_DELETE texDest;
        
        TexCfgSaver::saveSimple( outFile, TEXTYPE_CUBE, tcp.format, TEXADDR_CLAMP, TEXF_LINEAR, 
            TEXF_LINEAR, false, TexObjLoadParas::MIP_FROMFILE ); // 保存纹理描述文件
    }

    void EnvMapBuilder::_testSave( const String& outFile )
    {
        // 测试mipmap生成
        m_process->GeneralInputMips();

        const float outGamma = 1.0f; // 目前输出为线性

        TextureObj* texDest = g_renderDevice->createTextureObj();

        TexObjCreateParas tcp;

        tcp.type   = TEXTYPE_CUBE;
        tcp.usage  = TEXUSA_STATIC;
        tcp.format = PIXFMT_A16B16G16R16F;
        tcp.width  = m_process->m_InputSize;
        tcp.height = m_process->m_InputSize;
        tcp.levels = m_process->m_NumInputMipLevels;

        if ( !texDest->create( tcp ) )
        {
            KHAOS_DELETE texDest;
            return;
        }

        texDest->fetchSurface();

        vector<uint8>::type buffers;
        buffers.resize( m_process->m_InputSize * m_process->m_InputSize * sizeof(float) * 4 );

        for ( int level = 0; level < texDest->getLevels(); ++ level )
        {
            int dataPitch = m_process->m_InputSurfaceMips[level][0].m_Width * sizeof(float) * 4;

            for ( int face = 0; face < 6; ++face )
            {
                CImageSurface& surSrc = m_process->m_InputSurfaceMips[level][face];
                SurfaceObj* surDest = texDest->getSurface( (CubeMapFace)face, level );

                surSrc.GetImageDataScaleGamma( CP_VAL_FLOAT32, 4, dataPitch, &buffers[0], 1.0f, outGamma );
                surDest->fillData( 0, &buffers[0], dataPitch );
            }
        }

        String fullFile = g_fileSystem->getFullFileName( outFile );
        texDest->save( fullFile.c_str() );
        KHAOS_DELETE texDest;

        TexCfgSaver::saveSimple( outFile, TEXTYPE_CUBE, tcp.format, TEXADDR_CLAMP, TEXF_LINEAR, 
            TEXF_LINEAR, false, TexObjLoadParas::MIP_FROMFILE ); // 保存纹理描述文件
    }

    void EnvMapBuilder::_initInputData()
    {
        PixelFormat pfmt = m_obj->getFormat();
        uint32 ofmt = _convertPixFmt( pfmt );

        const float maxClampVal = 1e+22f;
        const float igamma = m_obj->isSRGB() ? 2.2f : 1.0f;

        int width = m_obj->getWidth();
        int height = m_obj->getHeight();

        m_obj->getTextureObj()->fetchSurface();
        
        // read in top cube level into cube map processor
        for ( int i=0; i<6; ++i )
        {
            // now copy the surface depending on the type
            switch ( pfmt )
            {
            case PIXFMT_A8R8G8B8:
            case PIXFMT_X8R8G8B8:
            case PIXFMT_A16B16G16R16:
            case PIXFMT_A16B16G16R16F:
            case PIXFMT_A32B32G32R32F:
                {
                    // lock rect, copy data then unlock the rect
                    LockedRect lockRect;
                    m_obj->lockCube( (CubeMapFace)i, 0, TEXACC_READ, &lockRect, 0 );

                    m_process->SetInputFaceData( i, ofmt, 4, lockRect.pitch, lockRect.bits, 
                        maxClampVal, igamma, 1.0f );

                    m_obj->unlockCube( (CubeMapFace)i, 0 );
                }
                break;

            default:    
                { 
                    // 压缩格式，我们先转换为可直接读取的格式
                    SurfaceObj* surSrc = m_obj->getSurface( (CubeMapFace)i, 0 );
                    SurfaceObj* surTemp = g_renderDevice->createSurfaceObj();
                    surTemp->createOffscreenPlain( width, height, PIXFMT_A32B32G32R32F, 0 );
                    g_renderDevice->readSurfaceToCPU( surSrc, surTemp );

                    // lock rect, copy data then unlock the rect
                    LockedRect lockRect;
                    surTemp->lock( TEXACC_READ, &lockRect, 0 );

                    ofmt = _convertPixFmt( PIXFMT_A32B32G32R32F );
                    m_process->SetInputFaceData( i, ofmt, 4, lockRect.pitch, lockRect.bits, 
                        maxClampVal, igamma, 1.0f );

                    surTemp->unlock();

                    KHAOS_DELETE surTemp;
                }
                break;
            } // end switch
        } // end for
    }

    void EnvMapBuilder::_fixupEdge()
    {
        FixupCubeEdges( m_process->m_OutputSurface[m_currLevel], DefaultFixupType, DefaultFixupWidth );
    }

    void EnvMapBuilder::buildEnvSpecularIBL()
    {
        // 初始化样本
        if ( m_processMethod == FAST_METHOD ) // 重要性采样
        {
            // 需要mipmap数据，采用gpu gems3 ch.20方法
            m_process->GeneralInputMips();

            // hammersley分布，?k个样本,ggx映射
            m_rands.general( 8192, RandSamples::RT_Hammersley );
            m_mapper.setRandsDistribution( &m_rands, SphereSampleMapper::HemiSphereGGX );
            m_theSS.setSamples( &m_mapper );
        }
        else // 完全积分
        {
            m_process->BeginIntegrate();
        }

        const double roughness_min = 0.0;
        const double roughness_max = 1.0;
        const double roughness_len = roughness_max - roughness_min;

        int lastLevel = m_process->m_NumMipLevels - 1;

        // each level
        for ( m_currLevel = 0; m_currLevel < m_process->m_NumMipLevels; ++m_currLevel )
        {
            m_currMapSize   = m_process->m_OutputSurface[m_currLevel][0].m_Width;
            m_currRoughness = float(roughness_min + roughness_len * m_currLevel / lastLevel);

            Khaos::_outputDebugStr( "buildEnvSpecularIBL: lev=%d, mapsize=%d, rough=%f\n", 
                m_currLevel, m_currMapSize, m_currRoughness );

            // each face
            for ( m_currFace = 0; m_currFace < 6; ++m_currFace )
            {
                m_currSurfaceDest = &m_process->m_OutputSurface[m_currLevel][m_currFace];

                // each texel
                for ( int v = 0; v < m_currMapSize; ++v )
                {
                    for ( int u = 0; u < m_currMapSize; ++u )
                    {
                        _buildSpecIBL_EachTexel( u, v );
                    }
                }
            }

            // fix up edge
            _fixupEdge();
        }
    }

    void EnvMapBuilder::_buildSpecIBL_EachTexel( int x, int y )
    {
        // 得到uv
        float u, v;
        RowColToTexelCoord( x, y, &u, &v );

        // 得到反射向量
        Vector3 vR;
        TexelCoordToVector( m_currFace, u, v, m_currMapSize, vR.ptr(), DefaultFixupType );

        float totalWeight = 0.0f;
        Vector3 totalClr(Vector3::ZERO);

        if ( m_processMethod == FAST_METHOD ) // 重要性采样
        {
            _calcSpecularGetImportant( vR, totalClr, totalWeight );
        }
        else // 完全积分
        {
            EnvBakeContext context;
            context.pThis = this;
            context.vN = vR;
            context.weightTotal = 0;

            m_process->Integrate( _perSpecularGet, &context, context.vN.ptr(), totalClr.ptr(), 3 );

            totalWeight = (float)context.weightTotal;
        }

        // 输出
        Vector4& dest = *(Vector4*)(m_currSurfaceDest->GetSurfaceTexelPtr(x, y));

        if ( totalWeight > 0.001f )
        {
            dest = totalClr / totalWeight;
        }
        else
        {
            dest = *(Vector3*) GetCubeMapTexelPtr( m_process->m_InputSurface, vR.ptr() );
        }
    }

    void EnvMapBuilder::_calcSpecularGetImportant( const Vector3& vR,
        Vector3& totalClr, float& totalWeight )
    {
        if ( m_currRoughness < 1e-5f ) // 最光滑的时候做优化，一般是第0层
        {
            const float* src = GetCubeMapTexelPtr( m_process->m_InputSurface, vR.ptr() );
            totalClr.setValue( src );
            totalWeight = 1;
            return;
        }

        // 这里有个假定 N = R, V = R，见ue4论文pbr
        const Vector3& vN = vR;
        const Vector3& vV = vR;

        // 随机种子
        //static const uint VIEW_RAND = Math::randInt();
        //UIntVector2 seed = Math::scrambleTEA(UIntVector2(x, y));

        //seed.x ^= VIEW_RAND;
        //seed.y ^= VIEW_RAND;

        // hammersley随机分布，8k个样本,ggx映射
        //m_rands.setSeed( seed );
        //m_rands.general( 1, 8192, RandSamples::RT_Hammersley );

        // 生成符合当前参数的映射集
        m_mapper.setSpecularBRDFParas( vN, vV, m_currRoughness );
        m_mapper.general();

        const int cubeWidth = m_process->m_InputSize;
        const double solidAngleTexel = (4.0 * Math::PI) / (6 * cubeWidth * cubeWidth);

        // 卷积
        int numSamples = m_rands.getNumSamples();

        for ( int i = 0; i < numSamples; ++i )
        {
            const SphereSamples::Sample& sample = m_mapper.getSample( i );
            const Vector3& vH = sample.dir;
            Vector3 vL = Math::reflectDir( vV, vH, true );

            float NdotL = vN.dotProduct( vL );

            if ( NdotL > 0 )
            {
                NdotL = Math::minVal( NdotL, 1.0f );

                float lodLevel = 0;

                if ( m_currRoughness > 0 )
                {
                    float NdotH = Math::saturate( vN.dotProduct(vH) );
                    float VdotH = Math::saturate( vV.dotProduct(vH) );

                    double pdf_i = SphereSampleMapper::getGGXProbability( m_currRoughness, NdotH, VdotH );
                    double solidAngleSample = 1.0 / (numSamples * pdf_i);

                    lodLevel = 0.5f * Math::log2( float(solidAngleSample / solidAngleTexel) );
                }

                float src[4];
                m_process->GetMipmapTexelVal( vL.ptr(), lodLevel, src );

                totalClr += (*(Vector3*)(src)) * NdotL;
                totalWeight += NdotL;
            }
        }
    }

    bool EnvMapBuilder::_perSpecularGet( CCubeMapProcessor* proc, void* context, int face, int u, int v, 
        const float* vDir1, float* vals, double& deltaArea )
    {
        // 完全积分
        EnvBakeContext* envData = (EnvBakeContext*)(context);

        const Vector3& vN = envData->vN;
        const Vector3& vR = envData->vN;
        const Vector3& vV = envData->vN;

        const Vector3& vDir = *(Vector3*)vDir1;

        float NdotL = vDir.dotProduct( vN );
        if ( NdotL <= 0 )
            return false;

        NdotL = Math::minVal( NdotL, 1.0f );

        Vector3 vH = (vV + vDir).normalisedCopy();

        // 求pdf
        float NdotH = Math::saturate( vN.dotProduct(vH) );
        float VdotH = Math::saturate( vV.dotProduct(vH) );
        float pdf_i = SphereSampleMapper::getGGXProbability( envData->pThis->m_currRoughness, NdotH, VdotH );

        const float max_pdf = 1e+10f;
        if ( pdf_i > max_pdf )
            pdf_i = max_pdf;

        // 计算权重
        deltaArea = deltaArea * pdf_i * NdotL;
        envData->weightTotal += deltaArea;

        // 计算值
        const Vector3& lightSrc = *(Vector3*)proc->m_InputSurface[face].GetSurfaceTexelPtr( u, v );
        *(Vector3*)vals = lightSrc;

        return true;
    }

    void EnvMapBuilder::buildEnvDiffuseIBL()
    {
        if ( m_processMethod == FAST_METHOD ) // 重要性采样
        {
            // 假如cube size是256，一般样本也要256^2*6 = 64k * 6个样本
            // 为了保证一定质量，我们依然采用64k个样本，hammersley随机分布，全球均匀映射（为了保证平滑）
            m_rands.general( 65536, RandSamples::RT_Hammersley );
            m_mapper.setRandsDistribution( &m_rands, SphereSampleMapper::EntireSphereUniform );
            m_mapper.setCommConfig( true, false );
            m_mapper.general();
            m_theSS.setSamples( &m_mapper );
        }
        else // 完全积分
        {
            m_process->BeginIntegrate();
        }

        // diffuse应该只处理一层
        khaosAssert( m_process->m_NumMipLevels == 1 );
        m_currLevel = 0;

        m_currMapSize = m_process->m_OutputSurface[m_currLevel][0].m_Width;

        Khaos::_outputDebugStr( "buildEnvDiffuseIBL: mapsize=%d\n", m_currMapSize );

        // each face
        for ( m_currFace = 0; m_currFace < 6; ++m_currFace )
        {
            Khaos::_outputDebugStr( "buildEnvDiffuseIBL: face=%d\n", m_currFace );

            m_currSurfaceDest = &m_process->m_OutputSurface[m_currLevel][m_currFace];

            // each texel
            for ( int v = 0; v < m_currMapSize; ++v )
            {
                for ( int u = 0; u < m_currMapSize; ++u )
                {
                    _buildDiffuseIBL_EachTexel( u, v );
                }
            }
        }

        // fix up edge
        _fixupEdge();
    }

    void EnvMapBuilder::_buildDiffuseIBL_EachTexel( int x, int y )
    {
        EnvBakeContext context;

        // 得到uv
        float u, v;
        RowColToTexelCoord( x, y, &u, &v );

        // 得到法线向量
        TexelCoordToVector( m_currFace, u, v, m_currMapSize, context.vN.ptr(), DefaultFixupType );

        if ( m_processMethod == FAST_METHOD ) // 重要性采样
            m_mapper.setDiffuseBRDFParas( context.vN );

        // 计算此法线得到diffuse光照总和
        Vector3 diffTotal(Vector3::ZERO);

        if ( m_processMethod == FAST_METHOD ) // 重要性采样
            m_theSS.integrate( _perDiffuseGetImportant, this, diffTotal.ptr(), 3 );
        else // 完全积分
            m_process->Integrate( _perDiffuseGet, &context, context.vN.ptr(), diffTotal.ptr(), 3 );

        diffTotal /= Math::PI;

        // 输出
        Vector4& dest = *(Vector4*)(m_currSurfaceDest->GetSurfaceTexelPtr(x, y));
        dest = diffTotal;
    }

    bool EnvMapBuilder:: _perDiffuseGet( CCubeMapProcessor* proc, void* context, int face, int u, int v, 
        const float* vDir1, float* vals, double& deltaArea )
    {
        // 完全积分
        EnvBakeContext* diffData = (EnvBakeContext*)(context);

        const Vector3& vN = diffData->vN;
        const Vector3& vDir = *(Vector3*)vDir1;

        float ndotl = vDir.dotProduct( vN );

        if ( ndotl <= 0 )
        {
            return false;
        }

        const Vector3& lightSrc = *(Vector3*) proc->m_InputSurface[face].GetSurfaceTexelPtr( u, v );
        *(Vector3*)vals = lightSrc * Math::minVal( ndotl, 1.0f );
        return true;
    }

    void EnvMapBuilder::_perDiffuseGetImportant( SphereSamples* theSS, const SphereSamples::Sample& smpl,
        void* context, float* vals, int groupCnt )
    {
        // 重要性采样积分
        EnvMapBuilder* pThis = (EnvMapBuilder*)(context);

        const Vector3& vN = theSS->getMapper()->getNormal();
        float ndotl = smpl.dir.dotProduct( vN );

        if ( ndotl <= 0 )
        {
            vals[0] = vals[1] = vals[2] = 0;
            return;
        }

        const Vector3& lightSrc = *(Vector3*) GetCubeMapTexelPtr( pThis->m_process->m_InputSurface, smpl.dir.ptr() );
        *(Vector3*)vals = lightSrc * Math::minVal( ndotl, 1.0f );
    }

    //////////////////////////////////////////////////////////////////////////
    void makeMSRMap( const String& outputFile,
        const String& metallicFile, 
        const String& specularFile,
        const String& roughnessFile );
}


//////////////////////////////////////////////////////////////////////////
void _buildWork()
{
    //return;

    using namespace Khaos;

    const char* srcfile = "/Texture/stpeters_cross.dds";
    // "/Texture/stpeters_cross.dds" "/Texture/plaza.dds" "/Texture/church.dds"

    if ( 1 )
    {
        EnvMapBuilder emb;
        emb.init( EnvMapBuilder::FAST_METHOD, srcfile, SPEC_IBL_SIZE, 0 );
        emb.buildEnvSpecularIBL();
        emb.save( "/Texture/test_spec_ibl.dds" );
    }
    
    if ( 1 )
    {
        EnvMapBuilder emb;
        emb.init( EnvMapBuilder::FAST_METHOD, srcfile, DIFF_IBL_SIZE, 1 );
        emb.buildEnvDiffuseIBL();
        emb.save( "/Texture/test_diff_ibl.dds" );
    }
}

void _makeMSRTest()
{
    return;

    Khaos::makeMSRMap( "/Pistol/cerberus_mxr.dds",
        "/Pistol/Cerberus_M.tga", "", "/Pistol/Cerberus_R.tga" );
}

//////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK wndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

HWND createWindow(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= wndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= hInstance;
    wcex.hIcon			= 0;
    wcex.hCursor		= 0;
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName	= 0;
    wcex.lpszClassName	= _T("wndEnvMapBuilder");
    wcex.hIconSm		= 0;

    RegisterClassEx(&wcex);

    HWND hWnd = CreateWindow( _T("wndEnvMapBuilder"), _T(""), WS_OVERLAPPEDWINDOW,
        0, 0, 512, 512, NULL, NULL, hInstance, NULL );

    return hWnd;
}

void initSampleCreateContext( HINSTANCE hInst, HWND hWnd, Khaos::RootConfig& cfg )
{
    char exeName[MAX_PATH];
    GetModuleFileNameA( hInst, exeName, sizeof(exeName) );

    static char exePath[MAX_PATH];
    char* filePart;
    GetFullPathNameA( exeName, MAX_PATH, exePath, &filePart );
    *filePart = 0;

    RECT rcClient;
    GetClientRect( hWnd, &rcClient );

    cfg.rdcContext.handleWindow = hWnd;
    cfg.rdcContext.windowWidth  = rcClient.right - rcClient.left;
    cfg.rdcContext.windowHeight = rcClient.bottom - rcClient.top;
    cfg.fileSystemPath = Khaos::String(exePath) + "../Media/";
}

LRESULT CALLBACK wndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    using namespace Khaos;

    HWND hWnd = createWindow( hInstance );

    RootConfig rootConfig;
    initSampleCreateContext( hInstance, hWnd, rootConfig );

    KHAOS_NEW Root;
    g_root->init( rootConfig );

    _buildWork();
    _makeMSRTest();

    g_root->shutdown();
    KHAOS_DELETE g_root;

    return 0;
}

