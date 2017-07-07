
#include "KhaosRoot.h"
#include "KhaosTexCfgParser.h"
#include "CCubeMapProcessor.h"

namespace Khaos
{
    struct TextureItem_
    {
        TexturePtr tex;
        CImageSurface img;

        TextureItem_( const String& file )
        {
            load( file );
        }

        void load( const String& file )
        {
            if ( file.empty() )
                return;

            // gpu obj
            tex.attach( TextureManager::getOrCreateTexture( file ) );
            tex->load( false );

            const float maxClampVal = 1e+22f;
            const float igamma = tex->isSRGB() ? 2.2f : 1.0f;

            // cpu obj
            img.Init( tex->getWidth(), tex->getHeight(), 4 );

            // gpu -> cpu
            tex->getTextureObj()->fetchSurface();
            SurfaceObj* surSrc = tex->getSurface(0);

            SurfaceObj* surTemp = g_renderDevice->createSurfaceObj();
            surTemp->createOffscreenPlain( tex->getWidth(), tex->getHeight(), PIXFMT_A32B32G32R32F, 0 );
            g_renderDevice->readSurfaceToCPU( surSrc, surTemp );

            LockedRect lockRect;
            surTemp->lock( TEXACC_READ, &lockRect, 0 );

            img.SetImageDataClampDegammaScale( CP_VAL_FLOAT32, 4, lockRect.pitch, lockRect.bits, 
                maxClampVal, igamma, 1.0f );

            surTemp->unlock();

            KHAOS_DELETE surTemp;
        }
    };

    void _saveFile( CImageSurface& img, const String& file )
    {
        const float outGamma = 1.0f; // 目前输出为线性

        TextureObj* texDest = g_renderDevice->createTextureObj();

        TexObjCreateParas tcp;

        tcp.type   = TEXTYPE_2D;
        tcp.usage  = TEXUSA_STATIC;
        tcp.format = PIXFMT_A8R8G8B8;
        tcp.width  = img.m_Width;
        tcp.height = img.m_Height;

        if ( !texDest->create( tcp ) )
        {
            KHAOS_DELETE texDest;
            return;
        }

        vector<uint8>::type buffers;
        buffers.resize( img.m_Width * img.m_Height * sizeof(uint8) * 4 );

        int dataPitch = img.m_Width * sizeof(uint8) * 4;
        img.GetImageDataScaleGamma( CP_VAL_UNORM8, 4, dataPitch, &buffers[0], 1.0f, outGamma );

        texDest->fetchSurface();
        texDest->getSurface(0)->fillData( 0, &buffers[0], dataPitch );
          
        String fullFile = g_fileSystem->getFullFileName( file );
        texDest->save( fullFile.c_str() );
        KHAOS_DELETE texDest;

        TexCfgSaver::saveSimple( file, TEXTYPE_2D, tcp.format, TEXADDR_WRAP, TEXF_LINEAR, 
            TEXF_LINEAR, false, TexObjLoadParas::MIP_AUTO ); // 保存纹理描述文件
    }

    void makeMSRMap( const String& outputFile,
                     const String& metallicFile, 
                     const String& specularFile,
                     const String& roughnessFile )
    {
        // 读取原文件
        TextureItem_ texMetallic( metallicFile );
        TextureItem_ texSpecular( specularFile );
        TextureItem_ texRoughness( roughnessFile );

        // 合并
        TextureItem_* texOne = &texMetallic;
        if ( !texOne->tex.get() )
            texOne = &texSpecular;
        if ( !texOne->tex.get() )
            texOne = &texRoughness;

        int srcPitch = texOne->img.m_Width * texOne->img.m_NumChannels * sizeof(float);
        int srcChannels = texOne->img.m_NumChannels;

        CImageSurface imgMerge;
        imgMerge.Init( texOne->img.m_Width, texOne->img.m_Height, 4 );
        imgMerge.ClearChannelConst( 3, 1.0f ); // a-channel = 1

        if ( texMetallic.tex.get() )
            imgMerge.SetChannelData( 2, srcPitch, srcChannels, texMetallic.img.m_ImgData ); // r = m
        
        if ( texSpecular.tex.get() )
            imgMerge.SetChannelData( 1, srcPitch, srcChannels, texSpecular.img.m_ImgData ); // g = s

        if ( texRoughness.tex.get() )
            imgMerge.SetChannelData( 0, srcPitch, srcChannels, texRoughness.img.m_ImgData ); // b = r

        // 写目标文件
        _saveFile( imgMerge, outputFile );
    }
}

