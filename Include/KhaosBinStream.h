#pragma once
#include "KhaosStdTypes.h"
#include "KhaosDebug.h"

namespace Khaos
{
    class BinStreamWriter
    {
    public:
        BinStreamWriter( uint needBytes = 1024 ) : 
            m_block((uint8*)KHAOS_MALLOC(needBytes)), m_size(needBytes), m_currSize(0), m_manager(true)
        {
        }

        BinStreamWriter( void* block, uint size ) :
            m_block((uint8*)block), m_size(size), m_currSize(0), m_manager(false)
        {
        }

        ~BinStreamWriter()
        {
            if ( m_manager )
                KHAOS_FREE(m_block);
        }

        void write( const void* data, uint len )
        {
            khaosAssert(data);

            uint newCurrSize = m_currSize + len;

            if ( m_manager )
            {
                // 容量不够自动扩展
                if ( newCurrSize > m_size )
                {
                    uint8* newBuff = (uint8*)KHAOS_MALLOC(newCurrSize * 2);
                    memcpy( newBuff, m_block, m_currSize );
                    KHAOS_FREE(m_block);

                    m_block = newBuff;
                    m_size = newCurrSize * 2;
                }
            }

            if ( newCurrSize <= m_size )
            {
                memcpy( m_block+m_currSize, data, len );
                m_currSize = newCurrSize;
            }
            else
            {
                khaosAssert(0);
            }
        }

        template<typename T>
        void write( const T& t )
        {
            write( &t, sizeof(t) );
        }

        void writeString( const String& str )
        {
            int len = (int)str.size()+1;
            int fillLen = (len + 3) / 4 * 4; // 对齐4

            write( fillLen );
            write( str.c_str(), len );

            if ( fillLen > len )
            {
                static const char ch[4] = {};
                write( ch, fillLen - len );
            }
        }

        template<class T>
        void writeVector( const T& vec )
        {
            write( (int)vec.size() );
            
            if ( vec.size() )
                write( &vec[0], sizeof(vec[0]) * (int)vec.size() );
        }

        void clear()
        {
            m_currSize = 0;
        }

    public:
        uint8* getBlock()       const { return m_block; }
        uint   getSize()        const { return m_size; }
        uint   getCurrentSize() const { return m_currSize; }

    private:
        uint8* m_block;
        uint   m_size;
        uint   m_currSize;
        bool   m_manager;
    };

    class BinStreamReader
    {
    public:
        BinStreamReader( const void* block, uint size ) :
            m_block((const uint8*)block), m_size(size), m_currSize(0)
        {
        }

        void read( void* data, uint len )
        {
            khaosAssert(data);

            uint newCurrSize = m_currSize + len;

            if ( newCurrSize <= m_size )
            {
                memcpy( data, m_block+m_currSize, len );
                m_currSize = newCurrSize;
            }
            else
            {
                khaosAssert(0);
            }
        }

        template<typename T>
        void read( T& t )
        {
            read( &t, sizeof(t) );
        }

        template<typename T>
        T read()
        {
            T t;
            read( &t, sizeof(t) );
            return t;
        }

        void readString( String& str )
        {
            int strLen = 0;
            read( strLen );

            str = (const char*)getCurPos();
            skip( strLen );
        }

        template<class T>
        void readVector( T& vec )
        {
            int vec_size = 0;
            read( vec_size );

            vec.resize(vec_size);

            if ( vec_size )
                read( &vec[0], sizeof(vec[0]) * vec_size );
        }

        void skip( uint len )
        {
            uint newCurrSize = m_currSize + len;

            if ( newCurrSize <= m_size )
            {
                m_currSize = newCurrSize;
            }
            else
            {
                khaosAssert(0);
            }
        }

        void rewind()
        {
            m_currSize = 0;
        }

    public:
        const uint8* getBlock()       const { return m_block; }
        const uint8* getCurPos()      const { return m_block + m_currSize; }
        uint         getSize()        const { return m_size; }
        uint         getCurrentSize() const { return m_currSize; }

    private:
        const uint8* m_block;
        uint         m_size;
        uint         m_currSize;
    };
}

