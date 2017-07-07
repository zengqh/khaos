#include "KhaosPreHeaders.h"
#include "KhaosDist2Alpha.h"
#include "KhaosMath.h"
#include "KhaosTextureObj.h"
#include "KhaosRenderDevice.h"
#include "KhaosFileSystem.h"
#include "KhaosTexCfgParser.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void dist2Alpha( ISimpleTexData* srcData, ISimpleTexData* destData, const Dist2AlphaConfig& cfg )
    {
        const float alphaInSideRef = cfg.alphaInSideRef;

        int in_w  = srcData->getWidth();
        int in_h  = srcData->getHeight();
        int out_w = destData->getWidth();
        int out_h = destData->getHeight();

        float in_w_max  = float(in_w - 1);
        float in_h_max  = float(in_h - 1);
        float out_w_max = float(out_w - 1);
        float out_h_max = float(out_h - 1);

        float max_rad = 2.0f * cfg.spread * Math::maxVal( in_w/(float)out_w, in_h/(float)out_h );

        float in_max = Math::maxVal(in_w_max, in_h_max);
        if ( max_rad > in_max )
            max_rad = in_max;

        int irad = (int) Math::ceil( max_rad );

        for ( int y = 0; y < out_h; ++y )
        {
            for ( int x = 0; x < out_w; ++x )
            {
                // 低分辨率上的位置对应到高分辨率的原始位置
                int orig_x = (int) Math::lerp( 0, in_w_max, 0, out_w_max, (float)x );
                int orig_y = (int) Math::lerp( 0, in_h_max, 0, out_h_max, (float)y );

                float closest_dist = Math::POS_INFINITY;

                // 原始位置是否是“在内”
                bool inside_test = srcData->getAlphaVal( orig_x, orig_y ) > alphaInSideRef;

                // 在原始位置附近领域搜索最近距离
                for ( int oy = -irad; oy <= irad; ++oy )
                {
                    for ( int ox = -irad; ox <= irad; ++ox )
                    {
                        int x1 = orig_x + ox;
                        int y1 = orig_y + oy;

                        if ( (x1 >= 0) && (x1 < in_w) && // 点在范围内
                             (y1 >= 0) && (y1 < in_h) &&
                             (inside_test != (srcData->getAlphaVal(x1, y1) > alphaInSideRef)) // 且是相反的域
                           )
                        {
                            float d2 = Math::sqrt( (float)(ox*ox + oy*oy) );
                            if ( d2 < closest_dist )
                                closest_dist = d2;
                        }
                    }
                }

                // 写入结果
                float alpha = Math::lerp( 0.0f, 0.5f, 0.0f, max_rad, closest_dist ); // 距离映射为[0, 0.5]
                alpha = Math::minVal(0.5f, alpha);

                if ( !inside_test ) // 外部为负数距离
                    alpha = -alpha;

                alpha = 0.5f + alpha; // to [0, 1]

                destData->setAlphaVal( x, y, alpha ); // set alpha value
            }
        }
    }


    //////////////////////////////////////////////////////////////////////////
    class SimpleTexData_TexObj : public ISimpleTexData
    {
    public:
        SimpleTexData_TexObj( TextureObjUnit* tex, int channel ) : m_srcTex(tex), m_channel(channel)
        {
            m_srcTex->_fetchReadData();
        }

        virtual int getWidth() const  { return m_srcTex->getWidth(); }
        virtual int getHeight() const { return m_srcTex->getHeight(); }

        virtual float getAlphaVal( int x, int y ) const
        {
            return m_srcTex->_readTex2DPix( x, y )[m_channel];
        }

        virtual void  setAlphaVal( int x, int y, float a ) {}

    public:
        TextureObjUnit* m_srcTex;
        int m_channel;
    };
    
    class SimpleTexData_Mem : public AllocatedObject, public ISimpleTexData
    {
    public:
        SimpleTexData_Mem( TextureObjUnit* tex, int destWidth, int destHeight, int channel )
        {
            m_width = destWidth;
            m_height = destHeight;
            m_channel = channel;

            m_clrs = KHAOS_NEW_ARRAY_T(Color, m_width*m_height);

            // 用源图来初始化数据，这里的数据处于线性空间
            tex->_fetchReadData();

            float in_w_max = tex->getWidth() - 1.0f;
            float in_h_max = tex->getHeight() - 1.0f;
            float out_w_max = destWidth - 1.0f;
            float out_h_max = destHeight - 1.0f;

            for ( int y = 0; y < m_height; ++y )
            {
                for ( int x = 0; x < m_width; ++x )
                {
                    int orig_x = (int) Math::lerp( 0, in_w_max, 0, out_w_max, (float)x );
                    int orig_y = (int) Math::lerp( 0, in_h_max, 0, out_h_max, (float)y );

                    m_clrs[y * m_width + x] = tex->_readTex2DPix( orig_x, orig_y );
                }
            }
        }

        ~SimpleTexData_Mem()
        {
            KHAOS_DELETE_ARRAY_T(m_clrs, m_width*m_height);
        }

        virtual int getWidth() const  { return m_width; }
        virtual int getHeight() const { return m_height; }

        virtual float getAlphaVal( int x, int y ) const { return 0; }

        virtual void  setAlphaVal( int x, int y, float a ) 
        {
            khaosAssert( 0 <= x && x < m_width );
            khaosAssert( 0 <= y && y < m_height );
            m_clrs[y * m_width + x][m_channel] = a;
        }

        int m_width;
        int m_height;
        int m_channel;
        Color* m_clrs;
    };

    void dist2Alpha( TextureObjUnit* srcTex, const String& destTexFile, const Dist2AlphaConfig& cfg )
    {
        int destWidth  = cfg.destWidth == 0  ? srcTex->getWidth()  : cfg.destWidth;
        int destHeight = cfg.destHeight == 0 ? srcTex->getHeight() : cfg.destHeight;

        SimpleTexData_TexObj simSrc( srcTex, 0 );
        SimpleTexData_Mem*   simDests[64] = {0};

        int genLevels = 0;
        int currWidth  = destWidth;
        int currHeight = destHeight;

        while ( true )
        {
            SimpleTexData_Mem* simOut = KHAOS_NEW SimpleTexData_Mem( srcTex, currWidth, currHeight, 0 );
            simDests[genLevels] = simOut;

            // 转换
            if ( cfg.flag & Dist2AlphaConfig::R_ENABLED )
            {
                simSrc.m_channel = simOut->m_channel = 0;
                dist2Alpha( &simSrc, simOut, cfg );
            }

            if ( cfg.flag & Dist2AlphaConfig::G_ENABLED )
            {
                simSrc.m_channel = simOut->m_channel = 1;
                dist2Alpha( &simSrc, simOut, cfg );
            }

            if ( cfg.flag & Dist2AlphaConfig::B_ENABLED )
            {
                simSrc.m_channel = simOut->m_channel = 2;
                dist2Alpha( &simSrc, simOut, cfg );
            }

            if ( cfg.flag & Dist2AlphaConfig::A_ENABLED )
            {
                simSrc.m_channel = simOut->m_channel = 3;
                dist2Alpha( &simSrc, simOut, cfg );
            }

            ++genLevels;
           
            if ( cfg.mipmapGenMinSize <= 0 ) // 不需要mipmap
                break;

            if ( currWidth > cfg.mipmapGenMinSize &&
                 currHeight > cfg.mipmapGenMinSize ) // 处理mipmap到达最小要求尺寸
            {
                currWidth /= 2;
                currHeight /= 2;
            }
            else
                break;
        }
        
        // 写入文件
        TextureObj* destTex = g_renderDevice->createTextureObj();

        TexObjCreateParas paras;
        paras.type = TEXTYPE_2D;
        paras.usage  = TEXUSA_STATIC;
        paras.format = cfg.destTexFmt == PIXFMT_INVALID ? srcTex->getFormat() : cfg.destTexFmt;
        paras.levels = genLevels;
        paras.width  = destWidth;
        paras.height = destHeight;

        bool sRGBWrite = srcTex->isSRGB();
        if ( cfg.sRGBWrite == 1 )
            sRGBWrite = true;
        else if ( cfg.sRGBWrite == 0 )
            sRGBWrite = false;

        destTex->create( paras );

        for ( int curLev = 0; curLev < genLevels; ++curLev )
        {
            destTex->fillConvertData( curLev, 0, simDests[curLev]->m_clrs->ptr(), 0, sRGBWrite );
            KHAOS_DELETE simDests[curLev];
        }

        String fullFile = g_fileSystem->getFullFileName( destTexFile );
        destTex->save( fullFile.c_str() );

        // 保存描述文件
        int mipmapReadSize = cfg.mipmapReadSize; // 默认为TexObjLoadParas::MIP_AUTO
        if ( cfg.mipmapGenMinSize > 0 )
             mipmapReadSize = TexObjLoadParas::MIP_FROMFILE;

        TexCfgSaver::saveSimple( destTexFile, paras.type, paras.format, 
            (TextureAddress)srcTex->getAddress().addrU, 
            TEXF_LINEAR, (TextureFilter)cfg.mipmapFilter, 
            sRGBWrite, 
            mipmapReadSize );
    }
}

