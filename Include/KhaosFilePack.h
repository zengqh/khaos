#pragma once
#include "KhaosStdTypes.h"

namespace Khaos
{
    class BinStreamWriter;

    //////////////////////////////////////////////////////////////////////////
    class FilePack : public AllocatedObject
    {
        struct Item : public AllocatedObject
        {
            Item() : offset(0) {}
            String fileName;
            String fileData;
            int    offset;
        };

        typedef map<String, Item*>::type ItemMap;

    public:
        FilePack();
        ~FilePack();

        bool open( const String& file );
        void close();

        void addFile( const String& file, const String& data );
        
    private:
        int  _calcHeadInfo();
        void _writeHeadInfo( BinStreamWriter& bsw, int headInfoSize );
        void _writeFileData( BinStreamWriter& bsw );

        void _clearItems();

    private:
        String  m_packName;
        String  m_basePath;
        ItemMap m_files;
        FILE*   m_fp;
    };

    //////////////////////////////////////////////////////////////////////////
    class FileUnpack : public AllocatedObject
    {
        struct Item : public AllocatedObject
        {
            Item() : offset(0), size(0) {}
            int offset;
            int size;
        };

        typedef map<String, Item>::type ItemMap;

    public:
        FileUnpack();
        ~FileUnpack();

        bool open( const String& file );
        void close();

        bool hasFile( const String& file ) const;
        int  getFileSize( const String& file ) const;

        bool readFile( const String& file, String& data );
        bool readFile( const String& file, void* data );

    private:
        void FileUnpack::_readHeadInfo();

    private:
        ItemMap m_files;
        FILE*   m_fp;
    };

    //////////////////////////////////////////////////////////////////////////
    bool exportPathToPack( const String& packFile, const String& basePath, bool isRecursion );
}

