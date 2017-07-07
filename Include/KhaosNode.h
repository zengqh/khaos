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
        // 当节点被添加到场景图中或移除出场景图的时候
        // 执行子树的_onSelfAdd/_onSelfRemove
        void _updateDerivedByAdd();
        void _updateDerivedByRemove();

        // 当节点或者父亲节点被添加到场景图中或移除出场景图的时候
        // 可以在这里做每个节点的处理，默认执行设置本节点父亲脏标记
        virtual void _onSelfAdd();
        virtual void _onSelfRemove();

        // 设置自己脏标记，同时通知子孙
        void _setDerivedSelfTransformDirty();
        // 设置自己父亲脏标记，同时通知子孙
        void _setDerivedParentTransformDirty();

        // 设置节点的自己脏/父亲脏标记
        virtual void _setSelfTransformDirty();
        virtual void _setParentTransformDirty();

        // 更新自己
        virtual void _update();

        // 检查更新变换
        virtual bool _checkUpdateTransform();

    public:
        // 更新子树
        void _updateDerived();

    protected:
        // 名字
        String      m_name;

        // 相对父节点位置，方位，缩放
        Vector3     m_position;
        Quaternion  m_orientation;
        Vector3     m_scale;

        // 相对父节点矩阵/累计矩阵
        Matrix4     m_matrix;
        Matrix4     m_derivedMatrix;

        // 父节点/孩子节点
        Node*       m_parent;
        NodeList    m_children;

        // 类型标记
        NodeType    m_type;

        // 变换信息过时，是否需要更新
        BitSet32    m_dirtyFlag;

        // 允许/禁止特性
        BitSet32    m_featureFlag;
    };
}

