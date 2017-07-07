#pragma once
#include "KhaosMeshNode.h"
#include "KhaosThread.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class GIThreadPool : public AllocatedObject
    {
        typedef vector<Thread*>::type ThreadList;

    public:
        GIThreadPool();
        ~GIThreadPool();

    public:
        void setOperate( Thread::callback_function func ) { m_func = func; }
        void addThread( void* para );
        void run();
        void join();

    private:
        ThreadList m_threads;
        Thread::callback_function m_func;
    };

    //////////////////////////////////////////////////////////////////////////
    class GIMesh : public AllocatedObject
    {
    public:
        struct FormFactor
        {
            int   faceIdx;
            float factor;
        };

        struct Face : public AllocatedObject
        {
            Face() : v0(0), v1(0), v2(0), 
                dirLit(0), clrR(0), clrG(0), clrB(0),
                factors(0), factorCnt(0)
            {}

            ~Face();

            void addFactor( int faceIdx, float factor );
            float getFactorTotal() const;
            void scaleFactor( float x );

            union
            {
                int     vtx[3];

                struct
                {
                    int v0, v1, v2;
                };
            };

            union
            {
                struct
                {
                    uint8 dirLitB, dirLitG, dirLitR, dirLitA;
                };

                uint32 dirLit;
            };

            int     clrR;
            int     clrG;
            int     clrB;

            uint32  mtr; // for test

            FormFactor* factors;
            int         factorCnt;
        };

        struct FaceInfo : public AllocatedObject
        {
            FaceInfo() : area(0) {}

            Vector3 normal;
            float   area;
        };

        struct VtxInfo : public AllocatedObject
        {
            VtxInfo() : face(0) , faceCnt(0) {}
            ~VtxInfo();

            void addFace( int f );

            int* face; // 相邻面列表
            int  faceCnt;
        };

        typedef vector<Face>::type     FaceArray;
        typedef vector<FaceInfo>::type FaceInfoArray;
        typedef vector<VtxInfo*>::type VtxInfoArray;
        typedef vector<Vector3>::type  Vector3Array;
        typedef vector<uint32>::type   DWordArray;

        struct ThreadCalcFaceFactorsPara
        {
            GIMesh* mesh;
            int     faceBegin;
            int     faceEnd;
        };

    public:
        GIMesh();
        ~GIMesh();

        void addMesh( const Vector3* posBuff, int stride, int posCnt, 
            const int* idxBuff, int idxCnt, const Color& clr );

        void initComplete();
        void updateOutMesh();

        FaceArray& getFaces() { return m_faces; }
        Mesh* getOutMesh() const { return m_outMesh; }

        void testDirLit( const Vector3& dir, const Color& dirClr );
        void calcIndirectLit();

    private:
        void _buildFaceInfo();
        void _buildVtxInfo();
        void _buildOutMesh();
        void _buildFactors();
        bool _readFactors();
        void _writeFactors();

        uint32 _calcVtxColor( int i ) const;

        float  _calcFormFactor( int i, int j ) const;
        float  _calcFormXToAj( const Vector3& vx, const Vector3Array& vtxJSet, int fx, int fy ) const;
        void   _buildFaceFactors( int faceIdx );
        void   _adjustFaceFactors();
        void   _sortFaceFactors( int faceIdx );
        
        static bool _sortFactor( const FormFactor& lhs, const FormFactor& rhs );

        static void _threadCalcFaceFactors( void* para );

        bool _isVisible( const Vector3& vx, const Vector3& vy, int fx, int fy ) const;

        void _genSamples( int i, Vector3Array& vtx ) const;

        void _updateVB();
        void _updateIB();

    private:
        FaceArray     m_faces;
        FaceInfoArray m_faceInfos;
        VtxInfoArray  m_vtxInfos;
        Vector3Array  m_posBuf;
        DWordArray    m_clrBuf;
        Mesh*         m_outMesh;
        GIThreadPool  m_thrCalc;
    };

    //////////////////////////////////////////////////////////////////////////
    class GISystem : public AllocatedObject
    {
    public:
        GISystem();
        ~GISystem();

        void addNode( MeshNode* node );
        void completeAddNode();

        GIMesh* getMesh() const { return m_mesh; }

    private:
        GIMesh* m_mesh;
    };
}

