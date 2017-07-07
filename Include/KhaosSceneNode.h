#pragma once
#include "KhaosNode.h"
#include "KhaosSceneObject.h"
#include "KhaosAxisAlignedBox.h"
#include "KhaosRenderableSharedData.h"

namespace Khaos
{
    class SceneGraph;
    class OctreeNode;
    class AreaOctree;
    class LightsInfo;

    class SceneNode : public Node, public ISceneObjectListener
    {
         KHAOS_DECLARE_RTTI(SceneNode)

    public:
        SceneNode();

    public:
        virtual void addChild( Node* node );

    public:
        const AxisAlignedBox& getWorldAABB() const;

    public:
        // ��������ͼ
        void _setSceneGraph( SceneGraph* sg ) { m_sceneGraph = sg; }
        SceneGraph* getSceneGraph() const { return m_sceneGraph; }

        // �����޳����Ľڵ�
        void _setOctreeNode( OctreeNode* octNode );
        OctreeNode* _getOctreeNode() const { return m_octNode; }

        // ɾ��֮ǰ��׼������
        virtual void _onDestroyBefore() {}

    public:
        void setEnabled( bool en ) { m_featureFlag.enableFlag(SNODE_ENABLE, en); }
        bool isEnabled() const { return m_featureFlag.testFlag(SNODE_ENABLE); }

    protected:
        // �Ƿ���˽������
        virtual AreaOctree* _getPrivateArea() const { return 0; }

    protected:
        // ����Node
        virtual void _onSelfAdd();
        virtual void _onSelfRemove();
        
        virtual void _setSelfTransformDirty();
        virtual void _setParentTransformDirty();

        virtual void _update();

        // ����ISceneObjectListener
        virtual void onObjectAABBDirty();

    protected:
        virtual void _setWorldAABBDirty();
        void _setOctreePosDirty();

        void _checkUpdateWorldAABB();
        void _checkUpdateOctreePos();

        virtual void _makeWorldAABB() {}

    protected:
        // ��������ͼ
        SceneGraph* m_sceneGraph;

        // �Լ�����������
        AreaOctree* m_area;

        // ���صİ˲����ڵ�
        OctreeNode* m_octNode;

        // �˽ڵ��Լ�������aabb
        AxisAlignedBox m_worldAABB;
    };

    class RenderableSceneNode : public SceneNode
    {
        KHAOS_DECLARE_RTTI(RenderableSceneNode)

    public:
        RenderableSceneNode();
        virtual ~RenderableSceneNode();

    public:
        // ���뵽��Ⱦ��
        virtual void _addToRenderer() {}

        // ��ȡ�ýڵ�ƹ���Ϣ
        LightsInfo* getLightInfo();

        // ��Ӱ
        void setCastShadow( bool en ) { m_featureFlag.enableFlag(SHDW_ENABLE_CAST, en); }
        bool isCastShadow() const { return m_featureFlag.testFlag(SHDW_ENABLE_CAST); }

        void setReceiveShadow( bool en ) { m_featureFlag.enableFlag(SHDW_ENABLE_RECE, en); }
        bool isReceiveShadow() const { return m_featureFlag.testFlag(SHDW_ENABLE_RECE); }

        // ������Ϣ
        RenderableSharedData& getRenderSharedData() { return m_rdSharedData; }

        void setLightMapID( int id ) { m_rdSharedData.setLightMapID(id); }
        int  getLightMapID() const { return m_rdSharedData.getLightMapID(); }

        void setShadowMapID( int id ) { m_rdSharedData.setShadowMapID(id); }
        int  getShadowMapID() const { return m_rdSharedData.getShadowMapID(); }

        void setEnvProbeID( int id ) { m_rdSharedData.setEnvProbeID(id); }
        int  getEnvProbeID() const { return m_rdSharedData.getEnvProbeID(); }

    protected:
        // �ڵ���ܵĵƹ���Ϣ
        LightsInfo*   m_lightsInfo;
        uint32        m_lightsInfoUpdate;

        // ��Ⱦ������Ϣ
        RenderableSharedData m_rdSharedData;
    };
}

