#pragma once
#include "KhaosMesh.h"
#include "KhaosRect.h"
#include "KhaosColor.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    struct ILightMapProcessCallback
    {
        virtual void onPerSubMeshBegin( SubMesh* subMesh, int subIdx ) = 0;
        virtual void onPerFaceBegin( int threadID, int faceIdx, const Vector3& faceTangent, const Vector3& faceBinormal, const Vector3& faceNormal ) = 0;
        virtual int  onDiscardTexel( int threadID, int xp, int yp ) = 0;
        virtual void onRegisterTexel( int threadID, int xp, int yp ) = 0;
        virtual void onPerTexel( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv ) = 0;
        virtual void onPerSubMeshEnd( SubMesh* subMesh ) = 0;
    };

    class LightMapProcess : public AllocatedObject
    {
        typedef vector<Vector3>::type  Vec3List;
        typedef vector<Vector4>::type  Vec4List;

        struct TexelInfo
        {
            TexelInfo() : used(false) {}
            Vector3 uvwG;
            Vector2 err;
            bool    used;
        };

        typedef vector<TexelInfo>::type TexelInfoArray;

    public:
        LightMapProcess( Mesh* mesh, const Matrix4& matWorld, int texWidth, int texHeight );

        void general( ILightMapProcessCallback* ev, int threads );

        void setNeedCalc( bool b ) { m_needCalc = b; }
        void setNeedTangent( bool b ) { m_needTangent = b; }
        void setNeedFaceTanget( bool b ) { m_needFaceTangent = b; }

    public:
        SubMesh* getCurrSubMesh() const { return m_currSubMesh; }

    private:
        static void _generalFaceStatic( int threadID, void* para, int f );
        void _generalFace( int threadID, int f );

        void _calcUVAABB( const Vector2& tex0, const Vector2& tex1, const Vector2& tex2,
            Vector2& minUV, Vector2& maxUV ) const;

        void _convertPixelAABB( const Vector2& minUV, const Vector2& maxUV,
            IntVector2& minPix, IntVector2& maxPix );

        Vector2 _xy2Center( const IntVector2& xy ) const;
        Vector2 _uv2pxy( const Vector2& uv ) const;

        bool _canDoCalc( int threadID, int xp, int yp, 
            const Vector2& tex0, const Vector2& tex1, const Vector2& tex2, 
            Vector3& uvwG );

        Vector2 _getUVWGLenErr( const Vector3& uvwG ) const;
        bool    _isUVWLenErrLess( const Vector2& errA, const Vector2& errB ) const;

    private:
        Mesh*   m_mesh;
        Matrix4 m_matWorld;
        int     m_texWidth;
        int     m_texHeight;

        SubMesh*      m_currSubMesh;
        Vec3List      m_currVBPos;
        Vec3List      m_currVBNormal;
        Vec4List      m_currVBTangent;
        VertexBuffer* m_currVB;
        IndexBuffer*  m_currIB;

        TexelInfoArray m_texels;
        ILightMapProcessCallback* m_event;

        Khaos::Mutex m_mtxTexInfos;

        bool m_needCalc;
        bool m_needTangent;
        bool m_needFaceTangent;
    };

    //////////////////////////////////////////////////////////////////////////
    struct ILightMapSource
    {
        virtual int   getLayerCount() const = 0;
        virtual int   getWidth() const = 0;
        virtual int   getHeight() const = 0;

        virtual void  setVal( int layer, int x, int y, float val ) = 0;
        virtual float getVal( int layer, int x, int y ) const = 0;

        virtual void  setUsed( int x, int y, bool b ) = 0;
        virtual bool  isUsed( int x, int y ) const = 0;
    };

    class LightMapPostBase : public AllocatedObject
    {
    protected:
        typedef vector<float>::type FloatArray;

        struct Texel 
        {
            int x, y;
            FloatArray vals;
        };

        typedef vector<Texel>::type TexelList;

    public:
        LightMapPostBase() : m_source(0), m_width(0), m_height(0), m_layerCnt(0), m_userData(0) {}

        void setUserData( void* ud ) { m_userData = ud; }
        void* getUserData() const { return m_userData; }

    protected:
        void _init( ILightMapSource* lmSrc );
        bool _hasVal( int x, int y ) const;

        void _copyTexelList( const TexelList& texList );

    protected:
        ILightMapSource* m_source;
        int m_width;
        int m_height;
        int m_layerCnt;

        TexelList m_tempData;

        void* m_userData;
    };

    class LightMapRemoveZero : public LightMapPostBase
    {
        typedef Vector3 (*FUNC)( LightMapRemoveZero* fiter, ILightMapSource* lmSrc, int x, int y );

        struct Position
        {
            Position() : x(0), y(0) {}
            Position( int x1, int y1 ) : x(x1), y(y1) {}

            int x, y;

            bool operator<( const Position& rhs ) const
            {
                return *(uint64*)(&x) < *(uint64*)(&rhs.x);
            }
        };

        typedef set<Position>::type PositionSet;

    public:
        LightMapRemoveZero() : m_func(0), m_blackVal(0), m_deltaVal(0), m_useSingle(true) {}

        void filter( ILightMapSource* lmSrc, FUNC func, float blackVal, float deltaVal, bool useSingle );

    private:
        void _checkZero( int x, int y );
        void _checkEdge( int x, int y );
        void _blurEdge();
        void _blurPos( int x, int y );

        bool _isBlackPoint( const Vector3& val ) const;
        bool _isLargeSelf( const Vector3& neighRef, const Vector3& selfRef ) const;

    private:
        FUNC  m_func;
        
        PositionSet m_edgePoints;

        float m_blackVal;
        float m_deltaVal;
        bool  m_useSingle;
    };

    class LightMapPostFill : public LightMapPostBase
    {
    public:
        void fill( ILightMapSource* lmSrc, int times );

    private:
        bool _getNeighborVal( int x, int y, float* val ) const;
    };

    class LightMapPostBlur : public AllocatedObject
    {
    public:
        LightMapPostBlur() : m_source(0) {}

        void filter( ILightMapSource* lmSrc, ILightMapSource* lmDest );

    private:
        float _getVal( int layer, int x, int y ) const;

    private:
        ILightMapSource* m_source;
    };

    //////////////////////////////////////////////////////////////////////////
    class SimpleLightMap : public AllocatedObject, public ILightMapSource
    {
    public:
        // only support r, rgb, rgba mode
        enum
        {
            CHANNEL_R, CHANNEL_G, CHANNEL_B, CHANNEL_A
        };

        typedef vector<float>::type FloatArray;
        typedef vector<bool>::type  BoolArray;

    public:
        SimpleLightMap();
        ~SimpleLightMap();

        void setSize( int width, int height, int channels );
        void clearChannel( int channel, float val );
        void clearAll( const Color& clr );
        void clearUsed();

        void setRed( int x, int y, float r );
        void setGreen( int x, int y, float g );
        void setBlue( int x, int y, float b );
        void setAlpha( int x, int y, float a );
        void setColor( int x, int y, const Color& clr );

        float getRed( int x, int y ) const;
        float getGreen( int x, int y ) const;
        float getBlue( int x, int y ) const;
        float getAlpha( int x, int y ) const;
        Color getColor( int x, int y ) const;
        Color getColorByUV( const Vector2& uv ) const;
        
        void addLightMap( SimpleLightMap* litMap, const Color& scale, bool hasAlpha );
        void scaleColor( const Color& scale, bool hasAlpha );

        void saveFile( PixelFormat format, pcstr file );

    public:
        virtual int   getLayerCount() const { return m_channels; }
        virtual int   getWidth() const { return m_width; }
        virtual int   getHeight() const { return m_height; }

        virtual void  setVal( int layer, int x, int y, float val );
        virtual float getVal( int layer, int x, int y ) const;

        virtual void  setUsed( int x, int y, bool b );
        virtual bool  isUsed( int x, int y ) const;

    private:
        FloatArray     m_data;
        BoolArray      m_used;
        int            m_width;
        int            m_height;
        int            m_channels;
    };

    //////////////////////////////////////////////////////////////////////////
    class SimpleVolumeMap : public AllocatedObject
    {
    public:
        typedef vector<Color>::type ColorArray;

    public:
        SimpleVolumeMap() : m_width(0), m_height(0), m_depth(0) {}

        void init( int w, int h, int d, const Color& clr );

        void setColor( int x, int y, int z, const Color& clr );
        const Color& getColor( int x, int y, int z ) const;
        Color& getColor( int x, int y, int z );

        const Color* getData() const;
        Color* getData();
        
        int getDataSize() const { return (int)m_data.size(); }
        int getWidth() const { return m_width; }
        int getHeight() const { return m_height; }
        int getDepth() const { return m_depth; }

        void saveFile( PixelFormat format, pcstr file );

    private:
        ColorArray m_data;
        int        m_width;
        int        m_height;
        int        m_depth;
    };

    //////////////////////////////////////////////////////////////////////////
    class LightMapPreBuildHeadFile : public AllocatedObject
    {
    public:
        struct ItemHeadInfo : public AllocatedObject
        {
            ItemHeadInfo() 
            {
                KhaosStaticAssert( sizeof(*this) == sizeof(uint64) );
                *(uint64*)this = -1LL;
            }

            // 这些信息用来校验生成端的来源
            uint subIdxInMesh  : 12; // 对应模型中的哪个子模型，每个模型中最大子模型2^12=4096个
            uint faceIdxInMesh : 20; // 对应模型中的哪个子模型的哪个面，每个子模型中最大面数2^20=1M个

            // 这些信息用来校验存储端的信息是否有效
            uint threadID : 6;  // 位于哪个线程的存储库， 最大2^6 = 64个线程
            uint queueNo  : 26; // 在此线程的存储库顺序号， 最大2^64 = 8k * 8k个像素
        };

        typedef vector<ItemHeadInfo>::type InfoList; // 每个像素的信息

    public:
        LightMapPreBuildHeadFile();
        ~LightMapPreBuildHeadFile();

    public:
        void setSize( int w, int h );
        void setSize( int w, int h, int d );
        void setMaxRayCount( int cnt );

        int getWidth()  const { return m_width; }
        int getHeight() const { return m_height; }
        int getDepth()  const { return m_depth; }
        int getMaxRayCount() const { return m_maxRayCount; }

    public:
        int getItemInfoCount() const { return (int)m_infos.size(); }
        
        int toTexelID( int xp, int yp ) const;
        int toTexelID( int xv, int yv, int zv ) const;

        const ItemHeadInfo* getItemInfo( int id ) const;
        ItemHeadInfo* getItemInfo( int id );

        const ItemHeadInfo* getItemInfo( int xp, int yp ) const;
        ItemHeadInfo* getItemInfo( int xp, int yp );

        const ItemHeadInfo* getItemInfo( int xv, int yv, int zv ) const;
        ItemHeadInfo* getItemInfo( int xv, int yv, int zv );

    public:
        void setFile( const String& file ) { m_file = file; }
        void openFile( const String& file );
        void saveFile();

    private:
        int  m_width;
        int  m_height;
        int  m_depth;
        int  m_maxRayCount;

        String m_file;
        InfoList m_infos;
    };

    class LightMapPreBuildFile : public AllocatedObject
    {
    public:
        enum
        {
            RIFLAG_OTHER_BACK_SURFACE = 0x1
        };

        struct RayInfo : public AllocatedObject
        {
            RayInfo()
            { 
                KhaosStaticAssert(sizeof(*this) == sizeof(uint32));
                *(uint32*)this = 0;
            }

            bool isOtherBackFace() const { return (flag & RIFLAG_OTHER_BACK_SURFACE) != 0; }

            uint32 flag : 4;          // 16个标记
            uint32 globalFaceID : 28; // 也就是说全局面数最大2^28，足够了
        };

        typedef vector<RayInfo>::type RayInfoList;
        typedef vector<uint8>::type RayUsedList;

        struct TexelItem : public AllocatedObject
        {
            TexelItem() {}

            //static int basicInfoSize() { return sizeof(int); }

            void setMaxRayCount( int cnt );
            RayInfo* addRayInfo( int i );
            const RayInfo* getRayInfo( int i ) const;
            void clearRayInfos();

            RayInfoList m_rayInfos; // 所有射线交点信息
            RayUsedList m_rayUseds; // 射线的使用标记，false表示没有交点
        };

        typedef vector<TexelItem>::type TexelItemList;
        typedef list<LightMapPreBuildFile*>::type RequestList;

    public:
        LightMapPreBuildFile( LightMapPreBuildHeadFile* headFile, int threadID );
        ~LightMapPreBuildFile();

        // file op
        void openNewHeadFile( const String& file );
        void openExistHeadFile( const String& file );
        void closeFile();

        // for write
        void registerCurrTexelItem( int xp, int yp, int subIdx, int faceIdx );
        TexelItem* setCurrTexelItem( int xp, int yp );
        TexelItem* setCurrTexelItem( int xv, int yv, int zv );
        void writeCurrTexelItem();

        // for read
        bool canReadItem( int xp, int yp, int subIdx, int faceIdx ) const;
        bool canReadItem( int xv, int yv, int zv, int subIdx, int faceIdx ) const;

        TexelItem* readNextTexelItem( int xp, int yp );
        TexelItem* readNextTexelItem( int xv, int yv, int zv );

    private:
        bool _canReadItem( int id, int subIdx, int faceIdx ) const;

        void _readNextInit();
        void _postSelfTask();
        TexelItem* _readNextTexelItem( int requestID );

        void _buildNextBuff();
        void _readOneTexelItem( TexelItem* item );
        void _waitBuild();

        // 线程任务操作
    public:
        static void startBuildThread();
        static void shutdownBuildThread();

    private:
        static void _buildNextTexelItemStatic( void* );

    private:
        LightMapPreBuildHeadFile* m_headFile;
        FILE*                     m_fp;   
        FileBufferStream          m_fps;
        int                       m_threadID;
        bool                      m_modeForWrite;

        // 写
        TexelItem     m_currItem;

        // 读取
        int m_currUserQueueNo; // 请求者的当前序号
        int m_currBuildQueueNo; // 创造者的当前序号
        int m_queueNoTotal; // 总共的数量

        TexelItemList m_buffReadItemsA; // 可用缓冲A和正读取缓冲B，可交替迭代
        TexelItemList m_buffReadItemsB;
        int           m_buffReadPos; // 在缓冲A的读取位置
        volatile bool m_buffBuilding; // 表明正在读取缓存B，读完后待机，等待与A交换

    private:
        // 线程管理
        static Thread s_buildThread;
        static RequestList s_requestList;
        static Signal s_buildSing;
        static Mutex  s_mtxRequest;
        static volatile bool s_runBuild;
    };
}


