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
        // �Լ����˺͸����뺢�Ӷ������ϵ
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
        // ���Լ���
        _onSelfAdd();

        // �ٺ�����
        for ( NodeList::iterator it = m_children.begin(), ite = m_children.end(); it != ite; ++it )
        {
            (*it)->_updateDerivedByAdd();
        }
    }

    void Node::_updateDerivedByRemove()
    {
        // �Ⱥ�����
        for ( NodeList::iterator it = m_children.begin(), ite = m_children.end(); it != ite; ++it )
        {
            (*it)->_updateDerivedByRemove();
        }

        // ���Լ���
        _onSelfRemove();
    }

    void Node::_onSelfAdd()
    {
        // �����Լ�������
        _setParentTransformDirty();
    }

    void Node::_onSelfRemove()
    {
        // �����Լ�������
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
        // �����Լ�����
        _setSelfTransformDirty();

        // ͬ��֪ͨ���ӣ���֮������
        for ( NodeList::iterator it = m_children.begin(), ite = m_children.end(); it != ite; ++it )
        {
            (*it)->_setDerivedParentTransformDirty();
        }
    }

    void Node::_setDerivedParentTransformDirty()
    {
        // �����յ���������ñ�ǣ�����֮���ӵĺ���
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
            // ƽ��
            switch (ts)
            {
            case TS_LOCAL: // ���ؿռ�ƽ��
                // ��ƽ������d�ӱ��ؿռ�ת�������ռ�(m_position�ڸ��ռ䶨��)
                // R,S�ޱ仯
                m_position += m_orientation * d;
                break;

            case TS_PARENT: // ���ռ�ƽ��
                // �����ڸ��ռ�ֱ�Ӽ�
                // R,S�ޱ仯
                m_position += d;
                break;

            case TS_WORLD: // ����ռ�ƽ��
                // ��ƽ������d������ռ�ת�������ռ�(m_position�ڸ��ռ䶨��)
                // R,S�ޱ仯
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

            // ��ת
            switch ( ts )
            {
            case TS_LOCAL: // ���ؿռ���ת
                // �µĸ��ռ�R = R * q * inv(R) * R = R * q
                // T * R * S => T * (R * q) * S
                // T,S�ޱ仯
                m_orientation = m_orientation * q;
                break;

            case TS_PARENT: // ���ռ���ת
                // T * R * S => T * (q * R) * S
                // T,S�ޱ仯
                m_orientation = q * m_orientation;
                break;

            case TS_WORLD: // ����ռ���ת
                // �µĸ��ռ�R = R * inv(Rp-w * R) * q * (Rp-w * R) = R * inv(Rw) * q * Rw
                // m_Orientation = m_orientation * _getDerivedOrientation().inverse() * q * _getDerivedOrientation(); 
                // ��ʱ��֧���ˣ�Ҫ�������ۼƵ���ת�������ߴӾ�����ȡ����������
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
        // �ֽ�
        Vector3 pos;
        Vector3 sca;
        Quaternion rot;
        mat.decomposition( pos, sca, rot );

        // ��ֵ
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
        // �Ӹ��ڵ㿪ʼ���¸���
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

        // �Լ��任����
        if ( m_dirtyFlag.testFlag(DF_SELF_TRANSFORM) )
        {
            m_matrix.makeTransform( m_position, m_scale, m_orientation );
            needUpdateDerivedTransform = true;
            m_dirtyFlag.unsetFlag(DF_SELF_TRANSFORM);
        }

        // ���ױ任�и���
        if ( m_dirtyFlag.testFlag(DF_PARENT_TRANSFORM) )
        {
            needUpdateDerivedTransform = true;
            m_dirtyFlag.unsetFlag(DF_PARENT_TRANSFORM);
        }

        // ��Ҫ�����ۼƾ���
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

