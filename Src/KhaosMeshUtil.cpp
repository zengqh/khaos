#include "KhaosPreHeaders.h"
#include "KhaosMeshUtil.h"
#include "KhaosMesh.h"
#include "KhaosMeshManager.h"
#include "KhaosD3D9RenderImpl.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void SmoothGroupGather::gatherFaces( const IntList& ib, IntListArray& groups )
    {
        khaosAssert( ib.size() % 3 == 0 );
        groups.clear();
        
        map<int, int>::type vtxGroupMap; // 顶点索引 -> 平滑组

        int faceCnt = (int)ib.size() / 3;

        for ( int f = 0; f < faceCnt; ++f )
        {
            int offset = f * 3;

            // 这个面的三个顶点索引
            int vi[3] = { ib[offset], ib[offset+1], ib[offset+2] };

            // 判断他们是否已经处于否个平滑组
            int groupId = -1;
            for ( int i = 0; i < 3; ++i )
            {
                map<int, int>::type::iterator it = vtxGroupMap.find( vi[i] );
                if ( it != vtxGroupMap.end() )
                {
                    groupId = it->second;
                    break;
                }
            }

            // 如果没有则是个新平滑组
            if ( groupId == -1 )
            {
                groupId = (int)groups.size();
                groups.push_back( IntList() );
            }

            // 标记三个顶点的平滑组
            for ( int i = 0; i < 3; ++i )
                vtxGroupMap[ vi[i] ] = groupId;

            // 把面放入这个平滑组
            groups[groupId].push_back( f );
        }
    }

    void SmoothGroupGather::gatherVertices( const IntList& ib, const IntList& faces, IntSet& vertices )
    {
        // 统计一个平滑组使用到的顶点集合
        vertices.clear();

        for ( int g = 0; g < (int)faces.size(); ++g )
        {
            int f = faces[g];
            int offset = f * 3;

            vertices.insert( ib[offset] );
            vertices.insert( ib[offset+1] );
            vertices.insert( ib[offset+2] );
        }
    }

    void SmoothGroupGather::gatherSides( const IntList& ib, const IntList& faces, SideMap& sides )
    {
        // 得到一个平滑组的边的使用情况
        for ( int g = 0; g < (int)faces.size(); ++g )
        {
            int f = faces[g];
            int offset = f * 3;

            int v0 = ib[offset];
            int v1 = ib[offset+1];
            int v2 = ib[offset+2];

            ++(sides[Side(v0, v1)]);
            ++(sides[Side(v0, v2)]);
            ++(sides[Side(v1, v2)]);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    int GeneralMeshUV::s_minTexSize = 64;
    int GeneralMeshUV::s_maxTexSize = 2048;

    void GeneralMeshUV::setTexSizeBound( int minSize, int maxSize )
    {
        if ( minSize >= 0 )
            s_minTexSize = minSize;

        if ( maxSize >= 0 )
            s_maxTexSize = maxSize;
    }

    Mesh* GeneralMeshUV::apply( const String& newMeshName, int* texSize )
    {
        // prepare something
        _prepare();
     
        // calc area etc info
        int texSizeFinal = _calcSuitableTexSize( *texSize );

        // convert to d3d mesh
        void* d3dMesh = _convertD3DMesh();

        // use d3d to create uv atlas
        void* pVertexRemapArray = 0;
        void* d3dMeshOut = _createUVAtlas( d3dMesh, texSizeFinal, &pVertexRemapArray );

        // build our new mesh
        Mesh* newMesh = _buildNewMesh( newMeshName, d3dMesh, d3dMeshOut, pVertexRemapArray );
        *texSize = texSizeFinal;
        return newMesh;
    }

    void GeneralMeshUV::_prepare()
    {
        // cache first
        m_mesh->cacheLocalData( true );

        // build vb temp buffer
        int smCnt = m_mesh->getSubMeshCount();
        int vtxCnt = 0;

        m_vbMap.resize( smCnt );

        Matrix4 matNormal = m_matWorld.transpose() * m_matWorld.inverseAffine();

        for ( int i = 0; i < smCnt; ++i )
        {
            SubMesh* sm = m_mesh->getSubMesh(i);

            // vb
            VertexBuffer* vb = sm->getVertexBuffer();
            int vbCnt = vb->getVertexCount();

            m_vbMap[i].resize( vbCnt );

            for ( int v = 0; v < vbCnt; ++v )
            {
                VertexID vid(i, v, vtxCnt+v);
                
                const Vector3& posModel = *(vb->getCachePos(v));
                const Vector3& normModel = *(vb->getCacheNormal(v));

                vid.posWorld  = m_matWorld.transformAffine( posModel );
                vid.normWorld = (matNormal * Vector4(normModel, 0)).asVec3();
                vid.uv1       = *(vb->getCacheTex(v));

                if ( vb->hasElement( VFM_TAN ) ) // 有切线
                {
                    const Vector4& tangentModel = *(vb->getCacheTanget(v));
                    vid.tangentWorld.asVec3() = (matNormal * Vector4(tangentModel.asVec3(), 0)).asVec3();
                    vid.tangentWorld.w = tangentModel.w;
                }

                m_vb.push_back( vid );
                m_vbMap[i][v] = vid.id; // old submesh index, vertex index(in submesh) => new id
            }

            // ib
            IndexBuffer* ib = sm->getIndexBuffer();
            int faceCnt = sm->getPrimitiveCount();
            khaosAssert( sm->getPrimitiveType() == PT_TRIANGLELIST );

            for ( int f = 0; f < faceCnt; ++f ) // 每个三角形索引依次推入列表
            {
                int v0, v1, v2;
                ib->getCacheTriIndices( f, v0, v1, v2 );

                m_ib.push_back( vtxCnt+v0 );
                m_ib.push_back( vtxCnt+v1 );
                m_ib.push_back( vtxCnt+v2 );
            }

            vtxCnt += vbCnt;
        }
    }

    void GeneralMeshUV::_generalFaceInfo( bool needNorm, bool needArea )
    {
        // 生成面基本信息
        int faceCnt = (int) m_ib.size() / 3;
        m_faceInfo.resize( faceCnt );

        for ( int f = 0; f < faceCnt; ++f )
        {
            int i = f * 3;

            int v0 = m_ib[i];
            int v1 = m_ib[i+1];
            int v2 = m_ib[i+2];

            const Vector3& pos0 = m_vb[v0].posWorld;
            const Vector3& pos1 = m_vb[v1].posWorld;
            const Vector3& pos2 = m_vb[v2].posWorld;

            if ( needArea )
                m_faceInfo[f].area = Math::calcArea( pos0, pos1, pos2 );

            if ( needNorm )
                m_faceInfo[f].norm = Math::calcNormal( pos0, pos1, pos2 );
        }
    }

    float GeneralMeshUV::_calcTotalArea() const
    {
        // 统计mesh所有的面积和
        float areaTotal = 0;

        int faceCnt = (int) m_faceInfo.size();

        for ( int f = 0; f < faceCnt; ++f )
        {
            areaTotal += m_faceInfo[f].area;
        }

        return areaTotal;
    }

    bool GeneralMeshUV::_calcSmoothGroupAvgNormal( const SmoothGroupGather::IntList& faceGroup, Vector3& avgNorm ) const
    {
        // 找到这个平滑组平均法线
        avgNorm = Vector3::ZERO;

        int faceCnt = (int)faceGroup.size();

        for ( int g = 0; g < faceCnt; ++g )
        {
            int f = faceGroup[g];
            avgNorm += m_faceInfo[f].norm;
        }

        avgNorm.normalise();

        // 法线是否一致
        for ( int g = 0; g < faceCnt; ++g )
        {
            int f = faceGroup[g];
            float dot = avgNorm.dotProduct( m_faceInfo[f].norm );
            if ( !Math::realEqual(dot, 1.0f, 1e-2f) )
                return false;
        }

        return true;
    }

    bool GeneralMeshUV::_findSmoothGroupStretchVector( const SmoothGroupGather::IntList& faceGroup, 
        Vector3& vecMax, Vector3& avgNorm ) const
    {
        // 找到这个平滑组平均法线
        if ( !_calcSmoothGroupAvgNormal(faceGroup, avgNorm) )
            return false;

        // 得到边信息
        SmoothGroupGather gather;
        SmoothGroupGather::SideMap sides;
        gather.gatherSides( m_ib, faceGroup, sides );

        // 找到最大的边
        float sideMax = 0;
        int   faceIdx = -1;

        KHAOS_FOR_EACH( SmoothGroupGather::SideMap, sides, it )
        {
            int usedCnt = it->second;
            if ( usedCnt != 1 ) // 只有1个公共边，就是轮廓
                continue;

            const SmoothGroupGather::Side& side = it->first;

            const Vector3& posA = m_vb[side.v0].posWorld;
            const Vector3& posB = m_vb[side.v1].posWorld;

            Vector3 posAB = posB - posA;
            float   lenAB = posAB.length();

            if ( lenAB > sideMax ) // 记录最大的边
            {
                sideMax = lenAB;
                vecMax  = posAB;
            }
        } // end of per face

        return true;
    }

    bool GeneralMeshUV::_getMagnifyMatrix( const SmoothGroupGather::IntSet& vertices,
        const Vector3& vecMax, const Vector3& avgNorm, 
        Matrix4& matAdj, float& lenX, float& lenZ ) const
    {
        // 计算调整的矩阵

         // 所在面的标准基
        Vector3 axisX = vecMax.normalisedCopy();
        Vector3 axisY = avgNorm;
        Vector3 axisZ = axisX.crossProduct( axisY );

        // 转换到面空间内，得到aabb
        Vector2 rangeX, rangeZ;
        bool init = true;

        KHAOS_FOR_EACH( SmoothGroupGather::IntSet, vertices, it )
        {
            const Vector3& pt = m_vb[ *it ].posWorld;

            float x = axisX.dotProduct( pt );
            float z = axisZ.dotProduct( pt );

            if ( init )
            {
                init = false;
                rangeX.setValue( x );
                rangeZ.setValue( z );
            }
            else
            {
                rangeX.makeRange( x );
                rangeZ.makeRange( z );
            }
        }

        // 判断x,z比率是否相差过大
        lenX = rangeX.getRange();
        lenZ = rangeZ.getRange();
        float lenXPerZ = lenX / lenZ;

        const float maxMultiScale = 5;

        if ( lenXPerZ > maxMultiScale ) // x方向长度（最长）比z方向长n倍
        {
            // 我们扩大z方向倍率
            float scaleZ = lenXPerZ / maxMultiScale;
            lenZ *= scaleZ; // 最终返回调整后的lenZ

            Matrix3 matRotFace;
            matRotFace.fromAxes( axisX, axisY, axisZ );
            Matrix4 matRotFace4(matRotFace);

            Matrix4 matScale(Matrix4::IDENTITY);
            matScale.setScale( Vector3(1, 1, scaleZ) );

            matAdj = matRotFace4 * matScale * matRotFace4.inverseAffine(); // 先变换到面空间（乘以逆），然后放大，最后还原
            return true;
        }

        return false;
    }

    void GeneralMeshUV::_transformSmoothGroup( const SmoothGroupGather::IntSet& vertices, const Matrix4& mat )
    {
        // 变换组中点
        KHAOS_FOR_EACH( SmoothGroupGather::IntSet, vertices, it )
        {
            VertexID& vid = m_vb[ *it ];
            vid.posWorld = mat.transformAffine( vid.posWorld );
        }
    }

    void GeneralMeshUV::_adjustSmoothGroupStretch( int i )
    {
        const SmoothGroupGather::IntList& faceGroup = m_faceGroups[i];
        SmoothGroupInfo& groupInfo = m_sgInfos[i];

        // 找调整边
        groupInfo.isFlat = _findSmoothGroupStretchVector( faceGroup, groupInfo.vecMax, groupInfo.avgNorm );
        if ( !groupInfo.isFlat )
            return;

        // 得到所有点
        SmoothGroupGather gather;
        gather.gatherVertices( m_ib, faceGroup, groupInfo.vertices );

        // 获取调整矩阵
        Matrix4 matAdj;
        float lenX, lenZ;
        bool needAdj = _getMagnifyMatrix( groupInfo.vertices, groupInfo.vecMax, groupInfo.avgNorm, matAdj, lenX, lenZ );
        groupInfo.areaBox = lenX * lenZ; // 记录面积包围大小
        if ( !needAdj )
            return;

        // 调整
        _transformSmoothGroup( groupInfo.vertices, matAdj );
    }

    void GeneralMeshUV::_adjustSmoothGroupArea( int i, float avgArea )
    {
        const SmoothGroupGather::IntList& faceGroup = m_faceGroups[i];
        SmoothGroupInfo& groupInfo = m_sgInfos[i];

        // 只处理平面
        if ( !groupInfo.isFlat )
            return;

        // 只处理少面
        int faceCnt = (int)faceGroup.size();
        //if ( faceCnt > 2 )
        //    return;

        // 平滑组真实面积
        float totAreaRealCurr = 0;
        for ( int g = 0; g < faceCnt; ++g )
        {
            int f = faceGroup[g];
            totAreaRealCurr += m_faceInfo[f].area;
        }

        // 是否合适
        float maxArea = Math::maxVal( totAreaRealCurr, groupInfo.areaBox );
        if ( maxArea < 1e-6f )
            return;

        float esVal = Math::fabs(groupInfo.areaBox - totAreaRealCurr) / maxArea;
        if ( esVal > 0.2f ) // 误差超过20%，放弃
            return;

        // 平滑组平均面积
        float avgAreaCurr = maxArea / faceCnt;

        // 调整过小过大的面积
        const float migMultiScale = 4;
        const float magMultiScale = 4;
        float needScale = 1.0f;

        if ( (avgArea / avgAreaCurr) > migMultiScale ) // 当前过小
        {
            // 需要放大的倍数
            needScale = (avgArea / avgAreaCurr) / migMultiScale;
            needScale = Math::sqrt( needScale );
            //if ( needScale > 16 )
            //    needScale = 16;
        }
        else if ( (avgAreaCurr / avgArea) > magMultiScale ) // 当前过大
        {
            // 需要缩小的倍数
#if 1
            needScale = (avgAreaCurr / avgArea) / magMultiScale;
            needScale = Math::sqrt( needScale );
            if ( needScale > 16 )
                needScale = 16;
            needScale = 1.0f / needScale;
#endif
        }

        Matrix4 matScale(Matrix4::IDENTITY);
        matScale.setScale( Vector3(needScale) );

        // 调整
        _transformSmoothGroup( groupInfo.vertices, matScale );
    }

    float GeneralMeshUV::_getAutoTexSizeUnit( int type ) const
    {
        float pixelPerSize = 0;

        switch( type )
        {
        case AUTO_TEX_SIZE_HIGH:
            pixelPerSize = 10;
            break;

        case AUTO_TEX_SIZE_MED:
            pixelPerSize = 7;
            break;

        case AUTO_TEX_SIZE_LOW:
            pixelPerSize = 4;
            break;

        default:
            khaosAssert(0);
            break;
        }

        return pixelPerSize;
    }

    int GeneralMeshUV::_estimateTexSize( int type, float area, int smoothGroups ) const
    {
        float space_size;

        float area_side = Math::maxVal( Math::sqrt( area ), (float)s_minTexSize );
        if ( area_side <= 32 )
            space_size = 4;
        else
            space_size = 3;

        // 计算padding面积
        float avgArea = area / smoothGroups;
        float paddingArea = Math::sqrt( avgArea ) * space_size * smoothGroups; // 占用n像素填充空余，每相邻charts
        //if ( paddingArea > area * 1.5f )
        //    paddingArea = area * 1.5f;

        // 估算真实面积
        float areaReal = area * 3.0f + paddingArea * 3.0f; // 20%扩张 + 填充面积

        // 面积边长
        float sideWidth = Math::sqrt(areaReal); // 边长
        float pixelUnit = _getAutoTexSizeUnit( type ); // 自动大小的比率
        float pixelsWidth = pixelUnit * sideWidth; // 像素边长
        int   texSize = (int)Math::ceil( pixelsWidth ); // 取上整

        // 限制最小最大
        if ( texSize < s_minTexSize )
            texSize = s_minTexSize;
        else if ( texSize > s_maxTexSize )
            texSize = s_maxTexSize;

        return texSize;
    }

    int GeneralMeshUV::_calcSuitableTexSize( int texSize )
    {
        // 生成基本信息
        _generalFaceInfo( true, true );

        // 分成平滑组
        SmoothGroupGather gather;
        gather.gatherFaces( m_ib, m_faceGroups );
        m_sgInfos.resize( m_faceGroups.size() );

        // 调节平滑组拉伸
        for ( int i = 0; i < (int)m_faceGroups.size(); ++i )
        {
            //_adjustSmoothGroupStretch( i );
        }

        // 更新面积信息
        _generalFaceInfo( false, true );

        // 调节平滑组过小面积
        float areaTotalAdj = _calcTotalArea();
        float avgArea = areaTotalAdj / (m_ib.size() / 3);

        for ( int i = 0; i < (int)m_faceGroups.size(); ++i )
        {
            //_adjustSmoothGroupArea( i, avgArea );
        }

        // 获取mesh总面积
        _generalFaceInfo( false, true );
        float areaTotalFinal = _calcTotalArea();

         // 自动大小时，估算面积
        if ( texSize <= 0 )
            texSize = _estimateTexSize( texSize, areaTotalFinal, (int)m_faceGroups.size() );
       
        return texSize;
    }

    void* GeneralMeshUV::_convertD3DMesh() const
    {
        LPDIRECT3DDEVICE9 d3d9Device = static_cast<D3D9RenderDevice*>( g_renderDevice )->_getD3DDevice();

        LPD3DXMESH pMesh = 0;

        if ( FAILED( D3DXCreateMeshFVF( m_ib.size() / 3, m_vb.size(),
                D3DXMESH_32BIT | D3DXMESH_SYSTEMMEM, 
                D3DFVF_XYZB1 | D3DFVF_LASTBETA_UBYTE4 | D3DFVF_NORMAL | D3DFVF_TEX2,
                d3d9Device, &pMesh ) ) )
        {
            return 0;
        }

        // copy vb
        SimpleVertex* pvb = 0;
        pMesh->LockVertexBuffer( 0, (void**)&pvb );

        for ( size_t i = 0; i < m_vb.size(); ++i )
        {
            pvb[i].pos  = m_vb[i].posWorld;
            pvb[i].id   = (int)i;
            pvb[i].norm = m_vb[i].normWorld;
            pvb[i].uv1  = m_vb[i].uv1;
            pvb[i].uv2  = Vector2::ZERO;
        }

        pMesh->UnlockVertexBuffer();

        // copy ib
        int* pib = 0;
        pMesh->LockIndexBuffer( 0, (void**)&pib );
        memcpy( pib, &m_ib[0], sizeof(int) * m_ib.size() );
        pMesh->UnlockIndexBuffer();

        // return
        return pMesh;
    }

    void* GeneralMeshUV::_createUVAtlas( void* d3dMesh, int texSize, void** ppVertexRemapArray )
    {
        float fGutter = 0.0f;
        if ( texSize <= 32 ) // 32解析度以下用4像素间距
            fGutter = 4.0f;
        else // 高解析度用3像素间距
            fGutter = 3.0f;

        const float  fMaxStretch = 0.5f; // 允许最大拉伸比
        const uint32 dwTextureIndex = 1; // 第2套uv

        LPD3DXMESH   srcMesh = (LPD3DXMESH)d3dMesh;
        LPD3DXMESH   destMesh = 0;
        LPD3DXBUFFER pFacePartitioning = 0;
        LPD3DXBUFFER pVertexRemapArray = 0;
        float        fMaxStretchOut = 0;
        UINT         dwNumChartsOut = 0;

        HRESULT hr = D3DXUVAtlasCreate( srcMesh, 0, fMaxStretch, texSize, texSize, fGutter, dwTextureIndex,
            0, 0, 0, 0, 0, 0,
            D3DXUVATLAS_GEODESIC_FAST, &destMesh, &pFacePartitioning, &pVertexRemapArray,
            &fMaxStretchOut, &dwNumChartsOut );

        if ( FAILED(hr) )
            return 0;

        pFacePartitioning->Release();
        *ppVertexRemapArray = pVertexRemapArray;
        return destMesh;
    }

    Mesh* GeneralMeshUV::_buildNewMesh( const String& newMeshName, void* d3dMesh, void* d3dMeshOut, void* pVertexRemapArray ) const
    {
        LPD3DXMESH   srcMesh  = (LPD3DXMESH)d3dMesh;
        LPD3DXMESH   destMesh = (LPD3DXMESH)d3dMeshOut;
        LPD3DXBUFFER vtxRemap = (LPD3DXBUFFER)pVertexRemapArray;

        int32* fromVb = (int32*)vtxRemap->GetBufferPointer();
        
        SimpleVertex* pvb = 0;
        int* pib = 0;

        destMesh->LockVertexBuffer( D3DLOCK_READONLY, (void**)&pvb );
        destMesh->LockIndexBuffer( D3DLOCK_READONLY, (void**)&pib );

        int vbCnt   = (int)destMesh->GetNumVertices();
        int faceCnt = (int)destMesh->GetNumFaces();
        int subCnt  = (int)m_vbMap.size();

        khaosAssert( vtxRemap->GetBufferSize() / sizeof(uint32) == vbCnt );

        VertexPNTTOutListArray newVbs_0(subCnt);
        VertexPNGTTOutListArray newVbs_1(subCnt);
        bool hasTangent = m_mesh->getSubMesh(0)->getVertexBuffer()->hasElement( VFM_TAN ); // 目前要求所有submesh格式一致

        IntListMap newIbs(subCnt);
        IntList newIbMaps(vbCnt);

        for ( int i = 0; i < vbCnt; ++i )
        {
            // ps: 可能2个新的顶点来自同一个旧顶点（被分裂），但反之则不会
            const SimpleVertex& sv = pvb[i]; // 转换后的顶点（带uv2）
            khaosAssert( sv.id == fromVb[i] ); // 来源应当就是旧的id
            const VertexID& oldVid = m_vb[fromVb[i]]; // 转换前旧顶点信息
            const VertexBuffer* buff = m_mesh->getSubMesh(oldVid.subIdx)->getVertexBuffer(); // 该点所在旧缓冲

            VertexPNTT newVtx_0;
            newVtx_0.pos()  = *buff->getCachePos(oldVid.vtxIdx); // 复制旧的pos等
            newVtx_0.norm() = *buff->getCacheNormal(oldVid.vtxIdx);
            newVtx_0.tex()  = *buff->getCacheTex(oldVid.vtxIdx);
            khaosAssert( newVtx_0.tex() == sv.uv1 );

            newVtx_0.tex2() = sv.uv2; // 复制新生成的uv2

            if ( hasTangent ) // 有切线
            {
                VertexPNGTT newVtx_1;
                newVtx_1.pos() = newVtx_0.pos();
                newVtx_1.norm() = newVtx_0.norm();
                newVtx_1.tex() = newVtx_0.tex();
                newVtx_1.tex2() = newVtx_0.tex2();
                newVtx_1.tang() = *buff->getCacheTanget(oldVid.vtxIdx);

                newIbMaps[i] = (int) newVbs_1[oldVid.subIdx].size(); // 新的点在新submesh下索引
                newVbs_1[oldVid.subIdx].push_back( newVtx_1 ); // 新的submesh加入该新顶点
            }
            else // 无切线
            {
                newIbMaps[i] = (int) newVbs_0[oldVid.subIdx].size(); // 新的点在新submesh下索引
                newVbs_0[oldVid.subIdx].push_back( newVtx_0 ); // 新的submesh加入该新顶点
            }
            
        }

        for ( int f = 0; f < faceCnt; ++f )
        {
            int newIdx0 = pib[f*3]; // 新面在新的大集合中索引
            int newIdx1 = pib[f*3+1];
            int newIdx2 = pib[f*3+2];

            const VertexID& oldVid0 = m_vb[fromVb[newIdx0]]; // 找到来源
            const VertexID& oldVid1 = m_vb[fromVb[newIdx1]];
            const VertexID& oldVid2 = m_vb[fromVb[newIdx2]];

            khaosAssert( oldVid0.subIdx == oldVid1.subIdx && oldVid1.subIdx == oldVid2.subIdx ); // 应当出自同一个submesh

            newIbs[oldVid0.subIdx].push_back( newIbMaps[newIdx0] ); // 加入在新submesh下索引
            newIbs[oldVid0.subIdx].push_back( newIbMaps[newIdx1] );
            newIbs[oldVid0.subIdx].push_back( newIbMaps[newIdx2] );
        }

        Mesh* meshNew = 0;
        if ( hasTangent )
             meshNew = _createNewMesh( newMeshName, newVbs_1, newIbs );
        else
            meshNew = _createNewMesh( newMeshName, newVbs_0, newIbs );

        // 
        destMesh->UnlockIndexBuffer();
        destMesh->UnlockVertexBuffer();

        srcMesh->Release();
        destMesh->Release();
        vtxRemap->Release();

        return meshNew;
    }

    template<class VertexOutListArray>
    Mesh* GeneralMeshUV::_createNewMesh( const String& newMeshName, const VertexOutListArray& newVbs, const IntListMap& newIbs ) const
    {
        // 创建新mesh：复制原来的材质等信息，填充新的vb,ib
        Mesh* newMesh = MeshManager::createMesh( newMeshName );
        if ( !newMesh )
            return 0;

        newMesh->setNullCreator();

        for ( int subIdx = 0; subIdx < m_mesh->getSubMeshCount(); ++subIdx )
        {
            SubMesh* sm = newMesh->createSubMesh();
            sm->setPrimitiveType( PT_TRIANGLELIST );
            sm->setMaterialName( m_mesh->getSubMesh(subIdx)->getMaterialName() );

            sm->createVertexBuffer( &newVbs[subIdx][0], (int)newVbs[subIdx].size() );
            sm->createIndexBuffer( &newIbs[subIdx][0], (int)newIbs[subIdx].size() );
        }

        newMesh->updateAABB( true );
        return newMesh;
    }
}

