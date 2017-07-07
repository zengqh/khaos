#pragma once
#include "KhaosStdTypes.h"
#include "KhaosRTTI.h"
#include "KhaosSceneNode.h"
#include "KhaosAreaNode.h"
#include "KhaosMeshNode.h"
#include "KhaosLightNode.h"
#include "KhaosCameraNode.h"
#include "KhaosEnvProbeNode.h"
#include "KhaosVolumeProbeNode.h"
#include "KhaosSkyEnv.h"
#include "KhaosInstObjSharedData.h"
#include "KhaosSceneBVH.h"


namespace Khaos
{
    class SceneGraph;

    //////////////////////////////////////////////////////////////////////////
    // Scene Find Option
    class SceneFindOption
    {
    public:
        typedef bool (*PreCullCallbackFunc)( void*, SceneNode* );
        typedef void (*FindCallbackFunc)( void*, SceneNode* );
        
    public:
        enum Option
        {
            OP_CAMERA_CULL,
            OP_RAY_INTERSECT,
            OP_LMRAY_INTERSECT
        };

    public:
        SceneFindOption() : option(OP_CAMERA_CULL), nodeMask(0), context(0), camera(0), cullDisabled(true), preCull(0)
        {
            KHAOS_CLEAR_ARRAY(resultNodes);
        }

    public:
        Option      option;
        uint32      nodeMask;
        void*       context;

        union
        {
            Camera*   camera;
            Ray*      ray;
        };
        
        bool        cullDisabled;

        PreCullCallbackFunc preCull;
        FindCallbackFunc    resultNodes[NT_MAX];
    };

    //////////////////////////////////////////////////////////////////////////
    class _FindBaseOp : public AllocatedObject, public AreaOctree::IFindObjectCallback
    {
    public:
        _FindBaseOp( SceneGraph* sg, SceneFindOption* sfo );
        virtual ~_FindBaseOp() {}

    protected:
        SceneFindOption::FindCallbackFunc _preCheck( SceneNode* node ) const;

    protected:
        SceneGraph*      m_sg;
        SceneFindOption* m_sfo;
    };

    class _FindByCamera : public _FindBaseOp
    {
    public:
        _FindByCamera( SceneGraph* sg, SceneFindOption* sfo ) : _FindBaseOp(sg, sfo) {}

        virtual AreaOctree::Visibility   onTestAABB( const AxisAlignedBox& box );
        virtual AreaOctree::ActionResult onTestObject( Octree::ObjectType* obj, AreaOctree::Visibility vis );
    };

    template<class T>
    class _FindByRay : public _FindBaseOp
    {
    public:
        _FindByRay( SceneGraph* sg, SceneFindOption* sfo ) : _FindBaseOp(sg, sfo) {}

        virtual AreaOctree::Visibility   onTestAABB( const AxisAlignedBox& box );
        virtual AreaOctree::ActionResult onTestObject( Octree::ObjectType* obj, AreaOctree::Visibility vis );
    };

    //////////////////////////////////////////////////////////////////////////
    // travel
    struct ITravelSceneGraphCallback
    {
        virtual void onVisitNode( SceneNode* node ) = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    // SceneGraph
    class SceneGraph : public AllocatedObject
    {
    public:
        typedef unordered_map<String, SceneNode*>::type NodeMap;
        typedef vector<SceneNode*>::type                NodeList;

    public:
        SceneGraph();
        ~SceneGraph();

    public:
        void _setName( const String& name ) { m_name = name; }
        const String& getName() const { return m_name; }

    public:
        // 场景创建
        SceneNode* createSceneNode( ClassType type, const String& name );

        template<class T>
        T* createSceneNode( const String& name ) { return static_cast<T*>(createSceneNode(KHAOS_CLASS_TYPE(T), name)); }

        AreaNode* createRootNode( const String& name );
        AreaNode* getRootNode( const String& name ) const;
        NodeList& _getRootNodes() { return m_roots; }

        // 场景销毁
        void       destroySceneNode( const String& name );
        void       destroySceneNode( SceneNode* node );
        void       destroyDerivedSceneNode( const String& name );
        void       destroyDerivedSceneNode( SceneNode* node );
        void       destroyAllSceneNodes();

        // 场景查询
        SceneNode* getSceneNode( const String& name ) const;

        void find( SceneFindOption& sfo );

        void travel( ITravelSceneGraphCallback* trv );

        // 场景更新
        void update();

        // 全局对象
        SkyEnv& getSkyEnv() { return m_skyEnv; }

        InstanceSharedDataMgr& getInstSharedDataMgr() { return m_instSharedDataMgr; }

        void initSceneBVH();
        SceneBVH* getBVH() const { return m_bvh; }

    private:
        void _findBase( AreaOctree::IFindObjectCallback* fn );
        void _findByCamera( SceneFindOption& sfo );
        template<class T>
        void _findByRay( SceneFindOption& sfo );

        void _travelNode( SceneNode* node, ITravelSceneGraphCallback* trv );

    private:
        // 场景图名称
        String m_name;

        // 所有节点的注册表
        NodeMap m_nodeMap;

        // 根节点列表
        NodeList m_roots;

        // 全局对象
        SkyEnv m_skyEnv;

        // 实例共享数据管理
        InstanceSharedDataMgr m_instSharedDataMgr;

        // 场景bvh，烘焙系统等使用
        SceneBVH* m_bvh;
    };


    //////////////////////////////////////////////////////////////////////////
    // Scene Find Utils
    class SGFindBase : public AllocatedObject
    {
    public:
        void setFinder( Camera* camera );
        void setFinder( Ray* ray );
        void setFinder( LimitRay* ray );
        void setNodeMask( uint32 nodeMask );
        void setCullDisable( bool cullDisable );
        void setContext( void* context );
        void setPreCull( SceneFindOption::PreCullCallbackFunc preCull );
        void setResult( int type, SceneFindOption::FindCallbackFunc result );

        void find( SceneGraph* sg );

    protected:
        SceneFindOption m_sfo;
    };

#define KHAOS_SFG_SET_PRECULL(cls, func) \
    struct func##_Proxy \
    { \
        static bool proxy( void* context, SceneNode* node ) \
        { \
            return static_cast<cls*>(context)->func( node ); \
        } \
    }; \
    setContext( this ); \
    setPreCull( &func##_Proxy::proxy );

#define KHAOS_SFG_SET_RESULT(cls, type, func) \
    struct func##_Proxy \
    { \
        static void proxy( void* context, SceneNode* node ) \
        { \
            static_cast<cls*>(context)->func( node ); \
        } \
    }; \
    setContext( this ); \
    setResult( type, &func##_Proxy::proxy );
}

