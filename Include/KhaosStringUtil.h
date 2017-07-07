#pragma once
#include "KhaosStdTypes.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
#define KHAOS_STRFMT(buff, size, fmt) \
    char buff[size] = {}; \
    va_list args; \
    va_start( args, fmt ); \
    vsnprintf( buff, sizeof(buff)-1, fmt, args ); \
    va_end( args );

#define KHAOS_STRFMT_LN(buff, size, fmt) \
    char buff[size] = {}; \
    va_list args; \
    va_start( args, fmt ); \
    int write_n_ = vsnprintf( buff, sizeof(buff)-1, fmt, args ); \
    va_end( args ); \
    if ( 0 <= write_n_ && write_n_ < sizeof(buff)-1 ) \
    buff[write_n_] = '\n';


    //////////////////////////////////////////////////////////////////////////
    typedef vector<String>::type                StringVector;
    typedef list<String>::type                  StringList;
    typedef unordered_map<String, String>::type StringMap;

    //////////////////////////////////////////////////////////////////////////
    // for string class functions
    inline bool isBlankChar( char ch )
    {
        return ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t';
    }

    inline bool isBlankChar( wchar_t ch )
    {
        return ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t';
    }

    template<class StrType>
    void trimLeft( StrType& str )
    {
        int len = int(str.size());
        int it = 0;

        while ( it < len ) // 从头开始找一个非空
        {
            if ( !isBlankChar(str[it]) )
                break;
            ++it;
        }

        if ( it > 0 ) // 删除开头到非空之前的
            str.erase( str.begin(), str.begin() + it );
    }

    template<class StrType>
    void trimRight( StrType& str )
    {
        int len = int(str.size());
        int rit = len - 1;

        while ( rit >= 0 ) // 从末尾开始找一个非空
        {
            if ( !isBlankChar(str[rit]) )
                break;
            --rit;
        }

        ++rit; // 非空的后一个开始全部删除
        if ( rit < len )
            str.erase( str.begin() + rit, str.end() );
    }

    template<class T>
    void trim( T& str )
    {
        trimLeft( str );
        trimRight( str );
    }

    template<class T>
    T trimCopy( const T& str )
    {
        T tmp(str);
        trim(tmp);
        return tmp;
    }

    template<class T, class U, class V>
    void splitString( U& outList, const T& str, const V* sp, bool skipEmpty )
    {
        outList.clear();
        if ( str.empty() )
            return;

        size_t it = 0;
        size_t ith = str.find_first_of( sp, it );

        while ( ith != T::npos )
        {
            if ( !skipEmpty || ith > it )
                outList.push_back( str.substr(it, ith-it) );

            it = ith + 1;
            ith = str.find_first_of( sp, it );
        }

        if ( !skipEmpty || it < str.size() )
            outList.push_back( str.substr(it) );
    }

    template<class T, class U>
    void splitStringByKey( U& outList, const T& str, const U& sp )
    {
        outList.clear();
        if ( str.empty() )
            return;

        size_t str_len = str.size();
        size_t i_last = 0;
        size_t i = 0;

        for ( ; ; )
        {
            bool found = false;

            for ( typename U::const_iterator it_sp = sp.begin(), it_spe = sp.end(); it_sp != it_spe; ++it_sp )
            {
                const T& str_sp = *it_sp;

                if ( str.compare( i, str_sp.size(), str_sp ) == 0 ) // 找到关键字
                {
                    // val
                    T tmp = str.substr(i_last, i-i_last);
                    trim(tmp);
                    if ( tmp.size() )
                        outList.push_back(tmp);

                    // key
                    outList.push_back( str_sp );

                    // next
                    i_last = i + str_sp.size();
                    i = i_last;
                    found = true;
                    break;
                }
            } // end of search key

            if ( !found ) // 没找到继续下一个
                ++i;
            
            // 结束
            if ( i == str_len )
            {
                if ( i_last < i ) // 最后个值不能空
                {
                    // val
                    T tmp = str.substr(i_last, i-i_last);
                    trim(tmp);
                    if ( tmp.size() )
                        outList.push_back(tmp);
                }

                break;
            }
        } // end of search string
    }

    template<class T>
    void replaceString( T& str, const T& strOld, const T& strNew )
    {
        size_t strOldSize = strOld.size();
        size_t strNewSize = strNew.size();
        
        size_t pos = str.find( strOld );

        while ( pos != T::npos )
        {
            str.replace( pos, strOldSize, strNew );
            pos = str.find( strOld, pos+strNewSize );
        }
    }

    template<class T>
    void splitFileName( const T& fileName, T& mainName, T& extName )
    {
        size_t pos = fileName.find_last_of( '.' );

        if ( pos != T::npos )
        {
            mainName = fileName.substr( 0, pos );
            extName  = fileName.substr( pos+1 );
        }
        else
        {
            mainName = fileName;
            extName.clear();
        }
    }

    // for c string functions 
    template<typename T>
    int findString( const T* str, T chMatch )
    {
        const T* pos = str;
        T ch;

        while ( (ch = *pos) != 0 )
        {
            if ( ch == chMatch )
                return (int)(pos - str);
            ++pos;
        }

        return -1;
    }

    template<typename T>
    int findString( const T* str, const T* strMatchList )
    {
        const T* pos = str;
        T ch;

        while ( (ch = *pos) != 0 )
        {
            if ( findString(strMatchList, ch) >= 0 )
                return (int)(pos - str);
            ++pos;
        }

        return -1;
    }

    template<typename T>
    void copyString( T* dest, const T* src )
    {
        while ( *dest++ = *src++ );
    }

    template<typename T>
    bool isStartWith( const T* s1, const T* s2 )
    {
        T ch1 = (T)-1, ch2 = (T)-1;

        while ( (ch2 = *s2) && (ch1 = *s1) ) // s2列前
        {
            if ( ch1 != ch2 )
                return 0; // false

            ++s1;
            ++s2;
        }

        return ( ch2 == 0 ); // s2串结束，说明是子串，ok
    }

    template<class T>
    bool isEndWith( const T& s1, const T& s2 )
    {
        int s1_len = (int)s1.size();
        int s2_len = (int)s2.size();

        if ( s1_len < s2_len )
            return false;

        return s1.compare(s1_len-s2_len, s2_len, s2) == 0;
    }

    inline int stringToInt( const char* str )
    {
        return strtol( str, 0, 10 );
    }

    inline int stringToInt( const wchar_t* str )
    {
        return wcstol( str, 0, 10 );
    }

    template<typename T>
    inline bool stringToBool( const T* str )
    {
        return stringToInt(str) != 0;
    }

    String intToString( int i );

    String uint64ToString( uint64 x );
    String uint64ToHexStr( uint64 x );

    inline String boolToString( bool b )
    {
        return intToString( b ? 1 : 0 );
    }

    String ptrToString( void* p );

    float  stringToFloat( const char* str );
    float  stringToFloat( const wchar_t* str );
    String floatToString( float v );

    void toLower( String& str );

    inline String toLowerCopy( const String& str )
    {
        String bk(str);
        toLower(bk);
        return bk;
    }

    template<class T>
    inline T getFileMainName( const T& fullname )
    {
        size_t pos = fullname.find_last_of("/\\");
        if ( pos != T::npos )
        {
            return fullname.substr(pos+1);
        }

        return T();
    }

    template<class T>
    inline T getFileTitleName( const T& fullname )
    {
        size_t pos = fullname.find_last_of(".");
        if ( pos != T::npos )
        {
            return fullname.substr(0, pos);
        }

        return fullname;
    }


    //////////////////////////////////////////////////////////////////////////
    inline StringVector makeStringVector( const String& n0 )
    {
        StringVector v;
        v.push_back( n0 );
        return v;
    }

    inline StringVector makeStringVector( const String& n0, const String& n1  )
    {
        StringVector v;
        v.push_back( n0 );
        v.push_back( n1 );
        return v;
    }

    inline StringVector makeStringVector( const String& n0, const String& n1, const String& n2 )
    {
        StringVector v;
        v.push_back( n0 );
        v.push_back( n1 );
        v.push_back( n2 );
        return v;
    }

    inline StringVector makeStringVector( const String& n0, const String& n1, const String& n2, const String& n3 )
    {
        StringVector v;
        v.push_back( n0 );
        v.push_back( n1 );
        v.push_back( n2 );
        v.push_back( n3 );
        return v;
    }

    inline StringVector makeStringVector( const String& n0, const String& n1, const String& n2, const String& n3, const String& n4 )
    {
        StringVector v;
        v.push_back( n0 );
        v.push_back( n1 );
        v.push_back( n2 );
        v.push_back( n3 );
        v.push_back( n4 );
        return v;
    }

    //////////////////////////////////////////////////////////////////////////
    // string helper class
    template<typename T>
    class TxtStreamReader_ : public AllocatedObject
    {
    public:
        TxtStreamReader_( const T* str ) : m_buff(0), m_str(str) {}

        TxtStreamReader_( const T* str, int len )
        {
            m_buff = KHAOS_MALLOC_ARRAY_T(T, len+1);
            memcpy( m_buff, str, len * sizeof(T) );
            m_buff[len] = 0;
            m_str = m_buff;
        }

        ~TxtStreamReader_()
        {
            if ( m_buff )
            {
                KHAOS_FREE(m_buff);
            }
        }

        bool getLine( T* buff )
        {
            if ( !m_str )
            {
                buff[0] = 0;
                return false;
            }

            // 找1个行
            int pos = findString<T>( m_str, '\n' );

            // 有换行
            if ( pos >= 0 )
            {
                std::copy( m_str, m_str + pos, buff );
                buff[pos] = 0;
                m_str += pos + 1;
            }
            else // 最后一行
            {
                copyString( buff, m_str );
                m_str = 0;

                if ( *buff == 0 )
                    return false;
            }

            return true;
        }

        template<class U>
        bool getLine( U& str )
        {
            T buff[4096] = {};
            bool ret = getLine(buff);
            str = buff;
            return ret;
        }

    private:
        T*       m_buff;
        const T* m_str;
    };

    typedef TxtStreamReader_<char>    TxtStreamReader;
    typedef TxtStreamReader_<wchar_t> WTxtStreamReader;
}

