#pragma once
#include "KhaosPlane.h"
#include "KhaosAxisAlignedBox.h"
#include "KhaosRay.h"
#include "KhaosMeshNode.h"
#include "KhaosSSE.h"

namespace Khaos
{
    typedef Vector2 SBVHVec2;
    typedef Vector3 SBVHVec3;
    typedef Vector4 SBVHVec4;
   
    struct SBVHBox
    {
        SBVHVec3 minPt;
        SBVHVec3 maxPt;

        SBVHBox() : minPt(0.5f), maxPt(-0.5f) {}
        SBVHBox( const SBVHVec3& m1, const SBVHVec3& m2 ) : minPt(m1), maxPt(m2) {}

        void mergeFirstPoint( const SBVHVec3& pt );
        void mergeNextPoint( const SBVHVec3& pt );

        bool isEmpty() const;

        SBVHBox operator+( const SBVHBox& rhs ) const;
    };

    struct SBVHAABBGroup
    {
        // Min planes for this set of bounding volumes. Array index is X/Y/Z.
	    KHAOS_ALIGN_16 SBVHVec4 minPts[3];

        // Max planes for this set of bounding volumes. Array index is X/Y/Z.
        KHAOS_ALIGN_16 SBVHVec4 maxPts[3];

        SBVHAABBGroup()
        {
            SBVHBox nullBox;
            setBox(0, nullBox);
            setBox(1, nullBox);
            setBox(2, nullBox);
            setBox(3, nullBox);
        }

        float getMinX( int boxIndex ) const { return minPts[0][boxIndex]; }
        float getMinY( int boxIndex ) const { return minPts[1][boxIndex]; }
        float getMinZ( int boxIndex ) const { return minPts[2][boxIndex]; }

        void setMinX( int boxIndex, float v ) { minPts[0][boxIndex] = v; }
        void setMinY( int boxIndex, float v ) { minPts[1][boxIndex] = v; }
        void setMinZ( int boxIndex, float v ) { minPts[2][boxIndex] = v; }

        float getMaxX( int boxIndex ) const { return maxPts[0][boxIndex]; }
        float getMaxY( int boxIndex ) const { return maxPts[1][boxIndex]; }
        float getMaxZ( int boxIndex ) const { return maxPts[2][boxIndex]; }

        void setMaxX( int boxIndex, float v ) { maxPts[0][boxIndex] = v; }
        void setMaxY( int boxIndex, float v ) { maxPts[1][boxIndex] = v; }
        void setMaxZ( int boxIndex, float v ) { maxPts[2][boxIndex] = v; }

        void setBox( int boxIndex, const SBVHBox& box )
        {
            minPts[0][boxIndex] = box.minPt.x;
            minPts[1][boxIndex] = box.minPt.y;
            minPts[2][boxIndex] = box.minPt.z;

            maxPts[0][boxIndex] = box.maxPt.x;
            maxPts[1][boxIndex] = box.maxPt.y;
            maxPts[2][boxIndex] = box.maxPt.z;
        }

        SBVHBox getBox( int boxIndex ) const
        {
            return SBVHBox( 
                SBVHVec3(minPts[0][boxIndex], minPts[1][boxIndex], minPts[2][boxIndex]),
                SBVHVec3(maxPts[0][boxIndex], maxPts[1][boxIndex], maxPts[2][boxIndex])
            );
        }
    };

    struct SBVHFaceInfo // 一个三角面的信息
    {
        SBVHFaceInfo() : instanceID(-1), subIndex(-1), faceIdx(-1) {}
        SBVHFaceInfo( int i, int s, int f ) : instanceID(i), subIndex(s), faceIdx(f) {}

        void buildOtherInfo();

        int instanceID; // 对应的实例
        int subIndex; // 对应实例的子模型
        int faceIdx; // 在子模型的面索引 

        SBVHVec3 worldVtx0; // 世界空间的顶点
        SBVHVec3 worldVtx1;
        SBVHVec3 worldVtx2;

        SBVHVec3 worldCenter; // 缓存中心点

#if 0
        SBVHVec3 worldNorm; // 没有规则化的法线 
        SBVHVec3 worldEdge1;
        SBVHVec3 worldEdge2;
#else
        VectorRegister vrVtx0;
        VectorRegister vrVtx1;
        VectorRegister vrVtx2;
        VectorRegister vrNorm;
        VectorRegister vrEdge1;
        VectorRegister vrEdge2;
#endif
        
    };

    struct SBVHFaceGroupInfo // 一个组四个面
    {
        SBVHFaceGroupInfo() { faceIDs[0] = faceIDs[1] = faceIDs[2] = faceIDs[3] = -1; }
        int faceIDs[4];
    };

