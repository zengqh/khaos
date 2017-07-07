#include "KhaosPreHeaders.h"
#include "KhaosFileSystem.h"
#include "KhaosMsgQueueIds.h"

#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
    #include <direct.h>
    #include <stdio.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <stdio.h>
#endif

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    FILE* FileLowAPI::open( const String& fileName, const char* mode )
    {
        FILE* fp = fopen( fileName.c_str(), mode );
        return fp;
    }

    void FileLowAPI::close( FILE* fp )
    {
        fclose( fp );
    }

    void FileLowAPI::locate( FILE* fp, uint32 offset, int ori )
    {
        fseek( fp, offset, ori );
    }

    void FileLowAPI::locateEnd( FILE* fp )
    {
        locate( fp, 0, SEEK_END );
    }

    uint32 FileLowAPI::getLocation( FILE* fp )
    {
        return ftell( fp );
    }

    void FileLowAPI::writeData( FILE* fp, const void* data, int bytes )
    {
        fwrite( data, 1, bytes, fp );
    }

    void FileLowAPI::writeString( FILE* fp, const String& str )
    {
        writeData( fp, (int)str.size() );
        writeData( fp, str.c_str(), (int)str.size() );
    }

    void FileLowAPI::readData( FILE* fp, void* data, int bytes )
    {
        int reads = (int)fread( data, 1, bytes, fp );
        khaosAssert( reads == bytes );
    }

    void FileLowAPI::readString( FILE* fp, String& str )
    {
        int len = 0;
        readData( fp, len );

        str.resize(len);

        if ( len > 0 )
            readData( fp, &str[0], len );
    }

    //////////////////////////////////////////////////////////////////////////
    bool FileBufferStream::open( const String& fileName, bool readOnly, int bufferSize )
    {
        close();

        const char* mode = readOnly ? "rb" : "wb";
        m_fp = FileLowAPI::open( fileName, mode );
        if ( !m_fp )
            return false;

        if ( readOnly )
        {
            FileLowAPI::locateEnd( m_fp );
            int maxLen = FileLowAPI::getLocation( m_fp );

            if ( bufferSize > maxLen ) // 不能超过文件大小
                bufferSize = maxLen;

            FileLowAPI::locate( m_fp, 0 );
        }

        m_buffer = KHAOS_MALLOC_ARRAY_T( uint8, bufferSize );
        m_bufferSize = bufferSize;
        m_bufferPos  = bufferSize;
        return true;
    }

    void FileBufferStream::close()
    {
        if ( m_fp )
        {
            FileLowAPI::close( m_fp );
            m_fp = 0;
        }

        if ( m_buffer )
        {
            KHAOS_FREE( m_buffer );
            m_buffer = 0;
        }
    }

    void FileBufferStream::readData( void* data, int bytes )
    {
        // 缓存剩余字节数
        int leftBytes = m_bufferSize - m_bufferPos;

        // 剩余字节数不够
        if ( leftBytes < bytes )
        {
            // 先复制剩余字节数
            if ( leftBytes > 0 )
            {
                memcpy( data, m_buffer+m_bufferPos, leftBytes );
                data = (uint8*)data + leftBytes;
                bytes -= leftBytes;
            }

            // 读取新缓存
            FileLowAPI::readData( m_fp, m_buffer, m_bufferSize );
            m_bufferPos = 0;
        }

        // 读取另一半剩余字节
        khaosAssert( bytes <= m_bufferSize );
        memcpy( data, m_buffer+m_bufferPos, bytes );
        m_bufferPos += bytes;
    }

    void FileBufferStream::readString( String& str )
    {
        int len = 0;
        readData( len );

        str.resize(len);

        if ( len > 0 )
            readData( &str[0], len );
    }

    //////////////////////////////////////////////////////////////////////////
    inline void _create_path( const char* path )
    {
#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
        _mkdir(path);
#else
        mkdir(path, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
#endif
    }

//    inline bool _is_absolute_path( const char* path )
//    {
//#if KHAOS_PLATFORM == KHAOS_PLATFORM_WIN32
//        return isalpha(uchar(path[0])) && path[1] == ':';
//#else
//        return path[0] == '/' || path[0] == '\\';
//#endif
//    }

    //inline String _concatenate_path( const String& base, const String& name )
    //{
    //    if ( base.empty() || _is_absolute_path(name.c_str()) )
    //        return name;
    //    else
    //        return base + '/' + name;
    //}

    void _create_paths( const char* path, int searchStart, bool isPath )
    {
        char  buf[1024] = {0};
        char  ch        = '\0';
        char* dest      = buf;

        if ( searchStart > 0 )
        {
            memcpy( dest, path, sizeof(char) * searchStart );
            path += searchStart;
            dest += searchStart;
        }

        while ( (ch = *path) != '\0' )
        {
            *dest = ch;

            ++path;
            ++dest;

            if ( ch == '\\' || ch == '/' )
                _create_path( buf );
        }

        if ( isPath )
            _create_path( buf );
    }

    void _create_paths( const char* path, bool isPath )
    {
        _create_paths( path, 0, isPath );
    }

    //////////////////////////////////////////////////////////////////////////
    String normalizePath( const String& str )
    {
        if ( str.empty() )
            return str;

        char x = str.back();
        if ( x != '/' && x != '\\' )
            return str + "/";

        return str;
    }

    bool writeStringToFile( const char* str, const char* file )
    {
        FILE* fp = fopen( file, "wt" );
        if ( !fp )
            return false;

        fputs( str, fp );
        fclose(fp);
        return true;
    }

 

    //////////////////////////////////////////////////////////////////////////
    // 默认的文件系统实现
    DefaultFileSystem::DefaultFileSystem() : m_listener(0)
    {
        g_msgQueueManager->registerWorkMsg( MQQUEUE_HIGH, MQMSG_DEFAULT_FILE_SYSTEM, this );
    }

    DefaultFileSystem::~DefaultFileSystem()
    {
        _clearAllExtFiles();
    }

    void DefaultFileSystem::release()
    {
        KHAOS_DELETE this;
    }

    void DefaultFileSystem::setListener( IFileSystemListener* listener )
    {
        m_listener = listener;
    }

    void DefaultFileSystem::setRootPath( const String& path )
    {
        m_rootPath = normalizePath(path);

        _registerExtFile( "System.pak", "system/" );
        _registerExtFile( "Shader.pak", "shader/" );
    }

    const String& DefaultFileSystem::getRootPath() const
    {
        return m_rootPath;
    }

    void DefaultFileSystem::start()
    {
    }

    void DefaultFileSystem::shutdown()
    {
    }

    bool DefaultFileSystem::getFileData( const String& fileName, DataBuffer& buff )
    {
        return _getFileData( fileName, buff );
    }

    void DefaultFileSystem::getAsyncFileData( const String& fileName, void* userData )
    {
        g_msgQueueManager->postWorkMessage( MQQUEUE_HIGH, MQMSG_DEFAULT_FILE_SYSTEM, userData, 0, &fileName );
    }

    void DefaultFileSystem::releaseFileData( DataBuffer& buff )
    {
        KHAOS_FREE( (void*)buff.data );
    }

    bool DefaultFileSystem::writeFile( const String& fileName, DataBuffer& buff )
    {
        String fullFileName = getFullFileName(fileName);
        _create_paths( fullFileName.c_str(), false );

        FILE* fp = fopen( fullFileName.c_str(), "wb" );
        if ( !fp )
            return false;

        fwrite( buff.data, 1, buff.dataLen, fp );
        fclose(fp);
        return true;
    }

    String DefaultFileSystem::getFullFileName( const String& fileName ) const
    {
        return m_rootPath+fileName;//_concatenate_path( m_rootPath, fileName );
    }

    bool DefaultFileSystem::isExist( const String& fileName ) const
    {
        String fullFileName = getFullFileName(fileName);
        bool ok = _access( fullFileName.c_str(), 0 ) != -1;

        if ( !ok ) // 外围文件失败在加载内核包文件
            return _isExtFileExist( fileName );

        return ok;
    }

    void DefaultFileSystem::onWorkImpl( MsgQueueBase* mq, Message* msg )
    {
        // 线程回调处理加载文件
        khaosAssert( msg->msg == MQMSG_DEFAULT_FILE_SYSTEM );

        void* userData = msg->lparam;
        const String& strFile = msg->sparam;

        DataBuffer db;
        if ( _getFileData( strFile, db ) )
            m_listener->onGetFileResult( strFile, userData, db, IFileSystem::GR_OK );
        else
            m_listener->onGetFileResult( strFile, userData, db, IFileSystem::GR_FAILED );
    }

    void DefaultFileSystem::onCancelMsg( MsgQueueBase* mq, Message* msg )
    {
        // 中止一个下载消息
        khaosAssert( msg->msg == MQMSG_DEFAULT_FILE_SYSTEM );

        void* userData = msg->lparam;
        const String& strFile = msg->sparam;

        m_listener->onCancelGetFile( strFile, userData );
    }

    bool DefaultFileSystem::_getFileData( const String& file, DataBuffer& db )
    {
        // 加载文件到db，db所有权归调用者
        String fullFileName = getFullFileName(file);

        FILE* fp = fopen( fullFileName.c_str(), "rb" );
        if ( !fp )
        {
            if ( _getExtFileData( file, db ) ) // 外围文件失败在加载内核包文件
                return true;

            db.data = 0;
            db.dataLen = 0;
            return false;
        }

        fseek( fp, 0, SEEK_END );
        size_t len = ftell( fp );
        fseek( fp, 0, SEEK_SET );
        uint8* buff = KHAOS_MALLOC_ARRAY_T( uint8, len+1 ); // +1 for tex zero end
        fread( buff, 1, len, fp );
        fclose(fp);

        buff[len] = 0; // for tex

        db.data = buff;
        db.dataLen = (int)len;
        return true;
    }

    void DefaultFileSystem::_clearAllExtFiles()
    {
        for ( size_t i = 0; i < m_extFiles.size(); ++i )
        {
            ExtMountFile* file = m_extFiles[i];
            KHAOS_DELETE file;
        }

        m_extFiles.clear();
    }

    void DefaultFileSystem::_registerExtFile( const String& packFile, const String& path )
    {
        ExtMountFile* file = KHAOS_NEW ExtMountFile;
        file->path = path;
        file->pack.open( m_rootPath + packFile );

        m_extFiles.push_back( file );
    }

    int DefaultFileSystem::_isExtFile( const String& file1, int* packIdx ) const
    {
        if ( file1.empty() )
            return -1;

        String file = toLowerCopy(file1);

        int offset = 0;

        if ( file.front() == '/' || file.front() == '\\' )
            offset = 1;    

        for ( size_t i = 0; i < m_extFiles.size(); ++i )
        {
            const String& path = m_extFiles[i]->path;

            if ( isStartWith( &file[offset], path.c_str() ) )
            {
                *packIdx = (int)i;
                return offset + (int)path.size();
            }
        }

        return -1;
    }

    bool DefaultFileSystem::_isExtFileExist( const String& file ) const
    {
        int packIdx = 0;
        int idx = _isExtFile( file, &packIdx );
        if ( idx < 0 )
            return false;

        String fileInSys = file.substr( idx );
        return m_extFiles[packIdx]->pack.hasFile( fileInSys );
    }

    bool DefaultFileSystem::_getExtFileData( const String& file, DataBuffer& db )
    {
        int packIdx = 0;
        int idx = _isExtFile( file, &packIdx );
        if ( idx < 0 )
            return false;

        String fileInSys = file.substr( idx );
        int len = m_extFiles[packIdx]->pack.getFileSize( fileInSys );
        if ( len < 0 )
            return false;

        uint8* buff = KHAOS_MALLOC_ARRAY_T( uint8, len+1 ); // +1 for tex zero end
        m_extFiles[packIdx]->pack.readFile( fileInSys, buff );
        buff[len] = 0; // for tex

        db.data = buff;
        db.dataLen = len;
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    FileSystem* g_fileSystem = 0;

    FileSystem::DataBufferAuto::~DataBufferAuto()
    {
        if ( data )
            g_fileSystem->releaseFileData( *this );
    }

    FileSystem::FileSystem() : m_fileSys(0) 
    {
        khaosAssert( !g_fileSystem );
        g_fileSystem = this;
    }

    FileSystem::~FileSystem()
    {
        if ( m_fileSys )
            m_fileSys->release();
        g_fileSystem = 0;
    }

    void FileSystem::setFileSystem( IFileSystem* fs ) 
    {
        if ( m_fileSys )
            m_fileSys->release();
        m_fileSys = fs; 
    }

    void FileSystem::setListener( IFileSystemListener* listener )
    {
        m_fileSys->setListener( listener );
    }

    void FileSystem::setRootPath( const String& path )
    {
        m_fileSys->setRootPath( path );
    }

    const String& FileSystem::getRootPath() const
    {
        return m_fileSys->getRootPath();
    }

    void FileSystem::start()
    {
        m_fileSys->start();
    }

    void FileSystem::shutdown()
    {
        m_fileSys->shutdown();
    }

    bool FileSystem::getFileData( const String& fileName, DataBuffer& buff )
    {
        return m_fileSys->getFileData( fileName, buff );
    }

    void FileSystem::getAsyncFileData( const String& fileName, void* userData )
    {
        m_fileSys->getAsyncFileData( fileName, userData );
    }

    void FileSystem::releaseFileData( DataBuffer& buff )
    {
        m_fileSys->releaseFileData( buff );
    }

    bool FileSystem::writeFile( const String& fileName, DataBuffer& buff )
    {
        return m_fileSys->writeFile( fileName, buff );
    }

    String FileSystem::getFullFileName( const String& fileName ) const
    {
        return m_fileSys->getFullFileName( fileName );
    }

    bool FileSystem::isExist( const String& fileName ) const
    {
        return m_fileSys->isExist( fileName );
    }
}

