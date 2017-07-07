#include "KhaosPreHeaders.h"
#include "KhaosNode.h"


namespace Khaos
{
    Node::Node() : m_position(Vector3::ZERO), m_orientation(Quaternion::IDENTITY), m_scale(Vector3::UNIT_SCALE),
        m_parent(0), m_type(NT_NODE)
    {
        m_dirtyFlag.setFlag( DF_SELF_TRANSFORM | DF_PARENT_TRANSFORM );
        m_featureFlag.setFlag( TS_ENABLE_POS | TS_ENABLE_ROT | TS_ENABLE_SCALE );
    }

    void Node::_onDestruct()
    {
        // 自己死了和父亲与孩子都脱离关系
        removeAllChildren();

        if ( m_parent )
            m_parent->removeChild( this );
    }

    void Node::_destruct()
    {
        _onDestruct();
        KHAOS_DELETE this;
    }

    void Node::addChild( Node* node )
    {
        khaosAssert( node->m_parent == 0 );
        khaosAssert( std::find(m_children.begin(), m_children.end(), node) == m_children.end() );
        node->m_parent = this;
        m_children.push_back( node );
        node->_updateDerivedByAdd();
    }
    
    void Node::removeChild( Node* node )
    {
        khaosAssert( node->m_parent == this );
        NodeList::iterator it = std::find(m_children.begin(), m_children.end(), node);
        khaosAssert( it != m_children.end() );
        node->m_parent = 0;
        m_children.erase( it );
        node->_updateDerivedByRemove();
    }

    void Node::removeAllChildren()
    {
        for ( NodeList::iterator it = m_children.begin(), ite = m_children.end(); it != ite; ++it )
        {
            Node* node = *it;
            node->m_parent = 0;
            node->_updateDerivedByRemove();
        }

        m_children.clear();
    }

    void Node::_updateDerivedByAdd()
    {
        // 先自己做
        _onSelfAdd();

        // 再孩子做
        for ( NodeList::iterator it = m_children.begin(), ite = m_children.end(); it != ite; ++it )
        {
            (*it)->_updateDerivedByAdd();
        }
    }

    void Node::_updateDerivedByRemove()
    {
        // 先孩子做
        for ( NodeList::iterator it = m_children.begin(), ite = m_children.end(); it != ite; ++it )
        {
            (*it)->_updateDerivedByRemove();
        }

        // 再自己做
        _onSelfRemove();
    }

    void Node::_onSelfAdd()
    {
        // 设置自己父亲脏
        _setParentTransformDirty();
    }

    void Node::_onSelfRemove()
    {
        // 设置自己父亲脏
        _setParentTransformDirty();
    }

    void Node::_setSelfTransformDirty()
    {
        m_dirtyFlag.setFlag( DF_SELF_TRANSFORM );
    }

    void Node::_setParentTransformDirty()
    {
        m_dirtyFlag.setFlag( DF_PARENT_TRANSFORM );
    }

    void Node::_setDerivedSelfTransformDirty()
    {
        // 设置自己脏标记
        _setSelfTransformDirty();

        // 同步通知孩子，告之我脏了
        for ( NodeList::iterator it = m_children.begin(), ite = m_children.end(); it != ite; ++it )
        {
            (*it)->_setDerivedParentTransformDirty();
        }
    }

    void Node::_setDerivedParentTransformDirty()
    {
        // 孩子收到父脏后，设置标记，并告之孩子的孩子
        _setParentTransformDirty();

        for ( NodeList::iterator it = m_children.begin(), ite = m_children.end(); it != ite; ++it )
        {
            (*it)->_setDerivedParentTransformDirty();
        }
    }

    void Node::setPosition( const Vector3& pos )
    {
        if ( m_featureFlag.testFlag(TS_ENABLE_POS) )
        {
            m_position = pos;
            _setDerivedSelfTransformDirty();
        }
    }

    void Node::setPosition( float x, float y, float z )
    {
        setPosition( Vector3(x, y, z) );
    }

    void Node::setOrientation( const Quaternion& ori )
    {
        if ( m_featureFlag.testFlag(TS_ENABLE_ROT) )
        {
            m_orientation = ori;
            m_orientation.normalise();
            _setDerivedSelfTransformDirty();
        }
    }

    void Node::setOrientation( const Vector3& axis, float angle )
    {
        setOrientation( Quaternion(angle, axis) );
    }

    void Node::resetOrientation()
    {
        setOrientation( Quaternion::IDENTITY );
    }

    void Node::setScale( const Vector3& sca )
    {
        if ( m_featureFlag.testFlag(TS_ENABLE_SCALE) )
        {
            m_scale = sca;
            _setDerivedSelfTransformDirty();
        }
    }

    void Node::setScale( float x, float y, float z )
    {
        setScale( Vector3(x, y, z) );
    }

    void Node::setScale( float s )
    {
        setScale( Vector3(s, s, s) );
    }

