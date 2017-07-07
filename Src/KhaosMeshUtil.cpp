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
        
        map<int, int>::type vtxGroupMap; // �������� -> ƽ����

        int faceCnt = (int)ib.size() / 3;

        for ( int f = 0; f < faceCnt; ++f )
        {
            int offset = f * 3;

            // ������������������
            int vi[3] = { ib[offset], ib[offset+1], ib[offset+2] };

            // �ж������Ƿ��Ѿ����ڷ��ƽ����
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

            // ���û�����Ǹ���ƽ����
            if ( groupId == -1 )
            {
                groupId = (int)groups.size();
                groups.push_back( IntList() );
            }

            // ������������ƽ����
            for ( int i = 0; i < 3; ++i )
                vtxGroupMap[ vi[i] ] = groupId;

            // ����������ƽ����
            groups[groupId].push_back( f );
        }
    }

    void SmoothGroupGather::gatherVertices( const IntList& ib, const IntList& faces, IntSet& vertices )
    {
        // ͳ��һ��ƽ����ʹ�õ��Ķ��㼯��
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
        // �õ�һ��ƽ����ıߵ�ʹ�����
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

                if ( vb->hasElement( VFM_TAN ) ) // ������
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

            for ( int f = 0; f < faceCnt; ++f ) // ÿ���������������������б�
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
        // �����������Ϣ
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
        // ͳ��mesh���е������
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
        // �ҵ����ƽ����ƽ������
        avgNorm = Vector3::ZERO;

        int faceCnt = (int)faceGroup.size();

        for ( int g = 0; g < faceCnt; ++g )
        {
            int f = faceGroup[g];
            avgNorm += m_faceInfo[f].norm;
        }

        avgNorm.normalise();

        // �����Ƿ�һ��
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
        // �ҵ����ƽ����ƽ������
        if ( !_calcSmoothGroupAvgNormal(faceGroup, avgNorm) )
            return false;

        // �õ�����Ϣ
        SmoothGroupGather gather;
        SmoothGroupGather::SideMap sides;
        gather.gatherSides( m_ib, faceGroup, sides );

        // �ҵ����ı�
        float sideMax = 0;
        int   faceIdx = -1;

        KHAOS_FOR_EACH( SmoothGroupGather::SideMap, sides, it )
        {
            int usedCnt = it->second;
            if ( usedCnt != 1 ) // ֻ��1�������ߣ���������
                continue;

            const SmoothGroupGather::Side& side = it->first;

            const Vector3& posA = m_vb[side.v0].posWorld;
            const Vector3& posB = m_vb[side.v1].posWorld;

            Vector3 posAB = posB - posA;
            float   lenAB = posAB.length();

            if ( lenAB > sideMax ) // ��¼���ı�
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
        // ��������ľ���

         // ������ı�׼��
        Vector3 axisX = vecMax.normalisedCopy();
        Vector3 axisY = avgNorm;
        Vector3 axisZ = axisX.crossProduct( axisY );

        // ת������ռ��ڣ��õ�aabb
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

        // �ж�x,z�����Ƿ�������
        lenX = rangeX.getRange();
        lenZ = rangeZ.getRange();
        float lenXPerZ = lenX / lenZ;

        const float maxMultiScale = 5;

        if ( lenXPerZ > maxMultiScale ) // x���򳤶ȣ������z����n��
        {
            // ��������z������
            float scaleZ = lenXPerZ / maxMultiScale;
            lenZ *= scaleZ; // ���շ��ص������lenZ

            Matrix3 matRotFace;
            matRotFace.fromAxes( axisX, axisY, axisZ );
            Matrix4 matRotFace4(matRotFace);

            Matrix4 matScale(Matrix4::IDENTITY);
            matScale.setScale( Vector3(1, 1, scaleZ) );

            matAdj = matRotFace4 * matScale * matRotFace4.inverseAffine(); // �ȱ任����ռ䣨�����棩��Ȼ��Ŵ����ԭ
            return true;
        }

        return false;
    }

    void GeneralMeshUV::_transformSmoothGroup( const SmoothGroupGather::IntSet& vertices, const Matrix4& mat )
    {
        // �任���е�
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

        // �ҵ�����
        groupInfo.isFlat = _findSmoothGroupStretchVector( faceGroup, groupInfo.vecMax, groupInfo.avgNorm );
        if ( !groupInfo.isFlat )
            return;

        // �õ����е�
        SmoothGroupGather gather;
        gather.gatherVertices( m_ib, faceGroup, groupInfo.vertices );

        // ��ȡ��������
        Matrix4 matAdj;
        float lenX, lenZ;
        bool needAdj = _getMagnifyMatrix( groupInfo.vertices, groupInfo.vecMax, groupInfo.avgNorm, matAdj, lenX, lenZ );
        groupInfo.areaBox = lenX * lenZ; // ��¼�����Χ��С
        if ( !needAdj )
            return;

        // ����
        _transformSmoothGroup( groupInfo.vertices, matAdj );
    }

    void GeneralMeshUV::_adjustSmoothGroupArea( int i, float avgArea )
    {
        const SmoothGroupGather::IntList& faceGroup = m_faceGroups[i];
        SmoothGroupInfo& groupInfo = m_sgInfos[i];

        // ֻ����ƽ��
        if ( !groupInfo.isFlat )
            return;

        // ֻ��������
        int faceCnt = (int)faceGroup.size();
        //if ( faceCnt > 2 )
        //    return;

        // ƽ������ʵ���
        float totAreaRealCurr = 0;
        for ( int g = 0; g < faceCnt; ++g )
        {
            int f = faceGroup[g];
            totAreaRealCurr += m_faceInfo[f].area;
        }

        // �Ƿ����
        float maxArea = Math::maxVal( totAreaRealCurr, groupInfo.areaBox );
        if ( maxArea < 1e-6f )
            return;

        float esVal = Math::fabs(groupInfo.areaBox - totAreaRealCurr) / maxArea;
        if ( esVal > 0.2f ) // ����20%������
            return;

        // ƽ����ƽ�����
        float avgAreaCurr = maxArea / faceCnt;

        // ������С��������
        const float migMultiScale = 4;
        const float magMultiScale = 4;
        float needScale = 1.0f;

        if ( (avgArea / avgAreaCurr) > migMultiScale ) // ��ǰ��С
        {
            // ��Ҫ�Ŵ�ı���
            needScale = (avgArea / avgAreaCurr) / migMultiScale;
            needScale = Math::sqrt( needScale );
            //if ( needScale > 16 )
            //    needScale = 16;
        }
        else if ( (avgAreaCurr / avgArea) > magMultiScale ) // ��ǰ����
        {
            // ��Ҫ��С�ı���
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

        // ����
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

        // ����padding���
        float avgArea = area / smoothGroups;
        float paddingArea = Math::sqrt( avgArea ) * space_size * smoothGroups; // ռ��n���������࣬ÿ����charts
        //if ( paddingArea > area * 1.5f )
        //    paddingArea = area * 1.5f;

        // ������ʵ���
        float areaReal = area * 3.0f + paddingArea * 3.0f; // 20%���� + ������

        // ����߳�
        float sideWidth = Math::sqrt(areaReal); // �߳�
        float pixelUnit = _getAutoTexSizeUnit( type ); // �Զ���С�ı���
        float pixelsWidth = pixelUnit * sideWidth; // ���ر߳�
        int   texSize = (int)Math::ceil( pixelsWidth ); // ȡ����

        // ������С���
        if ( texSize < s_minTexSize )
            texSize = s_minTexSize;
        else if ( texSize > s_maxTexSize )
            texSize = s_maxTexSize;

        return texSize;
    }

    int GeneralMeshUV::_calcSuitableTexSize( int texSize )
    {
        // ���ɻ�����Ϣ
        _generalFaceInfo( true, true );

        // �ֳ�ƽ����
        SmoothGroupGather gather;
        gather.gatherFaces( m_ib, m_faceGroups );
        m_sgInfos.resize( m_faceGroups.size() );

        // ����ƽ��������
        for ( int i = 0; i < (int)m_faceGroups.size(); ++i )
        {
            //_adjustSmoothGroupStretch( i );
        }

        // ���������Ϣ
        _generalFaceInfo( false, true );

        // ����ƽ�����С���
        float areaTotalAdj = _calcTotalArea();
        float avgArea = areaTotalAdj / (m_ib.size() / 3);

        for ( int i = 0; i < (int)m_faceGroups.size(); ++i )
        {
            //_adjustSmoothGroupArea( i, avgArea );
        }

        // ��ȡmesh�����
        _generalFaceInfo( false, true );
        float areaTotalFinal = _calcTotalArea();

         // �Զ���Сʱ���������
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
        if ( texSize <= 32 ) // 32������������4���ؼ��
            fGutter = 4.0f;
        else // �߽�������3���ؼ��
            fGutter = 3.0f;

        const float  fMaxStretch = 0.5f; // ������������
        const uint32 dwTextureIndex = 1; // ��2��uv

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
        bool hasTangent = m_mesh->getSubMesh(0)->getVertexBuffer()->hasElement( VFM_TAN ); // ĿǰҪ������submesh��ʽһ��

        IntListMap newIbs(subCnt);
        IntList newIbMaps(vbCnt);

        for ( int i = 0; i < vbCnt; ++i )
        {
            // ps: ����2���µĶ�������ͬһ���ɶ��㣨�����ѣ�������֮�򲻻�
            const SimpleVertex& sv = pvb[i]; // ת����Ķ��㣨��uv2��
            khaosAssert( sv.id == fromVb[i] ); // ��ԴӦ�����Ǿɵ�id
            const VertexID& oldVid = m_vb[fromVb[i]]; // ת��ǰ�ɶ�����Ϣ
            const VertexBuffer* buff = m_mesh->getSubMesh(oldVid.subIdx)->getVertexBuffer(); // �õ����ھɻ���

            VertexPNTT newVtx_0;
            newVtx_0.pos()  = *buff->getCachePos(oldVid.vtxIdx); // ���ƾɵ�pos��
            newVtx_0.norm() = *buff->getCacheNormal(oldVid.vtxIdx);
            newVtx_0.tex()  = *buff->getCacheTex(oldVid.vtxIdx);
            khaosAssert( newVtx_0.tex() == sv.uv1 );

            newVtx_0.tex2() = sv.uv2; // ���������ɵ�uv2

            if ( hasTangent ) // ������
            {
                VertexPNGTT newVtx_1;
                newVtx_1.pos() = newVtx_0.pos();
                newVtx_1.norm() = newVtx_0.norm();
                newVtx_1.tex() = newVtx_0.tex();
                newVtx_1.tex2() = newVtx_0.tex2();
                newVtx_1.tang() = *buff->getCacheTanget(oldVid.vtxIdx);

                newIbMaps[i] = (int) newVbs_1[oldVid.subIdx].size(); // �µĵ�����submesh������
                newVbs_1[oldVid.subIdx].push_back( newVtx_1 ); // �µ�submesh������¶���
            }
            else // ������
            {
                newIbMaps[i] = (int) newVbs_0[oldVid.subIdx].size(); // �µĵ�����submesh������
                newVbs_0[oldVid.subIdx].push_back( newVtx_0 ); // �µ�submesh������¶���
            }
            
        }

        for ( int f = 0; f < faceCnt; ++f )
        {
            int newIdx0 = pib[f*3]; // �������µĴ󼯺�������
            int newIdx1 = pib[f*3+1];
            int newIdx2 = pib[f*3+2];

            const VertexID& oldVid0 = m_vb[fromVb[newIdx0]]; // �ҵ���Դ
            const VertexID& oldVid1 = m_vb[fromVb[newIdx1]];
            const VertexID& oldVid2 = m_vb[fromVb[newIdx2]];

            khaosAssert( oldVid0.subIdx == oldVid1.subIdx && oldVid1.subIdx == oldVid2.subIdx ); // Ӧ������ͬһ��submesh

            newIbs[oldVid0.subIdx].push_back( newIbMaps[newIdx0] ); // ��������submesh������
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
        // ������mesh������ԭ���Ĳ��ʵ���Ϣ������µ�vb,ib
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

