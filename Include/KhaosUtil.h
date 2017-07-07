#pragma once
#include "KhaosStdHeaders.h"

namespace Khaos
{

#define KHAOS_PP_STRINGIZING(x)         #x
#define KHAOS_PP_STRINGIZING_MACRO(x)   KHAOS_PP_STRINGIZING(x)

#define KHAOS_PP_CAT_(x, y)             x##y
#define KHAOS_PP_CAT(x, y)              KHAOS_PP_CAT_(x, y)

#define KHAOS_MEMBER_OFFSET(t, x)   ((int) &((t*)0)->x)
#define KHAOS_ARRAY_SIZE(v)         (sizeof(v) / sizeof((v)[0]))

#define KHAOS_CLEAR_ZERO(x, s)  memset( x, 0, s )
#define KHAOS_CLEAR_ARRAY(x)    KHAOS_CLEAR_ZERO( x, sizeof(x) )

#define KHAOS_SAFE_DELETE(x)        { if (x) { KHAOS_DELETE (x);   (x) = 0; } }
#define KHAOS_SAFE_RELEASE(x)       { if (x) { (x)->release();     (x) = 0; } }

#define KHAOS_FOR_EACH(container, var, it) \
    for ( container::iterator it = (var).begin(), it##_e = (var).end(); it != it##_e; ++it )

#define KHAOS_FOR_EACH_CONST(container, var, it) \
    for ( container::const_iterator it = (var).begin(), it##_e = (var).end(); it != it##_e; ++it )

#define KHAOS_DEFINE_PROPERTY_BY_VAL(type, name) \
    public: \
        void set##name( type v ) { m_##name = v; } \
        type get##name() const { return m_##name; } \
    private:\
        type m_##name;

#define KHAOS_DEFINE_PROPERTY_BY_REF(type, name) \
    public: \
        void set##name( const type& v ) { m_##name = v; } \
        const type& get##name() const { return m_##name; } \
    private:\
        type m_##name;

    template<class T>
    void swapVal( T& a, T& b )
    {
        T t = a;
        a = b;
        b = t;
    }
}

