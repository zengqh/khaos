#include "KhaosPreHeaders.h"
#include "KhaosGI.h"
#include "KhaosMeshManager.h"
#include "KhaosRay.h"
#include "KhaosBinStream.h"


namespace Khaos
{
    const int MAX_GEN_SAMPLES = 30;

    //////////////////////////////////////////////////////////////////////////
    class TriSample
    {
    public:
        struct Data
        {
            float u, v, w;
        };

        typedef std::vector<Data> DataArray;

    public:
        static TriSample& getInstance()
        {
            static TriSample inst;
            return inst;
        }

    public:
        TriSample()
        {
            _buildTables( 500 );
        }

        const Data& getUVW( int i ) const
        {
            return m_datas[i];
        }

    private:
        void _buildTables( int cnt )
        {
            if ( g_fileSystem->isExist("trisam.data") )
            {
                FileSystem::DataBufferAuto buff;
                g_fileSystem->getFileData( "trisam.data", buff );
                int cnt = buff.dataLen / (sizeof(Data));
                m_datas.resize( cnt );
                memcpy( &m_datas[0], buff.data, buff.dataLen );
                return;
            }

            m_datas.resize( cnt );

            for ( int i = 0; i < cnt; ++i )
            {
                Data& data = m_datas[i];
                while ( !_get_rand_uv( data.u, data.v, data.w ) );
            }

            DataBuffer buff;
            buff.data = &m_datas[0];
            buff.dataLen = sizeof(m_datas[0]) * m_datas.size();
            g_fileSystem->writeFile( "trisam.data", buff );
        }

        bool _get_rand_uv( float& u, float& v, float& w ) const
        {
            float e1 = Math::unitRandom();
            float e2 = Math::unitRandom();

            e1 = Math::sqrt( e1 );
            u  = 1.0f - e1;
            v  = e2 * e1;

            w = 1.0f - u - v;
            if ( w < 0.0f || w > 1.0f )
            {
                char msg[256] = {};
                sprintf( msg, "u, v, w = %f, %f, %f\n", u, v, w );
                OutputDebugStringA( msg );
                return false;
            }

            return true;
        }

    private:
        DataArray m_datas;
    };
    

    //////////////////////////////////////////////////////////////////////////
    GIThreadPool::GIThreadPool() : m_func(0)
    {

    }

    GIThreadPool::~GIThreadPool()
    {
        join();
        for ( size_t i = 0; i < m_threads.size(); ++i )
        {
            KHAOS_DELETE_T( m_threads[i] );
        }
    }

    void GIThreadPool::addThread( void* para )
    {
        Thread* thread = KHAOS_NEW_T(Thread);
        thread->bind_func( m_func, para );
        m_threads.push_back( thread );
    }

    void GIThreadPool::run()
    {
        for ( size_t i = 0; i < m_threads.size(); ++i )
        {
            m_threads[i]->run();
        }
    }

