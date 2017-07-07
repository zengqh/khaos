#pragma once

namespace Khaos
{
    template<class T>
    class ResPtr
    {
    public:
        typedef T* Pointer;

    public:
        ResPtr() : m_ptr(0) {}

        ResPtr( const ResPtr& ptr ) : m_ptr(ptr.m_ptr)
        {
            if ( m_ptr )
                m_ptr->addRef();
        }

        template<class U>
        ResPtr( const ResPtr<U>& ptr ) : m_ptr(ptr.m_ptr)
        {
            if ( m_ptr )
                m_ptr->addRef();
        }

        explicit ResPtr( T* ptr ) : m_ptr(ptr) {} // 交换所有权，所以不用addRef

        ~ResPtr()
        {
            if ( m_ptr )
                m_ptr->release();
        }

    public:
        ResPtr& operator=( const ResPtr& ptr )
        {
            if ( m_ptr == ptr.m_ptr )
                return *this;

            ResPtr tmp(ptr);
            tmp.swap( *this );
            return *this;
        }

        template<class U>
        ResPtr& operator=( const ResPtr<U>& ptr )
        {
            if ( m_ptr == ptr.get() )
                return *this;

            ResPtr tmp(ptr);
            tmp.swap( *this );
            return *this;
        }

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

        void swap( ResPtr& ptr )
        {
            std::swap( m_ptr, ptr.m_ptr );
        }

        void attach( T* ptr )
        {
            khaosAssert( !m_ptr );
            if ( m_ptr ) // 这个判断只是为了安全，非设计如此
                m_ptr->release();
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
                m_ptr->release();
                m_ptr = 0;
            }
        }

        void reset( T* ptr )
        {
            release();
            attach( ptr );
        }

    private:
        T* m_ptr;
    };

    template<class T, class U> 
    bool operator==( const ResPtr<T>& lhs, const ResPtr<U>& rhs )
    {
        return lhs.get() == rhs.get();
    }

    template<class T, class U>
    bool operator!=( const ResPtr<T>& lhs, const ResPtr<U>& rhs )
    {
        return lhs.get() != rhs.get();
    }

    template<class T, class U>
    bool operator<( const ResPtr<T>& lhs, const ResPtr<U>& rhs )
    {
        return lhs.get() < rhs.get();
    }

    template<class T, class U>
    ResPtr<T> staticCast( ResPtr<U>& ptr )
    {
        if ( ptr.get() )
            ptr.get()->addRef();
        return ResPtr<T>( static_cast<typename ResPtr<T>::Pointer>(ptr.get()) );
    }
}

