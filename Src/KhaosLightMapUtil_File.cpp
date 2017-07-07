#include "KhaosPreHeaders.h"
#include "KhaosLightmapUtil.h"
#include "KhaosRenderDevice.h"
#include "KhaosTextureObj.h"
#include "KhaosBinStream.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    SimpleLightMap::SimpleLightMap() : m_width(0), m_height(0), m_channels(0)
    {
    }

    SimpleLightMap::~SimpleLightMap()
    {
    }

    void SimpleLightMap::setSize( int width, int height, int channels )
    {
        khaosAssert( 1 == channels || 3 == channels || 4 == channels );

        m_width = width;
        m_height = height;
        m_channels = channels;

        m_data.resize( width * height * channels, 0 );
        m_used.resize( width * height, false );
    }

    void SimpleLightMap::clearChannel( int channel, float val )
    {
        for ( int y = 0; y < m_height; ++y )
        {
            for ( int x = 0; x < m_width; ++x )
            {
                setVal( channel, x, y, val );
            }
        }
    }
    void  SimpleLightMap::clearAll( const Color& clr )
    {
        for ( int y = 0; y < m_height; ++y )
        {
            for ( int x = 0; x < m_width; ++x )
            {
                setColor( x, y, clr );
            }
        }
    }

    void SimpleLightMap::clearUsed()
    {
        m_used.clear();
        m_used.resize( m_width * m_height, false );
    }

    void SimpleLightMap::setVal( int channel, int x, int y, float v )
    {
        if ( 0 <= channel && channel < m_channels )
        {
            khaosAssert( 0 <= x && x < m_width );
            khaosAssert( 0 <= y && y < m_height );

            int offset = (y * m_width + x) * m_channels + channel;
            m_data[offset] = v;
        }
    }

    float SimpleLightMap::getVal( int channel, int x, int y ) const
    {
        if ( 0 <= channel && channel < m_channels )
        {
            khaosAssert( 0 <= x && x < m_width );
            khaosAssert( 0 <= y && y < m_height );

            int offset = (y * m_width + x) * m_channels + channel;
            return m_data[offset];
        }
        
        return 1.0f;
    }

    void SimpleLightMap::setUsed( int x, int y, bool b )
    {
        khaosAssert( 0 <= x && x < m_width );
        khaosAssert( 0 <= y && y < m_height );

        m_used[y * m_width + x] = b;
    }

    bool SimpleLightMap::isUsed( int x, int y ) const
    {
        khaosAssert( 0 <= x && x < m_width );
        khaosAssert( 0 <= y && y < m_height );

        return m_used[y * m_width + x];
    }

    void SimpleLightMap::setRed( int x, int y, float r )
    {
        setVal( CHANNEL_R, x, y, r );
    }

    void SimpleLightMap::setGreen( int x, int y, float g )
    {
        setVal( CHANNEL_G, x, y, g );
    }

    void SimpleLightMap::setBlue( int x, int y, float b )
    {
        setVal( CHANNEL_B, x, y, b );
    }

    void SimpleLightMap::setAlpha( int x, int y, float a )
    {
        setVal( CHANNEL_A, x, y, a );
    }
    
    float SimpleLightMap::getRed( int x, int y ) const
    {
        return getVal(CHANNEL_R, x, y);
    }

    float SimpleLightMap::getGreen( int x, int y ) const
    {
        return getVal(CHANNEL_G, x, y);
    }

    float SimpleLightMap::getBlue( int x, int y ) const
    {
        return getVal(CHANNEL_B, x, y);
    }

    float SimpleLightMap::getAlpha( int x, int y ) const
    {
        return getVal(CHANNEL_A, x, y);
    }

    void SimpleLightMap::setColor( int x, int y, const Color& clr )
    {
        setAlpha( x, y, clr.a );
        setRed( x, y, clr.r );
        setGreen( x, y, clr.g );
        setBlue( x, y, clr.b );
    }

    Color SimpleLightMap::getColor( int x, int y ) const
    {
        return Color(
            getRed(x, y),
            getGreen(x, y),
            getBlue(x, y),
            getAlpha(x, y)
            );
    }

    void _uvLinearPt( float uv, int size, int& low_uv, int& high_uv, float& line_val )
    {
        const float e = 1e-3f;

        float puv = uv * size;

        if ( puv < 0.5f + e ) // 小于第一个半像素
        {
            low_uv = high_uv = 0; // 就取第一个
            line_val = 0;
            return;
        }

        float max_uv = size - (0.5f + e); // 大于最后个半像素

        if ( puv > max_uv )
        {
            low_uv = high_uv = size - 1; // 就取最后个
            line_val = 0;
            return;
        }

        float puv_dec = puv - 0.5f;

        low_uv   = int(puv_dec);
        high_uv  = low_uv + 1;
        line_val = puv_dec - low_uv;
    }

    Color SimpleLightMap::getColorByUV( const Vector2& uv ) const
    {
        int   low_px, high_px;
        int   low_py, high_py;
        float line_x, line_y;

        _uvLinearPt( uv.x, m_width, low_px, high_px, line_x );
        _uvLinearPt( uv.y, m_height, low_py, high_py, line_y );

        Color val_lt = getColor( low_px, low_py );
        Color val_rt = getColor( high_px, low_py );
        Color val_up = val_lt + (val_rt - val_lt) * line_x;

        Color val_lb = getColor( low_px, high_py );
        Color val_rb = getColor( high_px, high_py );
        Color val_down = val_lb + (val_rb - val_lb) * line_x;

        return val_up + (val_down - val_up) * line_y;
    }

    void SimpleLightMap::addLightMap( SimpleLightMap* litMap, const Color& scale, bool hasAlpha )
    {
        for ( int y = 0; y < m_height; ++y )
        {
            for ( int x = 0; x < m_width; ++x )
            {
                Color clr = this->getColor( x, y );
                Color add = litMap->getColor( x, y ) * scale;;

                if ( hasAlpha )
                    clr += add;
                else
                    clr += add.getRGB();

                this->setColor( x, y, clr );
            }
        }
    }

    void SimpleLightMap::scaleColor( const Color& scale, bool hasAlpha )
    {
        int w = this->getWidth();
        int h = this->getHeight();

        for ( int y = 0; y < h; ++y )
        {
            for ( int x = 0; x < w; ++x )
            {
                Color clr = this->getColor( x, y ) * scale;

                if ( !hasAlpha )
                    clr = clr.getRGB();

                this->setColor( x, y, clr );
            }
        }
    }

    bool _buildLayerByPixFmt( vector<int>::type& defLayers, PixelFormat format )
    {
        khaosAssert( format == PIXFMT_A8R8G8B8 || format == PIXFMT_16F || format == PIXFMT_32F || format == PIXFMT_A16B16G16R16F );
        bool isFlt = (format == PIXFMT_16F || format == PIXFMT_32F || format == PIXFMT_A16B16G16R16F);

        if ( format == PIXFMT_16F || format == PIXFMT_32F )
        {
            defLayers.push_back( SimpleLightMap::CHANNEL_G ); // r
        }
        else if ( format == PIXFMT_A8R8G8B8 )
        {
            defLayers.push_back( SimpleLightMap::CHANNEL_B ); // b
            defLayers.push_back( SimpleLightMap::CHANNEL_G ); // g
            defLayers.push_back( SimpleLightMap::CHANNEL_R ); // r
            defLayers.push_back( SimpleLightMap::CHANNEL_A ); // a
        }
        else if ( format == PIXFMT_A16B16G16R16F )
        {
            defLayers.push_back( SimpleLightMap::CHANNEL_R ); // r
            defLayers.push_back( SimpleLightMap::CHANNEL_G ); // g
            defLayers.push_back( SimpleLightMap::CHANNEL_B ); // b
            defLayers.push_back( SimpleLightMap::CHANNEL_A ); // a
        }

        return isFlt;
    }

    void SimpleLightMap::saveFile( PixelFormat format, pcstr file )
    {
        vector<int>::type defLayers;
        bool isFlt = _buildLayerByPixFmt( defLayers, format );

        vector<uint8>::type datas_byte;
        vector<float>::type datas_flt;

        for ( int y = 0; y < m_height; ++y )
        {
            for ( int x = 0; x < m_width; ++x )
            {
                for ( size_t j = 0; j < defLayers.size(); ++j )
                {
                    float v = getVal( defLayers[j], x, y );

                    if ( isFlt )
                        datas_flt.push_back( v );
                    else
                        datas_byte.push_back( Math::clamp( Math::roundI(v * 255), 0, 255) );
                }
            }
        }

        TextureObj* tempTex = g_renderDevice->createTextureObj();

        TexObjCreateParas paras;
        paras.type   = TEXTYPE_2D;
        paras.usage  = TEXUSA_STATIC;
        paras.format = format;
        paras.levels = 1;
        paras.width  = m_width;
        paras.height = m_height;

        IntRect rect(0, 0, m_width, m_height);
        tempTex->create( paras );
        tempTex->fillData( 0, &rect, (isFlt ? (void*)(&datas_flt[0]) : (void*)(&datas_byte[0])), 0 );
        tempTex->fetchSurface();
        tempTex->getSurface(0)->save( file );

        KHAOS_DELETE tempTex;
    }

    //////////////////////////////////////////////////////////////////////////
    void SimpleVolumeMap::init( int w, int h, int d, const Color& clr )
    {
        m_width  = w;
        m_height = h;
        m_depth  = d;

        m_data.resize( w*h*d, clr );
    }

    void SimpleVolumeMap::setColor( int x, int y, int z, const Color& clr )
    {
        getColor(x, y, z) = clr;
    }

    const Color& SimpleVolumeMap::getColor( int x, int y, int z ) const
    {
        return const_cast<SimpleVolumeMap*>(this)->getColor( x, y, z );
    }

    Color& SimpleVolumeMap::getColor( int x, int y, int z )
    {
        khaosAssert( 0 <= x && x < m_width );
        khaosAssert( 0 <= y && y < m_height );
        khaosAssert( 0 <= z && z < m_depth );

        int idx = m_width * m_height * z + m_width * y + x;
        return m_data[idx];
    }

    const Color* SimpleVolumeMap::getData() const
    {
        return const_cast<SimpleVolumeMap*>(this)->getData();
    }

    Color* SimpleVolumeMap::getData()
    {
        return &m_data[0];
    }

    void SimpleVolumeMap::saveFile( PixelFormat format, pcstr file )
    {
        khaosAssert( format == PIXFMT_A16B16G16R16F || format == PIXFMT_A32B32G32R32F ); // 暂且只支持这个格式

        TextureObj* tempTex = g_renderDevice->createTextureObj();

        TexObjCreateParas paras;
        paras.type   = TEXTYPE_VOLUME;
        paras.usage  = TEXUSA_STATIC;
        paras.format = format;
        paras.levels = 1;
        paras.width  = m_width;
        paras.height = m_height;
        paras.depth  = m_depth;

        tempTex->create( paras );

        const int piece_pitch = m_width * sizeof(float) * 4;

        for ( int d = 0; d < m_depth; ++d )
        {
            const Color& clr = this->getColor( 0, 0, d );
            tempTex->fillVolumeData( 0, d, 0, &clr, piece_pitch );
        }

        tempTex->save( file );
        KHAOS_DELETE tempTex;
    }


    //////////////////////////////////////////////////////////////////////////
    LightMapPreBuildHeadFile::LightMapPreBuildHeadFile() : 
        m_width(0), m_height(0), m_depth(0), m_maxRayCount(0)
    {
    }

    LightMapPreBuildHeadFile::~LightMapPreBuildHeadFile()
    {
        saveFile(); // auto save
    }

    void LightMapPreBuildHeadFile::setSize( int w, int h )
    {
        setSize( w, h, 0 );
    }

    void LightMapPreBuildHeadFile::setSize( int w, int h, int d )
    {
        m_width  = w;
        m_height = h;
        m_depth  = d;

        int real_d = d ? d : 1;
        int cnt = w * h * real_d;

        m_infos.resize( cnt );
    }

    void LightMapPreBuildHeadFile::setMaxRayCount( int cnt )
    {
        m_maxRayCount = cnt;
    }

    int LightMapPreBuildHeadFile::toTexelID( int xp, int yp ) const
    {
        return m_width * yp + xp;
    }

    int LightMapPreBuildHeadFile::toTexelID( int xv, int yv, int zv ) const
    {
        return m_width * m_height * zv + m_width * yv + xv;
    }

    const LightMapPreBuildHeadFile::ItemHeadInfo* LightMapPreBuildHeadFile::getItemInfo( int id ) const
    {
        return const_cast<LightMapPreBuildHeadFile*>(this)->getItemInfo(id);
    }

    LightMapPreBuildHeadFile::ItemHeadInfo* LightMapPreBuildHeadFile::getItemInfo( int id )
    {
        return &m_infos[id];
    }

    const LightMapPreBuildHeadFile::ItemHeadInfo* LightMapPreBuildHeadFile::getItemInfo( int xp, int yp ) const
    {
        return const_cast<LightMapPreBuildHeadFile*>(this)->getItemInfo(xp, yp);
    }

    LightMapPreBuildHeadFile::ItemHeadInfo* LightMapPreBuildHeadFile::getItemInfo( int xp, int yp )
    {
        return getItemInfo( toTexelID(xp, yp) );
    }

    const LightMapPreBuildHeadFile::ItemHeadInfo* LightMapPreBuildHeadFile::getItemInfo( int xv, int yv, int zv ) const
    {
        return const_cast<LightMapPreBuildHeadFile*>(this)->getItemInfo(xv, yv, zv);
    }

    LightMapPreBuildHeadFile::ItemHeadInfo* LightMapPreBuildHeadFile::getItemInfo( int xv, int yv, int zv )
    {
        return getItemInfo( toTexelID(xv, yv, zv) );
    }

    void LightMapPreBuildHeadFile::openFile( const String& file )
    {
        FILE* fp = FileLowAPI::open( file.c_str(), "rb" );

        FileLowAPI::readData( fp, &m_width, sizeof(int)*4 );

        // texel
        int cnt = m_width * m_height;
        if ( m_depth != 0 )
            cnt *= m_depth;

        m_infos.resize( cnt );
        
        if ( cnt > 0 )
            FileLowAPI::readData( fp, &m_infos[0], sizeof(m_infos[0])*cnt );

        FileLowAPI::close( fp );
    }

    void LightMapPreBuildHeadFile::saveFile()
    {
        if ( m_file.empty() )
            return;

        FILE* fp = FileLowAPI::open( m_file.c_str(), "wb" );

        FileLowAPI::writeData( fp, &m_width, sizeof(int)*4 );

        // texel
        if ( m_infos.size() )
            FileLowAPI::writeData( fp, &m_infos[0], sizeof(m_infos[0]) * (int)m_infos.size() );

        FileLowAPI::close( fp );
    }

    //////////////////////////////////////////////////////////////////////////
    void LightMapPreBuildFile::TexelItem::setMaxRayCount( int cnt )
    {
        m_rayInfos.resize( cnt );
        m_rayUseds.resize( cnt, false );
    }

    LightMapPreBuildFile::RayInfo* LightMapPreBuildFile::TexelItem::addRayInfo( int i )
    {
        m_rayUseds[i] = true;
        return &m_rayInfos[i];
    }

    const LightMapPreBuildFile::RayInfo* LightMapPreBuildFile::TexelItem::getRayInfo( int i ) const
    {
        if ( m_rayUseds[i] )
            return &m_rayInfos[i];
        return 0;
    }

    void LightMapPreBuildFile::TexelItem::clearRayInfos()
    {
        int rayCnt = (int)m_rayUseds.size();

        if ( rayCnt > 0 )
        {
            KHAOS_CLEAR_ZERO( &m_rayUseds[0], sizeof(uint8) * rayCnt );
        }
    }


    //////////////////////////////////////////////////////////////////////////
    Thread LightMapPreBuildFile::s_buildThread;
    LightMapPreBuildFile::RequestList LightMapPreBuildFile::s_requestList;
    Signal LightMapPreBuildFile::s_buildSing;
    Mutex  LightMapPreBuildFile::s_mtxRequest;
    volatile bool LightMapPreBuildFile::s_runBuild;

    LightMapPreBuildFile::LightMapPreBuildFile( LightMapPreBuildHeadFile* headFile, int threadID ) : 
        m_headFile(headFile), m_fp(0), m_threadID(threadID), m_modeForWrite(false),
        m_currUserQueueNo(0), m_currBuildQueueNo(0), m_queueNoTotal(0),
        m_buffReadPos(0), m_buffBuilding(false)
    {
        m_currItem.setMaxRayCount( m_headFile->getMaxRayCount() );
    }

    LightMapPreBuildFile::~LightMapPreBuildFile()
    {
        closeFile(); 
    }

    void LightMapPreBuildFile::openNewHeadFile( const String& fullFile )
    {
        m_modeForWrite = true;

        m_fp = FileLowAPI::open( fullFile.c_str(), "wb" );
        if ( !m_fp )
        {
            khaosOutputStr( "openNewHeadFile open failed: %s\n", fullFile.c_str() );
            return;
        }

        FileLowAPI::writeData( m_fp, m_queueNoTotal ); // 先占用个位置
    }

    void LightMapPreBuildFile::openExistHeadFile( const String& fullFile )
    {
        m_modeForWrite = false;

        if ( !m_fps.open( fullFile, true, 1024*1024 ) )
        {
            khaosOutputStr( "openExistHeadFile open failed: %s\n", fullFile.c_str() );
            return;
        }

        // 读取item数量
        m_fps.readData( m_queueNoTotal );

        // 预分配交换缓存
        const int maxBuffItems = 100;
        m_buffReadItemsA.resize( maxBuffItems );
        m_buffReadItemsB.resize( maxBuffItems );

        const int maxRayCnt = (int) m_currItem.m_rayInfos.size();

        for ( int i = 0; i < maxBuffItems; ++i )
        {
            m_buffReadItemsA[i].setMaxRayCount( maxRayCnt );
            m_buffReadItemsB[i].setMaxRayCount( maxRayCnt );
        }

        // 读取第一批数据
        _readNextInit();
    }

    void LightMapPreBuildFile::closeFile()
    {
        khaosAssert( !m_buffBuilding );
        _waitBuild();

        if ( m_fp )
        {
            if ( m_modeForWrite ) // 最后关闭前写入总数
            {
                FileLowAPI::locate( m_fp, 0 );
                FileLowAPI::writeData( m_fp, m_queueNoTotal );
            }

            FileLowAPI::close( m_fp );
            m_fp = 0;
        }

        m_fps.close();
    }

    LightMapPreBuildFile::TexelItem* LightMapPreBuildFile::setCurrTexelItem( int xp, int yp )
    {
        // 写入位置
        m_currItem.clearRayInfos();
        return &m_currItem;
    }

    LightMapPreBuildFile::TexelItem* LightMapPreBuildFile::setCurrTexelItem( int xv, int yv, int zv )
    {
        // 写入位置
        m_currItem.clearRayInfos();
        return &m_currItem;
    }

    void LightMapPreBuildFile::registerCurrTexelItem( int xp, int yp, int subIdx, int faceIdx )
    {
        // 注意，外部要保证此注册调用顺序和原子性
        LightMapPreBuildHeadFile::ItemHeadInfo* headInfo = m_headFile->getItemInfo( xp, yp );

        headInfo->subIdxInMesh = subIdx;
        headInfo->faceIdxInMesh = faceIdx;

        headInfo->threadID = m_threadID;
        headInfo->queueNo  = m_queueNoTotal;

        ++m_queueNoTotal;
    }

    void LightMapPreBuildFile::writeCurrTexelItem()
    {
        // 将一个纹素数据写入流
        const TexelItem* item = &m_currItem;

        // 基本数据
        //FileLowAPI::writeData( m_fp, item, TexelItem::basicInfoSize() );

        // 射线数据
        BinStreamWriter bsw;

        int rayCount = (int)item->m_rayInfos.size();
        int rayInterCount = 0;

        for ( int i = 0; i < rayCount; ++i )
        {
            const RayInfo* ri = item->getRayInfo(i);

            if ( ri )
            {
                bsw.write( i ); // 序号
                bsw.write( ri, sizeof(*ri) ); // 数据

                ++rayInterCount;
            }
        }

        int pack[2] = { rayInterCount, (int) bsw.getCurrentSize() };

        FileLowAPI::writeData( m_fp, pack );
        FileLowAPI::writeData( m_fp, bsw.getBlock(), bsw.getCurrentSize() );
    }
   
    bool LightMapPreBuildFile::canReadItem( int xp, int yp, int subIdx, int faceIdx ) const
    {
        return _canReadItem( m_headFile->toTexelID(xp, yp), subIdx, faceIdx );
    }

    bool LightMapPreBuildFile::canReadItem( int xv, int yv, int zv, int subIdx, int faceIdx ) const
    {
        return _canReadItem( m_headFile->toTexelID(xv, yv, zv), subIdx, faceIdx );
    }

    bool LightMapPreBuildFile::_canReadItem( int id, int subIdx, int faceIdx ) const
    {
        LightMapPreBuildHeadFile::ItemHeadInfo* headInfo = m_headFile->getItemInfo(id);

        // 检查源是否应当存在
        if ( headInfo->subIdxInMesh != subIdx ) // 像素属于此子模型？
            return false;

        if ( headInfo->faceIdxInMesh != faceIdx ) // 像素属于此子模型中的面？
            return false;

        // 检查库中是否存在记录
        if ( headInfo->threadID != m_threadID ) // 记录的像素属于这个线程？
            return false;

        // 那么必定有
        return true;
    }

    LightMapPreBuildFile::TexelItem* LightMapPreBuildFile::readNextTexelItem( int xp, int yp )
    {
        int id = m_headFile->toTexelID( xp, yp );
        TexelItem* item = _readNextTexelItem( id );
        return item;
    }

    LightMapPreBuildFile::TexelItem* LightMapPreBuildFile::readNextTexelItem( int xv, int yv, int zv )
    {
        int id = m_headFile->toTexelID( xv, yv, zv );
        TexelItem* item = _readNextTexelItem( id );
        return item;
    }

    void LightMapPreBuildFile::_readNextInit()
    {
        // 读取第一批数据
        if ( m_currBuildQueueNo == m_queueNoTotal )
            return;

        // 请求
        _postSelfTask();

        // 等待
        _waitBuild();

        // 交换现有和预备缓存
        m_buffReadItemsA.swap( m_buffReadItemsB );

        // 预备下次
        _postSelfTask();
    }

    void LightMapPreBuildFile::_postSelfTask()
    {
        khaosAssert( !m_buffBuilding );

        if ( m_currBuildQueueNo == m_queueNoTotal )
            return;

        m_buffBuilding = true;
        {
            LockGuard lg_(s_mtxRequest);
            s_requestList.push_back( this );
        }
        s_buildSing.set();
    }

    LightMapPreBuildFile::TexelItem* LightMapPreBuildFile::_readNextTexelItem( int requestID )
    {
        // 必定从此线程中取到此id的货
        khaosAssert( m_headFile->getItemInfo(requestID)->threadID == m_threadID ); // 应当在此线程

        int requestQueueNo = m_headFile->getItemInfo(requestID)->queueNo; // 在此线程的序列

        while ( m_currUserQueueNo < requestQueueNo ) // 当前序列未到那么不断读取
        {
            ++m_currUserQueueNo; // 取下一个
            ++m_buffReadPos;

            // 我们定位所在缓存位置
            int buffCnt = (int)m_buffReadItemsA.size();

            if ( m_buffReadPos < buffCnt ) // 处于当前可用缓存
            {
                // ...
            }
            else
            {
                // 不够了，我们从生产缓存者去取新的
                // 首先等待生产完成
                _waitBuild();

                // 交换现有和预备缓存
                m_buffReadItemsA.swap( m_buffReadItemsB );
                m_buffReadPos = 0;

                // 可能的话取下一次的数据
                _postSelfTask();
            }
        }
       
        TexelItem* item = &m_buffReadItemsA[m_buffReadPos];
        return item;
    }

    void LightMapPreBuildFile::_waitBuild()
    {
        while ( m_buffBuilding )
        {
            Sleep(1);
        }
    }

    void LightMapPreBuildFile::_buildNextBuff()
    {
        // 按顺序，取一定的批次
        khaosAssert( m_buffBuilding );

        int maxReads = (int)m_buffReadItemsB.size();

        for ( int i = 0; 
            i < maxReads && m_currBuildQueueNo != m_queueNoTotal;
            ++i, ++m_currBuildQueueNo )
        {
            TexelItem* item = &m_buffReadItemsB[i];

            _readOneTexelItem( item );
        }

        m_buffBuilding = false;
    }

    void LightMapPreBuildFile::_readOneTexelItem( TexelItem* item )
    {
        // 读取当前纹素数据从流
        struct
        {
            int rayCount;
            int rayInfoTotalBytes;
        } pack;

        KhaosStaticAssert( sizeof(pack) == sizeof(int) * 2 );

        // 基本数据
        //m_fps.readData( item, TexelItem::basicInfoSize() );

        // 射线数据
        item->clearRayInfos();

        m_fps.readData( pack );

        for ( int i = 0; i < pack.rayCount; ++i )
        {
            int idx;
            m_fps.readData( idx ); // 序号

            RayInfo* ri = item->addRayInfo( idx );
            m_fps.readData( *ri ); // 数据
        }
    }

    void LightMapPreBuildFile::startBuildThread()
    {
        s_runBuild = true;
        s_buildThread.bind_func( &LightMapPreBuildFile::_buildNextTexelItemStatic, 0 );
        s_buildThread.run();
    }

    void LightMapPreBuildFile::shutdownBuildThread()
    {
        s_runBuild = false;
        s_buildSing.set();
        s_buildThread.join();
    }

    void LightMapPreBuildFile::_buildNextTexelItemStatic( void* )
    {
        while ( s_runBuild )
        {
            LightMapPreBuildFile* file = 0;

            // 从队列中取一个
            {
                LockGuard lg_(s_mtxRequest);

                if ( s_requestList.size() )
                {
                    file = s_requestList.front();
                    s_requestList.pop_front();
                }
            }

            if ( file ) // 可以生产
            {
                file->_buildNextBuff();
            }
            else // 没有生产请求，等通知
            {   
                s_buildSing.wait();
            }
        }
    }
}

