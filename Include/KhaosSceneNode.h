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
        // 所属场景图
        void _setSceneGraph( SceneGraph* sg ) { m_sceneGraph = sg; }
        SceneGraph* getSceneGraph() const { return m_sceneGraph; }

        // 所属剔除树的节点
        void _setOctreeNode( OctreeNode* octNode );
        OctreeNode* _getOctreeNode() const { return m_octNode; }

        // 删除之前的准备工作
        virtual void _onDestroyBefore() {}

    public:
        void setEnabled( bool en ) { m_featureFlag.enableFlag(SNODE_ENABLE, en); }
        bool isEnabled() const { return m_featureFlag.testFlag(SNODE_ENABLE); }

    protected:
        // 是否有私有区域
        virtual AreaOctree* _getPrivateArea() const { return 0; }

    protected:
        // 来自Node
        virtual void _onSelfAdd();
        virtual void _onSelfRemove();
        
        virtual void _setSelfTransformDirty();
        virtual void _setParentTransformDirty();

        virtual void _update();

        // 来自ISceneObjectListener
        virtual void onObjectAABBDirty();

    protected:
        virtual void _setWorldAABBDirty();
        void _setOctreePosDirty();

        void _checkUpdateWorldAABB();
        void _checkUpdateOctreePos();

        virtual void _makeWorldAABB() {}

    protected:
        // 所属场景图
        SceneGraph* m_sceneGraph;

        // 自己所处的区域
        AreaOctree* m_area;

        // 挂载的八叉树节点
        OctreeNode* m_octNode;

        // 此节点自己的世界aabb
        AxisAlignedBox m_worldAABB;
    };

    class RenderableSceneNode : public SceneNode
    {
        KHAOS_DECLARE_RTTI(RenderableSceneNode)

    public:
        RenderableSceneNode();
        virtual ~RenderableSceneNode();

    public:
        // 加入到渲染器
        virtual void _addToRenderer() {}

        // 获取该节点灯光信息
        LightsInfo* getLightInfo();

        // 阴影
        void setCastShadow( bool en ) { m_featureFlag.enableFlag(SHDW_ENABLE_CAST, en); }
        bool isCastShadow() const { return m_featureFlag.testFlag(SHDW_ENABLE_CAST); }

        void setReceiveShadow( bool en ) { m_featureFlag.enableFlag(SHDW_ENABLE_RECE, en); }
        bool isReceiveShadow() const { return m_featureFlag.testFlag(SHDW_ENABLE_RECE); }

        // 共享信息
        RenderableSharedData& getRenderSharedData() { return m_rdSharedData; }

        void setLightMapID( int id ) { m_rdSharedData.setLightMapID(id); }
        int  getLightMapID() const { return m_rdSharedData.getLightMapID(); }

        void setShadowMapID( int id ) { m_rdSharedData.setShadowMapID(id); }
        int  getShadowMapID() const { return m_rdSharedData.getShadowMapID(); }

        void setEnvProbeID( int id ) { m_rdSharedData.setEnvProbeID(id); }
        int  getEnvProbeID() const { return m_rdSharedData.getEnvProbeID(); }

    protected:
        // 节点接受的灯光信息
        LightsInfo*   m_lightsInfo;
        uint32        m_lightsInfoUpdate;

        // 渲染共享信息
        RenderableSharedData m_rdSharedData;
    };
}

