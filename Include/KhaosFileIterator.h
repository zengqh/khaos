#pragma once
#include "KhaosStdTypes.h"
#include "KhaosDebug.h"
#include <io.h>

namespace Khaos
{
    class file_iterator
    {
    public:
        file_iterator() : handle_(-1) {}

        file_iterator( const String& path )
        {
            _init( path );
        }

        ~file_iterator()
        {
            if ( handle_ != -1 )
                _findclose( handle_ );
        }

    public:
        bool next()
        {
            if ( handle_ != -1 )
            {
                if ( _findnext( handle_, &data_ ) == 0 )
                {
                    return _skipSpecPath();
                }

                _findclose( handle_ );
                handle_ = -1;
            }

            return false;
        }

    public:
        bool is_valid() const
        {
            return handle_ != -1;
        }

        bool is_file() const
        {
            return (data_.attrib & _A_SUBDIR) == 0;
        }

        pcstr file_name() const
        {
            return data_.name;
        }

    public:
        operator bool() const
        {
            return is_valid();
        }

        bool operator++()
        {
            return next();
        }

        bool operator++(int)
        {
            return next();
        }

        bool operator==( const file_iterator& rhs ) const
        {
            return handle_ == rhs.handle_;
        }

        bool operator!=( const file_iterator& rhs ) const
        {
            return handle_ != rhs.handle_;
        }

    private:
        bool _skipSpecPath()
        {
            if ( data_.attrib & _A_SUBDIR )
            {
                if ( strcmp(data_.name, ".") == 0 || strcmp(data_.name, "..") == 0 )
                    return next(); // Ìø¹ý.ºÍ..Ä¿Â¼
            }

            return true;
        }

        void _init( const String& path )
        {
            handle_ = _findfirst( (path + "/*.*").c_str(), &data_ );
        
            if ( handle_ != -1 )
            {
                _skipSpecPath();
            }
        }

    private:
        intptr_t    handle_;
        _finddata_t data_;
    };

    template<class Func>
    bool travel_path_impl_( const String& base_path, const String& rel_path, Func func, bool recursion )
    {
        file_iterator it( base_path + rel_path );

        while ( it )
        {
            if ( it.is_file() || !recursion )
            {
                if ( !func( base_path, rel_path, it ) )
                    return false;
            }
            else
            {
                String new_rel_path = rel_path + it.file_name() + "/";
                if ( !travel_path_impl_( base_path, new_rel_path, func, recursion ) )
                    return false;
            }

            ++it;
        }

        return true;
    }

    template<class Func>
    inline bool travel_path( const String& base_path, Func func, bool recursion )
    {
        return travel_path_impl_( base_path, String(), func, recursion );
    }
}

