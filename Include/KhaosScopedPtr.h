#pragma once
#include "KhaosMemory.h"
#include "KhaosTypes.h"


namespace Khaos
{
    template<class T, int FREE_METHOD = SPFM_DELETE>
    class ScopedPtr
    {
    public:
        typedef T* Pointer;

    private:
        ScopedPtr( const ScopedPtr& ptr );

        ScopedPtr& operator=( const ScopedPtr& ptr );

    public:
        ScopedPtr() : m_ptr(0) {}

        explicit ScopedPtr( T* ptr ) : m_ptr(ptr) {} // 交换所有权，所以不用addRef

        ~ScopedPtr()
        {
            release();
        }

    public:
        operator bool() const { return m_ptr != 0; }

        bool operator!() const { return m_ptr == 0; }

        T& operator*() const
        {
            khaosAssert(m_ptr);
            return *m_ptr;
        }

        T* operator->() const
        {
            khaosAssert(m_ptr);
            return m_ptr;
        }

    public:
        T* get() const { return m_ptr; }

        void attach( T* ptr )
        {
            khaosAssert( !m_ptr );
            release(); // 只是为了安全，非设计如此
            m_ptr = ptr;
        }

        T* detach()
        {
            T* tmp = m_ptr;
            m_ptr = 0;
            return tmp;
        }

        void release()
        {
            if ( m_ptr )
            {
                _destroy( IntToType<FREE_METHOD>() );
                m_ptr = 0;
            }
        }

        void reset( T* ptr )
        {
            release();
            attach( ptr );
        }

    private:
        void _destroy( IntToType<SPFM_FREE> )
        {
            KHAOS_FREE(m_ptr);
        }

        void _destroy( IntToType<SPFM_DELETE_T> )
        {
            KHAOS_DELETE_T(m_ptr);
        }

        void _destroy( IntToType<SPFM_DELETE> )
        {
            KHAOS_DELETE m_ptr;
        }

    private:
        T* m_ptr;
    };

    template<class T, class U, int FREE_METHOD> 
    bool operator==( const ScopedPtr<T,FREE_METHOD>& lhs, const ScopedPtr<U,FREE_METHOD>& rhs )
    {
        return lhs.get() == rhs.get();
    }

    template<class T, class U, int FREE_METHOD>
    bool operator!=( const ScopedPtr<T,FREE_METHOD>& lhs, const ScopedPtr<U,FREE_METHOD>& rhs )
    {
        return lhs.get() != rhs.get();
    }

    template<class T, class U, int FREE_METHOD>
    bool operator<( const ScopedPtr<T,FREE_METHOD>& lhs, const ScopedPtr<U,FREE_METHOD>& rhs )
    {
        return lhs.get() < rhs.get();
    }
}