    //////////////////////////////////////////////////////////////////////////
    struct SceneBVHNode
    {
    public:
        static SceneBVHNode* createNode();
        static void deleteDerived( SceneBVHNode* root );

    private:
        SceneBVHNode();
        
    public:
        ~SceneBVHNode();

        void makeLeaf();

        bool isLeaf() const
        {
            khaosAssert( !faceGroup || (!leftChild && !rightChild) );
            return faceGroup != 0;
        }

        SBVHBox getSelfBox() const
        {
            return boxGroup->getBox(0) + boxGroup->getBox(1);
        }

    public:
        SBVHAABBGroup*      boxGroup;  // 01-自己的两个孩子的左右空间；23-左孩子的两个孩子的左右空间
        SceneBVHNode*       leftChild; // 左右两个空间
        SceneBVHNode*       rightChild;
        SBVHFaceGroupInfo*  faceGroup; // 如果是叶子节点的话，存放面组（包含4个面）信息
        int                 beginFaceIndex; // 面信息起始索引
        int                 endFaceIndex; // 面信息结束索引, count = end - begin
    };

    //////////////////////////////////////////////////////////////////////////
    class SceneBVH : public AllocatedObject
    {
    public:
        typedef vector<String>::type NameList;

        typedef vector<SceneNode*>::type SceneNodeList; // instance id -> scene node

        typedef vector<SBVHFaceInfo*>::type FaceInfoArray; // face id -> face info

        template<class T>
        struct QueryContext
        {
            QueryContext( const T& ray_ );

            VectorRegister rayPos;
            VectorRegister negDir;

            VectorRegister originX;
            VectorRegister originY;
            VectorRegister originZ;
            VectorRegister invDirX;
            VectorRegister invDirY;
            VectorRegister invDirZ;

            const T& ray;
        };

        struct Result
        {
            Result() : faceID(-1), distance(Math::POS_INFINITY) {}

            void clear()
            {
                faceID = -1;
                distance = Math::POS_INFINITY;
            }

            int      faceID;
            float    distance;
            SBVHVec3 gravity;
        };

    public:
        SceneBVH();
        ~SceneBVH();

        void openFile( const String& file );
        void saveFile( const String& file );

        void addSceneNode( MeshNode* node );
        void build();

        bool intersect( const Ray& ray, Result& ret ) const;
        bool intersect( const LimitRay& ray, Result& ret ) const;

        void intersectTriAlways( const Ray& ray, int faceID, Result& ret ) const;
        void intersectTriAlways( const LimitRay& ray, int faceID, Result& ret ) const;

        SceneNode* getSceneNode( int instanceID ) const { return m_nodes[instanceID]; }
        const SBVHFaceInfo* getFaceInfo( int faceID ) const { return m_faceInfos[faceID]; }

        const NameList& getSceneNodeNames() const { return m_nodeNames; }
        void _addSceneNodeForName( SceneNode* node );
        void _completeAddSceneNodeForName();

    private:
        // build functions
        void _buildMeshNode( MeshNode* node, int instanceID );
        void _clearAllFaceInfos();

        void _devideDerived( SceneBVHNode* node, int level );
        SBVHBox _calcFaceGroupBound( int beginIdx, int endIdx ) const;
        
        // query functions
        bool _rayIntersectsTri( const QueryContext<Ray>& context, const SBVHFaceInfo* info,
            float& t, float& u, float& v ) const;

        void _rayIntersectsTriAlways( const QueryContext<Ray>& context, const SBVHFaceInfo* info,
            float& t, float& u, float& v ) const;

        void _checkNodeBoundBox( SceneBVHNode* node, const QueryContext<Ray>& context, float histroyMinTime, 
            SBVHVec4& hitTime, int boxHit[4] ) const;

        template<class T>
        void _calcIntersectPt( const SBVHFaceGroupInfo* faceGroup, const QueryContext<T>& context, Result& ret ) const;
        
        template<class T>
        void _intersectDerivedPreCalculated( SceneBVHNode* node, const QueryContext<T>& context, Result& ret, const SBVHVec4& hitTime, int boxHit[4] ) const;

        template<class T>
        void _intersectDerived( SceneBVHNode* node, const QueryContext<T>& context, Result& ret ) const;

        template<class T>
        bool _intersect( const T& ray, Result& ret ) const;

        template<class T>
        void _intersectTriAlways( const T& ray, int faceID, Result& ret ) const;

    private:
        SceneBVHNode*  m_root;

        NameList       m_nodeNames;
        SceneNodeList  m_nodes;
        FaceInfoArray  m_faceInfos;
    };
}

