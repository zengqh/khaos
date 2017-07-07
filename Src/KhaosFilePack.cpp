#include "KhaosPreHeaders.h"
#include "KhaosFilePack.h"
#include "KhaosBinStream.h"
#include "KhaosFileIterator.h"

namespace Khaos
{
    const int FILE_MAX_NAME_SIZE = 512;

    //////////////////////////////////////////////////////////////////////////
    inline uint8 _encodeByte( uint8 ch )
    {
        return ~ch + 22;
    }

    inline uint8 _decodeByte( uint8 ch )
    {
        return ~(ch - 22);
    }

    void _fileEncode( void* data, int size )
    {
        uint8* buf = (uint8*)data;

        for ( int i = 0; i < size; ++i )
        {
            buf[i] = _encodeByte( buf[i] );
        }
    }

    void _fileDecode( void* data, int size )
    {
        uint8* buf = (uint8*)data;

        for ( int i = 0; i < size; ++i )
        {
            buf[i] = _decodeByte( buf[i] );
        }
    }

    //////////////////////////////////////////////////////////////////////////
    FilePack::FilePack() : m_fp(0)
    {
    }

    FilePack::~FilePack()
    {
        close();
    }

    bool FilePack::open( const String& file )
    {
        close();

        m_fp = fopen( file.c_str(), "wb" );
        return m_fp != 0;
    }

    void FilePack::close()
    {
        if ( !m_fp )
            return;
        
        // 计算头文件信息大小
        int headInfoSize = _calcHeadInfo();

        // 写文件头
        BinStreamWriter bsw;
        _writeHeadInfo( bsw, headInfoSize );

        // 写入每个文件
        _writeFileData( bsw );

        // 打包文件
        fwrite( bsw.getBlock(), 1, bsw.getCurrentSize(), m_fp );
        fclose( m_fp );
        m_fp = 0;

        // 清理
        _clearItems();
    }

    int FilePack::_calcHeadInfo()
    {
        int headInfoSize = sizeof(int); // 文件数量

        int offset = 0; // 文件大小偏移

        KHAOS_FOR_EACH( ItemMap, m_files, it )
        {
            Item* item = it->second;
            item->offset = offset;
            offset += (int)item->fileData.size();

            headInfoSize += FILE_MAX_NAME_SIZE; // 文件名
            headInfoSize += sizeof(item->offset); // 定位偏移
            headInfoSize += sizeof(int); // 文件大小
        }

        return headInfoSize;
    }

    void FilePack::_writeHeadInfo( BinStreamWriter& bsw, int headInfoSize )
    {
        // 写入文件数量
        bsw.write( (int) m_files.size() );

        // 写入文件信息
        KHAOS_FOR_EACH( ItemMap, m_files, it )
        {
            Item* item = it->second;
            item->fileName.resize( FILE_MAX_NAME_SIZE, 0 );

            bsw.write( item->fileName.c_str(), item->fileName.size() );
            bsw.write( item->offset + headInfoSize );
            bsw.write( (int)item->fileData.size() );
        }
    }

    void FilePack::_writeFileData( BinStreamWriter& bsw )
    {
        KHAOS_FOR_EACH( ItemMap, m_files, it )
        {
            const Item* item = it->second;

            String tmp = item->fileData;
            if ( tmp.size() )
                _fileEncode( &tmp[0], (int)tmp.size() );

            bsw.write( tmp.c_str(), tmp.size() );
        }
    }

    void FilePack::_clearItems()
    {
        KHAOS_FOR_EACH( ItemMap, m_files, it )
        {
            Item* item = it->second;
            KHAOS_DELETE item;
        }

        m_files.clear();
    }

    void FilePack::addFile( const String& file, const String& data )
    {
        if ( m_files.find( file ) != m_files.end() )
            return;

        Item* item = KHAOS_NEW Item;

        item->fileName = file;
        item->fileData = data;

        m_files[file] = item;
    }

    //////////////////////////////////////////////////////////////////////////
    FileUnpack::FileUnpack() : m_fp(0)
    {
    }

    FileUnpack::~FileUnpack()
    {
        close();
    }

    bool FileUnpack::open( const String& file )
    {
        close();

        m_fp = fopen( file.c_str(), "rb" );
        if ( !m_fp )
            return false;

        _readHeadInfo();
        return true;
    }

    void FileUnpack::_readHeadInfo()
    {
        // 写入文件数量
        int fileCnt = 0;
        fread( &fileCnt, 1, sizeof(fileCnt), m_fp );

        // 写入文件信息
        for ( int i = 0; i < fileCnt; ++i )
        {
            char name[FILE_MAX_NAME_SIZE] = {};
            fread( name, 1, FILE_MAX_NAME_SIZE, m_fp );

            Item item;
            fread( &item.offset, 1, sizeof(item.offset), m_fp );
            fread( &item.size, 1, sizeof(item.size), m_fp );
            
            String lowname = toLowerCopy( name );
            m_files[lowname] = item;
        }
    }

    void FileUnpack::close()
    {
        if ( m_fp )
        {
            fclose( m_fp );
            m_fp = 0;
        }
    }

    bool FileUnpack::hasFile( const String& file1 ) const
    {
        String file = toLowerCopy(file1);
        return m_files.find( file ) != m_files.end();
    }

    int FileUnpack::getFileSize( const String& file1 ) const
    {
        String file = toLowerCopy(file1);
        ItemMap::const_iterator it = m_files.find( file );
        if ( it == m_files.end() )
            return -1;

        const Item& item = it->second;
        return item.size;
    }

    bool FileUnpack::readFile( const String& file1, String& data )
    {
        String file = toLowerCopy(file1);
        ItemMap::iterator it = m_files.find( file );
        if ( it == m_files.end() )
            return false;

        const Item& item = it->second;
        data.resize( item.size );

        fseek( m_fp, item.offset, SEEK_SET );

        if ( item.size > 0 )
        {
            fread( &data[0], 1, item.size, m_fp );
            _fileDecode( &data[0], item.size );
        }

        return true;
    }

    bool FileUnpack::readFile( const String& file1, void* data )
    {
        String file = toLowerCopy(file1);
        ItemMap::iterator it = m_files.find( file );
        if ( it == m_files.end() )
            return false;

        const Item& item = it->second;

        fseek( m_fp, item.offset, SEEK_SET );

        if ( item.size > 0 )
        {
            fread( data, 1, item.size, m_fp );
            _fileDecode( data, item.size );
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    struct GetFileOp_
    {
        FilePack* fp;

        bool _getFileData( const String& fullFileName, String& data )
        {
            FILE* fp = fopen( fullFileName.c_str(), "rb" );
            if ( !fp )
            {
                data.clear();
                return false;
            }

            fseek( fp, 0, SEEK_END );
            size_t len = ftell( fp );
            fseek( fp, 0, SEEK_SET );

            data.resize( len );
            if ( len > 0 )
                fread( &data[0], 1, len, fp );

            fclose(fp);

            return true;
        }

        bool operator()( const String& basePath, const String& relPath, const file_iterator& it )
        {
            String relFileName = relPath + it.file_name();
            String fullName = basePath + relFileName;
            String data;
            
            if ( !_getFileData( fullName, data ) )
                return true;

            fp->addFile( relFileName, data );
            return true;
        }
    };
   
    
    bool exportPathToPack( const String& packFile, const String& basePath, bool isRecursion )
    {
        FilePack filePack;
        if ( !filePack.open( packFile ) )
            return false;

        GetFileOp_ op;
        op.fp = &filePack;

        travel_path( basePath, op, isRecursion );
        return true;
    }


}