    void GIThreadPool::join()
    {
        for ( size_t i = 0; i < m_threads.size(); ++i )
        {
            m_threads[i]->join();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    GIMesh::Face::~Face()
    {
        KHAOS_FREE(factors);
    }

    void GIMesh::Face::addFactor( int faceIdx, float factor )
    {
        FormFactor* factorNewBuf = KHAOS_MALLOC_ARRAY_T(FormFactor, factorCnt+1);

        if ( factorCnt > 0 )
            memcpy( factorNewBuf, factors, factorCnt * sizeof(FormFactor) );

        factorNewBuf[factorCnt].faceIdx = faceIdx;
        factorNewBuf[factorCnt].factor  = factor;
        ++factorCnt;

        KHAOS_FREE(factors);
        factors = factorNewBuf;
    }

    float GIMesh::Face::getFactorTotal() const
    {
        float tot = 0;
        for ( int i = 0; i < factorCnt; ++i )
            tot += factors[i].factor;
        return tot;
    }

    void GIMesh::Face::scaleFactor( float x )
    {
        for ( int i = 0; i < factorCnt; ++i )
            factors[i].factor *= x;
    }

    GIMesh::VtxInfo::~VtxInfo()
    {
        KHAOS_FREE(face);
    }

    void GIMesh::VtxInfo::addFace( int f )
    {
        // 这里最小存储方式
        int* faceNewBuf = KHAOS_MALLOC_ARRAY_T(int, faceCnt+1);

        if ( faceCnt > 0 )
            memcpy( faceNewBuf, face, faceCnt * sizeof(int) );

        faceNewBuf[faceCnt] = f;
        ++faceCnt;

        KHAOS_FREE(face);
        face = faceNewBuf;
    }

    //////////////////////////////////////////////////////////////////////////
    GIMesh::GIMesh() : m_outMesh(0)
    {
    }

    GIMesh::~GIMesh()
    {
        KHAOS_SAFE_RELEASE( m_outMesh );
        
        int vtxCnt = (int) m_vtxInfos.size();
        for ( int i = 0; i < vtxCnt; ++i  )
        {
            VtxInfo* vi = m_vtxInfos[i];
            KHAOS_DELETE vi;
        }
    }

    void GIMesh::addMesh( const Vector3* posBuff, int stride, int posCnt, const int* idxBuff, int idxCnt, const Color& clr )
    {
        int lastVtxCnt = (int) m_posBuf.size();

        // 加入顶点
        for ( int i = 0; i < posCnt; ++i )
        {
            const Vector3& pos = *(const Vector3*)((const uint8*)posBuff + i * stride);
            m_posBuf.push_back( pos );
        }

        // 加入面
        khaosAssert( idxCnt % 3 == 0 );
        int faceCnt = idxCnt / 3;

        for ( int i = 0; i < faceCnt; ++i )
        {
            Face f;

            f.v0 = (*idxBuff) + lastVtxCnt;
            ++idxBuff;

            f.v1 = (*idxBuff) + lastVtxCnt;
            ++idxBuff;

            f.v2 = (*idxBuff) + lastVtxCnt; 
            ++idxBuff;

            f.mtr = clr.getAsARGB();

            m_faces.push_back( f );
        }
    }

    void GIMesh::initComplete()
    {
        _buildFaceInfo();
        _buildVtxInfo();
        _buildFactors();
        _buildOutMesh();
    }

    void GIMesh::testDirLit( const Vector3& dir1, const Color& dirClr )
    {
        Vector3 dir = -dir1.normalisedCopy();

        int faceCnt = (int) m_faces.size();
        
        for ( int i = 0; i < faceCnt; ++i )
        {
            Face&     f  = m_faces[i];
            FaceInfo& fi = m_faceInfos[i];

            float ldotn = dir.dotProduct( fi.normal );

            if ( ldotn > 0 )
            {
                Color diff;
                diff.setAsARGB( f.mtr );

                Color total = dirClr * diff * ldotn;
                f.dirLit = total.getAsARGB();
            }
            else
            {
                f.dirLit = 0;
            }
        }
    }

    void GIMesh::calcIndirectLit()
    {
        int faceCnt = (int) m_faces.size();

        for ( int i = 0; i < faceCnt; ++i )
        {
            Face& face_i  = m_faces[i];
            
            // 其他面对它的贡献
            Color indir_total(Color::BLACK);
            int factorCnt = face_i.factorCnt;

            for ( int j = 0; j < factorCnt; ++j )
            {
                int   fj_idx  = face_i.factors[j].faceIdx;
                float form_ij = face_i.factors[j].factor;

                const Face& face_j = m_faces[fj_idx];
                
                Color bj;
                bj.r = (face_j.dirLitR + face_j.clrR) / 255.0f;
                bj.g = (face_j.dirLitG + face_j.clrG) / 255.0f;
                bj.b = (face_j.dirLitB + face_j.clrB) / 255.0f;

                indir_total += bj * form_ij;
            }

            Color diff;
            diff.setAsARGB( face_i.mtr );
            indir_total = diff * indir_total;

            face_i.clrR = Math::clamp( int(indir_total.r * 255), 0, 255 );
            face_i.clrG = Math::clamp( int(indir_total.g * 255), 0, 255 );
            face_i.clrB = Math::clamp( int(indir_total.b * 255), 0, 255 );
        }
    }

   

    void GIMesh::_genSamples( int i, Vector3Array& vtx ) const
    {
        vtx.clear();

        const Face&    f  = m_faces[i];
        const Vector3& v0 = m_posBuf[f.v0];
        const Vector3& v1 = m_posBuf[f.v1];
        const Vector3& v2 = m_posBuf[f.v2];
        
        const TriSample& sample = TriSample::getInstance();

        for ( int i = 0; i < MAX_GEN_SAMPLES; ++i )
        {
            const TriSample::Data& sam = sample.getUVW( i );
            vtx.push_back( v0 * sam.u + v1 * sam.v + v2 * sam.w );
        }
    }

    bool GIMesh::_isVisible( const Vector3& vx, const Vector3& vy, int fx, int fy ) const
    {
        Vector3 vxy = vy - vx;
        float xy_len = vxy.normalise();

        if ( xy_len < 1e-3f ) // 两点很近，认为中间没有遮挡
            return true;

        Ray ray(vx, vxy);

        int faceCnt = (int)m_faces.size();
        
        for ( int i = 0; i < faceCnt; ++i )
        {
            if ( i == fx || i == fy ) // 和自己的面不处理
                continue;

            const Face& fa = m_faces[i];
            float inter_t = 0;

            if ( ray.intersects( m_posBuf[fa.v0], m_posBuf[fa.v1], m_posBuf[fa.v2], &inter_t ) ) // 和其中一个面交
            {
                if ( 1e-3f < inter_t && inter_t < xy_len - 1e-3f ) // 在x,y中间
                    return false;
            }
        }

        return true;
    }

    float GIMesh::_calcFormXToAj( const Vector3& vx, const Vector3Array& vtxJSet, int fx, int fy ) const
    {
        // 计算
        // ∫V(x,y)cos(t1)cos(t2)/(PI*r^2)dAy

        const FaceInfo& faceInfoI = m_faceInfos[fx];
        const FaceInfo& faceInfoJ = m_faceInfos[fy];

        const Vector3& iNormal = faceInfoI.normal;
        const Vector3& jNormal = faceInfoJ.normal;

        float jArea = faceInfoJ.area;

        float total = 0;
        int cnt = (int)vtxJSet.size();

        for ( int i = 0; i < cnt; ++i )
        {
            const Vector3& vy = vtxJSet[i];
            Vector3 vxy = vy - vx;
            float r = vxy.normalise();
            
            if ( r > 0.1f ) // 忽略相同的点(距离很近)，这个数过小引起/r^2不准确
            {
                float cos_t1 = iNormal.dotProduct( vxy );
                float cos_t2 = jNormal.dotProduct( -vxy );
                
                const float e = 0.00873f; // 89.5度 

                if ( cos_t1 > e && cos_t2 > e ) // 不在背面
                {
                    if ( _isVisible(vx, vy, fx, fy) ) // 中间无遮挡
                    {
                        float val = float((cos_t1 * (double)cos_t2) / ((double)r * r));
                        total += val;

                        //char msg[256];
                        //sprintf( msg, "val(%d) = %f, %f, || %f, %f, %f\n",
                        //    i, val, total, cos_t1, cos_t2, r );
                        //OutputDebugStringA( msg );
                    }
                }
            }
        }

        // E = (1/N) ∑(f(x) / P(x))
        // P(x) = 1/jArea
        // E = jArea / N * ∑f(x)
        total = (jArea * total) / (cnt * Math::PI);
        return total;
    }

    float GIMesh::_calcFormFactor( int i, int j ) const
    {
        // 计算 
        // F(i,j) = (1/A(i)) * ∫∫V(x,y)cos(t1)cos(t2)/(PI*r^2)dAydAx
        
        // 假如两个面法线相同，那么0
        float dotij = m_faceInfos[i].normal.dotProduct( m_faceInfos[j].normal );
        if ( Math::fabs(dotij - 1) < 1e-3f )
            return 0;

        // 在三角形i,j上生成采样点
        Vector3Array vtxISet, vtxJSet;
        _genSamples( i, vtxISet );
        _genSamples( j, vtxJSet );

        float total = 0;
        int cnt = (int) vtxISet.size();

        for ( int k = 0; k < cnt; ++k )
        {
            float xtoj = _calcFormXToAj( 
                vtxISet[k], vtxJSet, i, j );

            total += xtoj;

            //char msg[256] = {};
            //sprintf( msg, "xtoj(%d) = %f, %f\n", k, xtoj, total );
            //OutputDebugStringA( msg );
        }

        total = total / cnt; // iArea面积抵消
        return total;
    }

    bool GIMesh::_readFactors()
    {
        if ( !g_fileSystem->isExist( "gi.data" ) )
            return false;

        FileSystem::DataBufferAuto db;
        if ( !g_fileSystem->getFileData( "gi.data", db ) )
            return false;

        BinStreamReader bsr( db.data, db.dataLen );

        int faceCnt = (int) m_faces.size();
        
        for ( int i = 0; i < faceCnt; ++i )
        {
            int factorsCnt = 0;
            bsr.read( factorsCnt );

            Face& fa = m_faces[i];

            for ( int j = 0; j < factorsCnt; ++j )
            {
                FormFactor ff;
                bsr.read( ff );
                fa.addFactor( ff.faceIdx, ff.factor );
            }
        }

        return true;
    }

    void GIMesh::_writeFactors()
    {
        // 写入文件
        BinStreamWriter bsw;

        int faceCnt = (int) m_faces.size();

        for ( int i = 0; i < faceCnt; ++i )
        {
            Face& fa = m_faces[i];

            int factorCnt = fa.factorCnt;
            bsw.write( factorCnt );

            for ( int j = 0; j < factorCnt; ++j )
            {
                bsw.write( fa.factors[j] );
            }
        }

        DataBuffer db;
        db.data = bsw.getBlock();
        db.dataLen = bsw.getCurrentSize();
        g_fileSystem->writeFile( "gi.data", db );
    }

    bool GIMesh::_sortFactor( const FormFactor& lhs, const FormFactor& rhs )
    {
        return lhs.factor > rhs.factor;
    }

    void GIMesh::_sortFaceFactors( int faceIdx )
    {
        const int limFacCnt = 100;
        
        Face& fa = m_faces[faceIdx];
        int factorCnt = fa.factorCnt;
        float factorOldTotal = fa.getFactorTotal();

        std::sort( fa.factors, fa.factors+factorCnt, _sortFactor );

        for ( int i = limFacCnt; i < factorCnt; ++i )
            fa.factors[i].factor = 0;

        int faceZeroCnt = 0;
        for ( ; faceZeroCnt < factorCnt; ++faceZeroCnt )
        {
            if ( Math::fabs( fa.factors[faceZeroCnt].factor ) <= 1e-5f )
                break;
        }

        fa.factorCnt = faceZeroCnt;

        if ( factorOldTotal > 0.05f )
        {
            float curTotal = fa.getFactorTotal();
            float s = factorOldTotal / curTotal;
            fa.scaleFactor( s );

            char msg[256] = {};
            sprintf( msg, "sort factor(%d) scale: %f => %f\n", faceIdx, curTotal, factorOldTotal );
            OutputDebugStringA( msg );
        }

        char msg[256] = {};
        sprintf( msg, "sort factor(%d) count: %d\n", faceIdx, faceZeroCnt );
        OutputDebugStringA( msg );
    }

    void GIMesh::_adjustFaceFactors()
    {
        int faceCnt = (int) m_faces.size();

        for ( int i = 0; i < faceCnt; ++i )
        {
            Face& fa = m_faces[i];
            float factorCurrTotal = fa.getFactorTotal();

            // 超过1.0调整
            if ( factorCurrTotal > 1.0f )
            {
                float s = 1.0f / factorCurrTotal;
                fa.scaleFactor( s );

                char msg[256] = {};
                sprintf( msg, "adjust factor(%d): %f\n", i, factorCurrTotal );
                OutputDebugStringA( msg );
            }

            _sortFaceFactors( i );
        }
    }

    void GIMesh::_buildFaceFactors( int faceIdx )
    {
        Face& fa = m_faces[faceIdx];
        int faceCnt = (int) m_faces.size();
        float total = 0;

        for ( int j = 0; j < faceCnt; ++j )
        {
            if ( faceIdx == j ) // 自己忽略
                continue;

            float factor = _calcFormFactor( faceIdx, j );
            total += factor;

            //char buf[256] = {};
            //sprintf( buf, "curr total(%d) = %f, %f\n", j, factor, total );
            //OutputDebugStringA( buf );

            fa.addFactor( j, factor );
        }

        // print info
        char buf[256] = {};
        sprintf( buf, "form total(%d) = %f\n", faceIdx, total );
        OutputDebugStringA( buf );

        // 矫正到1.0
        //if ( total > 0.5f )
        //{
        //    float scale = 1.0f / total;

        //    for ( int k = 0; k < fa.factorCnt; ++k )
        //    {
        //        fa.factors[k].factor *= scale;
        //    }
        //}
    }

    void GIMesh::_threadCalcFaceFactors( void* para )
    {
        ThreadCalcFaceFactorsPara* ffp = (ThreadCalcFaceFactorsPara*)para;
        GIMesh* mesh = ffp->mesh;

        for ( int it = ffp->faceBegin, ite = ffp->faceEnd; it < ite; ++it )
        {
            mesh->_buildFaceFactors( it );
        }
    }

    void GIMesh::_buildFactors()
    {
        //return;

        if ( _readFactors() )
        {
            _adjustFaceFactors();
            return;
        }

        //for ( int i = 0; i < 1000; ++i )
        //rand();

        TriSample::getInstance(); // 初始化，避免多线程

        //float tt = _calcFormFactor( 150, 249 );
        //_buildFaceFactors( 150 );

        int faceCnt = (int) m_faces.size();
        const int threadCnt = 4; // 待定
        int faceCntPerThread = faceCnt / threadCnt;

        m_thrCalc.setOperate( _threadCalcFaceFactors );
        
        ThreadCalcFaceFactorsPara paras[threadCnt];

        for ( int i = 0; i < threadCnt; ++i )
        {
            ThreadCalcFaceFactorsPara& para = paras[i];

            para.mesh      = this;
            para.faceBegin = i * faceCntPerThread;
            para.faceEnd   = para.faceBegin + faceCntPerThread;

            if ( i == threadCnt - 1 )
                para.faceEnd = faceCnt;

            m_thrCalc.addThread( &para );    
        }

        m_thrCalc.run();
        m_thrCalc.join();

        _writeFactors();
        _adjustFaceFactors();
    }

    uint32 GIMesh::_calcVtxColor( int i ) const
    {
        // 计算顶点颜色通过周边面颜色
        const VtxInfo* vi = m_vtxInfos[i];
        
        int count_r = 0;
        int count_g = 0;
        int count_b = 0;

        int faceCnt = vi->faceCnt;

        for ( int j = 0; j < faceCnt; ++j )
        {
            const Face& f = m_faces[vi->face[j]];
            count_r += f.clrR + f.dirLitR; // test dirlit
            count_g += f.clrG + f.dirLitG;
            count_b += f.clrB + f.dirLitB;
        }

        count_r /= faceCnt;
        count_g /= faceCnt;
        count_b /= faceCnt;

        count_r = Math::clamp( count_r, 0, 255 );
        count_g = Math::clamp( count_g, 0, 255 );
        count_b = Math::clamp( count_b, 0, 255 );

        return Color::makeARGB( 255, count_r, count_g, count_b );
    }

    void GIMesh::_buildFaceInfo()
    {
        int faceCnt = (int) m_faces.size();
        m_faceInfos.resize( faceCnt );

        for ( int i = 0; i < faceCnt; ++i )
        {
            const Face& f = m_faces[i];
            FaceInfo& fi = m_faceInfos[i];

            fi.normal = Math::calcNormal( m_posBuf[f.v0], m_posBuf[f.v1], m_posBuf[f.v2] );
            fi.area   = Math::calcArea( m_posBuf[f.v0], m_posBuf[f.v1], m_posBuf[f.v2] );
        }
    }

    void GIMesh::_buildVtxInfo()
    {
        // 创建邻接表
        int vtxCnt= (int)m_posBuf.size();
        m_vtxInfos.resize( vtxCnt );

        int faceCnt = (int)m_faces.size();

        for ( int i = 0; i < vtxCnt; ++i )
        {
            VtxInfo* vi = KHAOS_NEW VtxInfo;

            for ( int j = 0; j < faceCnt; ++j )
            {
                const Face& f = m_faces[j];
                if ( f.v0 == i || f.v1 == i || f.v2 == i ) // 该面使用此点
                    vi->addFace(j);
            }

            m_vtxInfos[i] = vi;
        }
    }

    void GIMesh::_buildOutMesh()
    {
        khaosAssert( !m_outMesh );

        m_outMesh = MeshManager::createMesh( "" );
        m_outMesh->setNullCreator();

        SubMesh* subMesh = m_outMesh->createSubMesh();
        subMesh->setPrimitiveType( PT_TRIANGLELIST );

        // vb
        VertexBuffer* vb = subMesh->createVertexBuffer();
        vb->create( sizeof(VertexPTC) * m_posBuf.size(), HBU_DYNAMIC );
        vb->setDeclaration( g_vertexDeclarationManager->getDeclaration(VertexPTC::ID) );

        // ib
        IndexBuffer* ib = subMesh->createIndexBuffer();
        ib->create( sizeof(int) * m_faces.size() * 3, HBU_STATIC, IET_INDEX32 );

        // update
        _updateVB();
        _updateIB();
        m_outMesh->updateAABB( true );
    }

    void GIMesh::_updateVB()
    {
        SubMesh* subMesh = m_outMesh->getSubMesh(0);
        VertexBuffer* vb = subMesh->getVertexBuffer();

        int vtxCnt = (int)m_posBuf.size();

        VertexPTC* dest = (VertexPTC*) vb->lock( HBA_WRITE_DISCARD );

        for ( int i = 0; i < vtxCnt; ++i )
        {
            dest->pos() = m_posBuf[i];
            dest->tex() = Vector2::ZERO;
            dest->clr = _calcVtxColor(i);

            ++dest;
        }

        vb->unlock();
    }

    void GIMesh::_updateIB()
    {
        SubMesh* subMesh = m_outMesh->getSubMesh(0);
        IndexBuffer* ib = subMesh->getIndexBuffer();

        int faceCnt = (int)m_faces.size();
        int* dest = (int*) ib->lock( HBA_WRITE );

        for ( int i = 0; i < faceCnt; ++i )
        {
            const Face& f = m_faces[i];

            *dest = f.v0;
            ++dest;

            *dest = f.v1;
            ++dest;

            *dest = f.v2;
            ++dest;
        }

        ib->unlock();
    }

    void GIMesh::updateOutMesh()
    {
        _updateVB();
    }

    //////////////////////////////////////////////////////////////////////////
    GISystem::GISystem() : m_mesh( KHAOS_NEW GIMesh )
    {

    }

    GISystem::~GISystem()
    {
        KHAOS_DELETE m_mesh;
    }

    void GISystem::addNode( MeshNode* node )
    {
        const Matrix4& matWorld = node->getDerivedMatrix();

        Mesh* mesh = node->getMesh();
        int subCnt = mesh->getSubMeshCount();

        for ( int i = 0; i < subCnt; ++i )
        {
            SubMesh*      sm = mesh->getSubMesh( i );
            VertexBuffer* vb = sm->getVertexBuffer();
            IndexBuffer*  ib = sm->getIndexBuffer();

            // get vb in world
            khaosAssert( vb->getDeclaration()->getID() == VertexPNT::ID );

            int vtxCnt = vb->getVertexCount();

            vector<VertexPNT>::type vbuff( vtxCnt );
            vb->readData( &vbuff[0] );

            for ( int v = 0; v < vtxCnt; ++v )
            {
                Vector3& posCurr = vbuff[v].pos();
                posCurr = matWorld.transformAffine( posCurr );
            }

            // get ib
            vector<int>::type ibuff;
            ib->readData( ibuff );

            // add
            Material* mtr = node->getSubEntity(i)->getImmMaterial();
            khaosAssert(0);
#if 0
            const Color& clr = mtr->getAttrib<DiffuseAttrib>()->getDiffuse();
            m_mesh->addMesh( &vbuff[0].pos(), sizeof(VertexPNT), vtxCnt, &ibuff[0], ib->getIndexCount(), clr );
#endif
        }
    }

    void GISystem::completeAddNode()
    {
        m_mesh->initComplete();
    }
}