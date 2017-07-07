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

        if ( m_ridFrees.size() ) // 有空余
        {
            obj = m_ridFrees.back();
            m_ridFrees.pop_back();
        }
        else // 新创建
        {
            obj = KHAOS_NEW RIDObject;
        }

        khaosAssert( !obj->getOwner() );
        obj->setOwner( owner );
        obj->setRID( (int)m_ridObjs.size() ); // 初始rid为最后一位，并保证唯一
        m_ridObjs.push_back( obj );

        ++m_countUpdate; // 实际的数量，将在update后更新
        setDirty();
        return obj;
    }


    void RIDObjectManager::freeRIDObj( RIDObject* rid )
    {
        rid->setOwner( 0 ); // 仅仅作标记清空
        // 这里count无变化，update后将更新为实际数量
        --m_countUpdate; // 实际的数量，将在update后更新
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

        // 删除owner为0的
        khaosAssert( (int)m_ridObjs.size() >= m_countUpdate );
        if ( m_ridObjs.size() != m_countUpdate )
        {
            m_ridTemp.clear();

            KHAOS_FOR_EACH( RIDObjectList, m_ridObjs, it )
            {
                RIDObject* obj = *it;
                if ( _isZero(obj) ) // 为空则释放
                    m_ridFrees.push_back( obj );
                else // 存在则整理
                    m_ridTemp.push_back( obj );
            }

            khaosAssert( m_ridTemp.size() == m_countUpdate );
            m_ridObjs.swap( m_ridTemp );
            khaosAssert( m_ridObjs.size() == m_countUpdate );
        }

        // 排序
        if ( m_ridObjs.size() )
            std::sort( m_ridObjs.begin(), m_ridObjs.end(), m_func );

        // 分配id
        for ( size_t i = 0; i < m_ridObjs.size(); ++i )
        {
            m_ridObjs[i]->setRID( i );
        }

        m_dirty = false;
    }
}

