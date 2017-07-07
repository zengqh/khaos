#pragma once
#include "KhaosVector3.h"
#include "KhaosQuaternion.h"
#include "KhaosMatrix4.h"
#include "KhaosRTTI.h"
#include "KhaosIterator.h"
#include "KhaosNodeDef.h"

namespace Khaos
{
    class Node : public AllocatedObject
    {
        KHAOS_DECLARE_RTTI(Node)

    public:
        enum TransformSpace
        {
            TS_LOCAL,
            TS_PARENT,
            TS_WORLD
        };

    public:
        typedef vector<Node*>::type     NodeList;
        typedef RangeIterator<NodeList> Iterator;

    public:
        Node();
        virtual ~Node() {}

        virtual void _onDestruct();
        void _destruct();

    public:
        int getType() const { return m_type; }

        void _setName( const String& name ) { m_name = name; }
        const String& getName() const { return m_name; }

        virtual void addChild( Node* node );
        virtual void removeChild( Node* node );
        virtual void removeAllChildren();

        Node*     getParent() const { return m_parent; }
        Iterator  getChildren() const { return Iterator(m_children); }
        NodeList& _getChildren() { return m_children; }

    public:
        void setPosition( const Vector3& pos );
        void setPosition( float x, float y, float z );

        void setOrientation( const Quaternion& ori );
        void setOrientation( const Vector3& axis, float angle );
        void resetOrientation();

        void setScale( const Vector3& sca );
        void setScale( float x, float y, float z );
        void setScale( float s );

        const Vector3&    getPosition()    const { return m_position; }
        const Quaternion& getOrientation() const { return m_orientation; }
        const Vector3&    getScale()       const { return m_scale; } 

    public:
        void translate( const Vector3& d, TransformSpace ts = TS_PARENT );
        void translate( float x, float y, float z, TransformSpace ts = TS_PARENT );

        void rotate( const Quaternion& q, TransformSpace ts = TS_LOCAL );
        void rotate( const Vector3& axis, float angle, TransformSpace ts = TS_LOCAL );

        void yaw( float angle, TransformSpace ts = TS_LOCAL );
        void pitch( float angle, TransformSpace ts = TS_LOCAL );
        void roll( float angle, TransformSpace ts = TS_LOCAL );

        void scale( const Vector3& sca );
        void scale( float x, float y, float z );
        void scale( float s );

    public:
        void setMatrix( const Matrix4& mat );
        void lookAt( const Vector3& eye, const Vector3& target, const Vector3& upDir );

    public:
        const Matrix4& getMatrix() const;
        const Matrix4& getDerivedMatrix() const;

    protected:
        // ���ڵ㱻��ӵ�����ͼ�л��Ƴ�������ͼ��ʱ��
        // ִ��������_onSelfAdd/_onSelfRemove
        void _updateDerivedByAdd();
        void _updateDerivedByRemove();

        // ���ڵ���߸��׽ڵ㱻��ӵ�����ͼ�л��Ƴ�������ͼ��ʱ��
        // ������������ÿ���ڵ�Ĵ���Ĭ��ִ�����ñ��ڵ㸸������
        virtual void _onSelfAdd();
        virtual void _onSelfRemove();

        // �����Լ����ǣ�ͬʱ֪ͨ����
        void _setDerivedSelfTransformDirty();
        // �����Լ��������ǣ�ͬʱ֪ͨ����
        void _setDerivedParentTransformDirty();

        // ���ýڵ���Լ���/��������
        virtual void _setSelfTransformDirty();
        virtual void _setParentTransformDirty();

        // �����Լ�
        virtual void _update();

        // �����±任
        virtual bool _checkUpdateTransform();

    public:
        // ��������
        void _updateDerived();

    protected:
        // ����
        String      m_name;

        // ��Ը��ڵ�λ�ã���λ������
        Vector3     m_position;
        Quaternion  m_orientation;
        Vector3     m_scale;

        // ��Ը��ڵ����/�ۼƾ���
        Matrix4     m_matrix;
        Matrix4     m_derivedMatrix;

        // ���ڵ�/���ӽڵ�
        Node*       m_parent;
        NodeList    m_children;

        // ���ͱ��
        NodeType    m_type;

        // �任��Ϣ��ʱ���Ƿ���Ҫ����
        BitSet32    m_dirtyFlag;

        // ����/��ֹ����
        BitSet32    m_featureFlag;
    };
}