    void Node::translate( const Vector3& d, TransformSpace ts )
    {
        if ( m_featureFlag.testFlag(TS_ENABLE_POS) )
        {
            // 平移
            switch (ts)
            {
            case TS_LOCAL: // 本地空间平移
                // 把平移向量d从本地空间转换到父空间(m_position在父空间定义)
                // R,S无变化
                m_position += m_orientation * d;
                break;

            case TS_PARENT: // 父空间平移
                // 就是在父空间直接加
                // R,S无变化
                m_position += d;
                break;

            case TS_WORLD: // 世界空间平移
                // 把平移向量d从世界空间转换到父空间(m_position在父空间定义)
                // R,S无变化
                if ( m_parent )
                    m_position += m_parent->getDerivedMatrix().inverse().transformAffine(d);
                else
                    m_position += d;
                break;
            }

            _setDerivedSelfTransformDirty();
        }
    }

    void Node::translate( float x, float y, float z, TransformSpace ts )
    {
        return translate( Vector3(x, y, z), ts );
    }

    void Node::rotate( const Quaternion& q1, TransformSpace ts )
    {
        if ( m_featureFlag.testFlag(TS_ENABLE_ROT) )
        {
            Quaternion q = q1;
            q.normalise();

            // 旋转
            switch ( ts )
            {
            case TS_LOCAL: // 本地空间旋转
                // 新的父空间R = R * q * inv(R) * R = R * q
                // T * R * S => T * (R * q) * S
                // T,S无变化
                m_orientation = m_orientation * q;
                break;

            case TS_PARENT: // 父空间旋转
                // T * R * S => T * (q * R) * S
                // T,S无变化
                m_orientation = q * m_orientation;
                break;

            case TS_WORLD: // 世界空间旋转
                // 新的父空间R = R * inv(Rp-w * R) * q * (Rp-w * R) = R * inv(Rw) * q * Rw
                // m_Orientation = m_orientation * _getDerivedOrientation().inverse() * q * _getDerivedOrientation(); 
                // 暂时不支持了，要额外算累计的旋转量。或者从矩阵提取，计算量大
                khaosAssert(0);
                break;
            }

            _setDerivedSelfTransformDirty();
        }
    }

    void Node::rotate( const Vector3& axis, float angle, TransformSpace ts )
    {
        rotate( Quaternion(angle, axis), ts );
    }

    void Node::yaw( float angle, TransformSpace ts )
    {
        rotate( Vector3::UNIT_Y, angle, ts );
    }

    void Node::pitch( float angle, TransformSpace ts )
    {
        rotate( Vector3::UNIT_X, angle, ts );
    }

    void Node::roll( float angle, TransformSpace ts )
    {
        rotate( Vector3::UNIT_Z, angle, ts );
    }

    void Node::scale( const Vector3& sca )
    {
        if ( m_featureFlag.testFlag(TS_ENABLE_SCALE) )
        {
            m_scale *= sca;
            _setDerivedSelfTransformDirty();
        }
    }

    void Node::scale( float x, float y, float z )
    {
        scale( Vector3(x, y, z) );
    }

    void Node::scale( float s )
    {
        scale( Vector3(s, s, s) );
    }

    void Node::setMatrix( const Matrix4& mat )
    {
        // 分解
        Vector3 pos;
        Vector3 sca;
        Quaternion rot;
        mat.decomposition( pos, sca, rot );

        // 赋值
        setPosition( pos );
        setScale( sca );
        setOrientation( rot );
    }

    void Node::lookAt( const Vector3& eye, const Vector3& target, const Vector3& upDir )
    {
        // look at
        Matrix4 mat;
        Math::makeTransformLookAt( mat, eye, target, upDir );
        setMatrix( mat );
    }

    const Matrix4& Node::getMatrix() const
    {
        const_cast<Node*>(this)->_checkUpdateTransform();
        return m_matrix;
    }

    const Matrix4& Node::getDerivedMatrix() const
    {
        const_cast<Node*>(this)->_checkUpdateTransform();
        return m_derivedMatrix;
    }

    void Node::_updateDerived()
    {
        // 从根节点开始向下更新
        _update();

        for ( NodeList::iterator it = m_children.begin(), ite = m_children.end(); it != ite; ++it )
        {
            (*it)->_updateDerived();
        }
    }

    void Node::_update()
    {
        _checkUpdateTransform();
    }

    bool Node::_checkUpdateTransform()
    {
        bool needUpdateDerivedTransform = false;

        // 自己变换更新
        if ( m_dirtyFlag.testFlag(DF_SELF_TRANSFORM) )
        {
            m_matrix.makeTransform( m_position, m_scale, m_orientation );
            needUpdateDerivedTransform = true;
            m_dirtyFlag.unsetFlag(DF_SELF_TRANSFORM);
        }

        // 父亲变换有更新
        if ( m_dirtyFlag.testFlag(DF_PARENT_TRANSFORM) )
        {
            needUpdateDerivedTransform = true;
            m_dirtyFlag.unsetFlag(DF_PARENT_TRANSFORM);
        }

        // 需要更新累计矩阵
        if ( needUpdateDerivedTransform )
        {
            if ( m_parent )
            {
                m_derivedMatrix = m_parent->getDerivedMatrix() * m_matrix;
            }
            else
            {
                m_derivedMatrix = m_matrix;
            }
        }

        return needUpdateDerivedTransform;
    }
}

