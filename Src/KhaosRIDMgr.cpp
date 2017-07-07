#include "KhaosPreHeaders.h"
#include "KhaosRIDMgr.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void RIDObjectManager::_freeList( RIDObjectList& ridList )
    {
        for ( size_t i = 0; i < ridList.size(); ++i )
        {
            RIDObject* obj = ridList[i];
            KHAOS_DELETE obj;
        }

        ridList.clear();
    }

    void RIDObjectManager::_clear()
    {
        _freeList( m_ridObjs );
        _freeList( m_ridFrees );
    }

    RIDObject* RIDObjectManager::requestRIDObj( void* owner )
    {
        RIDObject* obj;

        if ( m_ridFrees.size() ) // �п���
        {
            obj = m_ridFrees.back();
            m_ridFrees.pop_back();
        }
        else // �´���
        {
            obj = KHAOS_NEW RIDObject;
        }

        khaosAssert( !obj->getOwner() );
        obj->setOwner( owner );
        obj->setRID( (int)m_ridObjs.size() ); // ��ʼridΪ���һλ������֤Ψһ
        m_ridObjs.push_back( obj );

        ++m_countUpdate; // ʵ�ʵ�����������update�����
        setDirty();
        return obj;
    }


    void RIDObjectManager::freeRIDObj( RIDObject* rid )
    {
        rid->setOwner( 0 ); // ������������
        // ����count�ޱ仯��update�󽫸���Ϊʵ������
        --m_countUpdate; // ʵ�ʵ�����������update�����
        setDirty();
    }

    inline bool _isZero( RIDObject* obj )
    {
        return !obj->getOwner();
    }

    void RIDObjectManager::update()
    {
        if ( !m_dirty )
            return;

        // ɾ��ownerΪ0��
        khaosAssert( (int)m_ridObjs.size() >= m_countUpdate );
        if ( m_ridObjs.size() != m_countUpdate )
        {
            m_ridTemp.clear();

            KHAOS_FOR_EACH( RIDObjectList, m_ridObjs, it )
            {
                RIDObject* obj = *it;
                if ( _isZero(obj) ) // Ϊ�����ͷ�
                    m_ridFrees.push_back( obj );
                else // ����������
                    m_ridTemp.push_back( obj );
            }

            khaosAssert( m_ridTemp.size() == m_countUpdate );
            m_ridObjs.swap( m_ridTemp );
            khaosAssert( m_ridObjs.size() == m_countUpdate );
        }

        // ����
        if ( m_ridObjs.size() )
            std::sort( m_ridObjs.begin(), m_ridObjs.end(), m_func );

        // ����id
        for ( size_t i = 0; i < m_ridObjs.size(); ++i )
        {
            m_ridObjs[i]->setRID( i );
        }

        m_dirty = false;
    }
}

