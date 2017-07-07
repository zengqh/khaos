#pragma once
#include "KhaosStdHeaders.h"
#include "KhaosMemory.h"

namespace Khaos
{ 
    // 整数类型
    typedef signed char int8;
    typedef short       int16;
    typedef int         int32;
    typedef long long   int64;

    typedef unsigned char       uint8;
    typedef unsigned short      uint16;
    typedef unsigned int        uint32;
    typedef unsigned long long  uint64;

    typedef unsigned char   uchar;
    typedef unsigned short  ushort;
    typedef unsigned int    uint;
    typedef unsigned long   ulong;

    // 字符
    typedef char*       pstr;
    typedef const char* pcstr;
}

// stl标准容器
namespace Khaos
{
    template <class T> 
    struct vector 
    { 
#if KHAOS_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::vector<T, STLAllocator<T> > type;
#else
        typedef typename std::vector<T> type;
#endif
    };

    template <class T> 
    struct list 
    { 
#if KHAOS_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::list<T, STLAllocator<T> > type;
#else
        typedef typename std::list<T> type;
#endif
    }; 

    template <class K, class V, class P = std::less<K> > 
    struct map
    { 
#if KHAOS_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::map<K, V, P, STLAllocator<std::pair<const K, V> > > type;
#else
        typedef typename std::map<K, V, P> type;
#endif
    };

    template <class T, class P = std::less<T> > 
    struct set 
    { 
#if KHAOS_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::set<T, P, STLAllocator<T> > type;
#else
        typedef typename std::set<T, P> type;
#endif
    }; 

    template <class K, class V, class P = std::less<K> > 
    struct multimap 
    { 
#if KHAOS_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::multimap<K, V, P, STLAllocator<std::pair<const K, V> > > type;
#else
        typedef typename std::multimap<K, V, P> type;
#endif
    };

    template <class T, class P = std::less<T> > 
    struct multiset 
    { 
#if KHAOS_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::multiset<T, P, STLAllocator<T> > type;
#else
        typedef typename std::multiset<T, P> type;
#endif
    }; 

    template<class K, class V, class H = hash_<K>, class E = std::equal_to<K> >
    struct unordered_map
    {
#if KHAOS_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename unordered_map_<K, V, H, E, STLAllocator<std::pair<const K, V> > > type;
#else
        typedef typename unordered_map_<K, V, H, E> type;
#endif
    };

    template<class K, class H = hash_<K>, class E = std::equal_to<K> >
	struct unordered_set
    {
#if KHAOS_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename unordered_set_<K, H, E, STLAllocator<K> > type;
#else
        typedef typename unordered_set_<K, H, E> type;
#endif
    };
}

// 字符串
namespace Khaos
{
#if KHAOS_STRING_USE_CUSTOM_MEMORY_ALLOCATOR
    typedef std::basic_string<char, std::char_traits<char>, STLAllocator<char> >            String;
    typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, STLAllocator<wchar_t> >   WString;
#else
    typedef std::string     String;
    typedef std::wstring    WString;
#endif

#if KHAOS_UNICODE
    typedef String  TString;
#else
    typedef WString TString;
#endif
}

#if KHAOS_STRING_USE_CUSTOM_MEMORY_ALLOCATOR
// 特化自定义String类的hash版本
namespace std
{
    template<>
    class tr1::hash<Khaos::String>
        : public unary_function<Khaos::String, size_t>
    {	// hash functor
    public:
        typedef Khaos::String _Kty;

        size_t operator()(const _Kty& _Keyval) const
        {	// hash _Keyval to size_t value by pseudorandomizing transform
            size_t _Val = 2166136261U;
            size_t _First = 0;
            size_t _Last = _Keyval.size();
            size_t _Stride = 1 + _Last / 10;

            for(; _First < _Last; _First += _Stride)
                _Val = 16777619U * _Val ^ (size_t)_Keyval[_First];
            return (_Val);
        }
    };

    template<>
    class tr1::hash<Khaos::WString>
        : public unary_function<Khaos::WString, size_t>
    {	// hash functor
    public:
        typedef Khaos::WString _Kty;

        size_t operator()(const _Kty& _Keyval) const
        {	// hash _Keyval to size_t value by pseudorandomizing transform
            size_t _Val = 2166136261U;
            size_t _First = 0;
            size_t _Last = _Keyval.size();
            size_t _Stride = 1 + _Last / 10;

            for(; _First < _Last; _First += _Stride)
                _Val = 16777619U * _Val ^ (size_t)_Keyval[_First];
            return (_Val);
        }
    };

    namespace tr1 
    {
        //using ::std::tr1::hash;
    }	// namespace tr1
}
#endif

