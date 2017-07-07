#pragma once
#include "KhaosStdTypes.h"
#include "KhaosTypes.h"

namespace Khaos
{
    template<class T, int FREE_METHOD = SPFM_DELETE>
    class ObjectRecycle : public AllocatedObject
    {
    public:
        typedef typename vector<T*>::type ObjList;

    public:
        ~ObjectRecycle()
        {
            clearPool();
        }

        void clearPool()
        {
            for ( typename ObjList::iterator it = m_unusedList.begin(), ite = m_unusedList.end(); it != ite; ++it )
            {
                T* obj = *it;
                _destroy( obj, IntToType<FREE_METHOD>() );
            }

            m_unusedList.clear();
        }

        T* requestObject()
        {
            if ( m_unusedList.size() )
            {
                T* obj = m_unusedList.back();
                m_unusedList.resize( m_unusedList.size() - 1 );
                return obj;
            }
            else
            {
                T* obj = _create( IntToType<FREE_METHOD>() );
                return obj;
            }
        }

        void freeObject( T* obj )
        {
            m_unusedList.push_back( obj );
        }

    private:
        T* _create( IntToType<SPFM_FREE> )
        {
            return KHAOS_MALLOC_T(T);
        }

        void _destroy( T* obj, IntToType<SPFM_FREE> )
        {
            KHAOS_FREE(obj);
        }

        T* _create( IntToType<SPFM_DELETE_T> )
        {
            return KHAOS_NEW_T(T)();
        }

        void _destroy( T* obj, IntToType<SPFM_DELETE_T> )
        {
            KHAOS_DELETE_T(obj);
        }

        T* _create( IntToType<SPFM_DELETE> )
        {
            return KHAOS_NEW T;
        }

        void _destroy( T* obj, IntToType<SPFM_DELETE> )
        {
            KHAOS_DELETE obj;
        }

    private:
        ObjList m_unusedList;
    };
}

