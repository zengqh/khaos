#pragma once
#include "KhaosStdTypes.h"
#include "KhaosMsgQueue.h"
#include "KhaosFilePack.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    String normalizePath( const String& str );
    bool   writeStringToFile( const char* str, const char* file );

    //////////////////////////////////////////////////////////////////////////
    struct FileLowAPI
    {
        // open/close
        static FILE* open( const String& fileName, const char* mode );
        static void close( FILE* fp );

        // locate
        static void locate( FILE* fp, uint32 offset, int ori = SEEK_SET );
        static void locateEnd( FILE* fp );
        
        static uint32 getLocation( FILE* fp );

        // write
        static void writeData( FILE* fp, const void* data, int bytes );

        template<class T>
        static void writeData( FILE* fp, const T& t )
        {
            writeData( fp, &t, sizeof(t) );
        }

        static void writeString( FILE* fp, const String& str );

        // read
        static void readData( FILE* fp, void* data, int bytes );

        template<class T>
        static void readData( FILE* fp, T& t )
        {
            readData( fp, &t, sizeof(t) );
        }

        template<class T>
        static T readData( FILE* fp )
        {
            T t;
            readData( fp, t );
            return t;
        }

        static void readString( FILE* fp, String& str );

        static String readString( FILE* fp )
        {
            String str;
            readString( fp, str );
            return str;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    class FileBufferStream : public AllocatedObject
    {
    public:
        FileBufferStream() : m_fp(0), m_buffer(0), m_bufferSize(0), m_bufferPos(0) {}
        ~FileBufferStream() { close(); }

    public:
        // open/close
        bool open( const String& fileName, bool readOnly, int bufferSize );
        void close();

        // read
        void readData( void* data, int bytes );

        template<class T>
        void readData( T& t )
        {
            readData( &t, sizeof(t) );
        }

        template<class T>
        T readData()
        {
            T t;
            readData( fp, t );
            return t;
        }

        void readString( String& str );

        String readString( FILE* fp )
        {
            String str;
            readString( str );
            return str;
        }

    private:
        FILE*  m_fp;
        uint8* m_buffer;
        int    m_bufferSize;
        int    m_bufferPos;
    };

    //////////////////////////////////////////////////////////////////////////
    // 文件系统接口
    struct DataBuffer
    {
        DataBuffer() : data(0), dataLen(0) {}

        const void* data;
        int         dataLen;
    };

    struct IFileSystemListener;

    struct IFileSystem
    {
        enum GetResult
        {
            GR_OK,
            GR_FAILED
        };

        virtual void release() = 0;

        // 事件
        virtual void setListener( IFileSystemListener* listener ) = 0;

        // 根目录
        virtual void setRootPath( const String& path ) = 0;

        virtual const String& getRootPath() const = 0;

        // 开启
        virtual void start() = 0;

        // 关闭
        virtual void shutdown() = 0;

        // 同步获取数据
        virtual bool getFileData( const String& fileName, DataBuffer& buff ) = 0;

        // 异步获取数据
        virtual void getAsyncFileData( const String& fileName, void* userData ) = 0;

        // 释放数据
        virtual void releaseFileData( DataBuffer& buff ) = 0;

        // 写文件
        virtual bool writeFile( const String& fileName, DataBuffer& buff ) = 0;

        // 其他操作
        virtual String getFullFileName( const String& fileName ) const = 0;

        virtual bool isExist( const String& fileName ) const = 0;
    };

    struct IFileSystemListener
    {
        virtual void onGetFileResult( const String& fileName, void* userData, DataBuffer& buff, IFileSystem::GetResult result ) = 0;
        virtual void onCancelGetFile( const String& fileName, void* userData ) = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    // 默认的文件系统实现
    class DefaultFileSystem : public AllocatedObject, public IFileSystem, public IMsgQueueListener
    {
        struct ExtMountFile : public AllocatedObject
        {
            String      path;
            FileUnpack  pack;
        };

        typedef vector<ExtMountFile*>::type ExtMountFileList;

    public:
        DefaultFileSystem();
        virtual ~DefaultFileSystem();

    private:
        // IFileSystem
        virtual void release();
        virtual void setListener( IFileSystemListener* listener );
        virtual void setRootPath( const String& path );
        virtual const String& getRootPath() const;
        virtual void start();
        virtual void shutdown();
        virtual bool getFileData( const String& fileName, DataBuffer& buff );
        virtual void getAsyncFileData( const String& fileName, void* userData );
        virtual void releaseFileData( DataBuffer& buff );
        virtual bool writeFile( const String& fileName, DataBuffer& buff );
        virtual String getFullFileName( const String& fileName ) const;
        virtual bool isExist( const String& fileName ) const;

    private:
        // IMsgQueueListener
        virtual void onWorkImpl( MsgQueueBase* mq, Message* msg );
        virtual void onCancelMsg( MsgQueueBase* mq, Message* msg );

    private:
        bool _getFileData( const String& file, DataBuffer& db );

        void _registerExtFile( const String& packFile, const String& path );
        void _clearAllExtFiles();

        int  _isExtFile( const String& file, int* packIdx ) const;
        bool _isExtFileExist( const String& file ) const;
        bool _getExtFileData( const String& file, DataBuffer& db );

    private:
        IFileSystemListener* m_listener;
        String               m_rootPath;
        ExtMountFileList     m_extFiles; // 外挂文件
    };

    //////////////////////////////////////////////////////////////////////////
    // 文件系统
    class FileSystem : public AllocatedObject
    {
    public:
        class DataBufferAuto : public DataBuffer, public Noncopyable
        {
        public:
            DataBufferAuto() {}
            explicit DataBufferAuto( const DataBuffer& db ) { *static_cast<DataBuffer*>(this) = db; }

            ~DataBufferAuto(); // 自动被g_fileSystem释放
        };

    public:
        FileSystem();
        ~FileSystem();

    public:
        // 可以更换文件系统实现，默认使用内置DefaultFileSystem
        void setFileSystem( IFileSystem* fs );

    public:
        void setListener( IFileSystemListener* listener );
        void setRootPath( const String& path );
        const String& getRootPath() const;
        void start();
        void shutdown();
        bool getFileData( const String& fileName, DataBuffer& buff );
        void getAsyncFileData( const String& fileName, void* userData );
        void releaseFileData( DataBuffer& buff );
        bool writeFile( const String& fileName, DataBuffer& buff );
        String getFullFileName( const String& fileName ) const;
        bool isExist( const String& fileName ) const;

    private:
        IFileSystem* m_fileSys;
    };

    extern FileSystem* g_fileSystem;
}

