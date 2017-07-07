#include "KhaosPreHeaders.h"
#include "KhaosLightmapUtil.h"
#include "KhaosRenderDevice.h"
#include "KhaosTextureObj.h"
#include "KhaosBinStream.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    LightMapProcess::LightMapProcess( Mesh* mesh, const Matrix4& matWorld, int texWidth, int texHeight ) :
        m_mesh(mesh), m_matWorld(matWorld), m_texWidth(texWidth), m_texHeight(texHeight),
        m_currSubMesh(0), m_currVB(0), m_currIB(0),
        m_event(0),
        m_needCalc(true), m_needTangent(false), m_needFaceTangent(false)
    {
    }

    void LightMapProcess::general( ILightMapProcessCallback* ev, int threads )
    {
        m_event = ev;
        
        m_texels.resize( m_texWidth * m_texHeight );

        m_mesh->cacheLocalData( false );

        int subMeshCnt = m_mesh->getSubMeshCount();

        for ( int subIdx = 0; subIdx < subMeshCnt; ++subIdx )
        {
            m_currSubMesh = m_mesh->getSubMesh(subIdx);

            // save current pos buffer in world space
            m_currVB = m_currSubMesh->getVertexBuffer();
            int vtxCnt = m_currVB->getVertexCount();

            m_currVBPos.resize( vtxCnt );
            m_currVBNormal.resize( vtxCnt );

            if ( m_needTangent )
                m_currVBTangent.resize( vtxCnt );

            if ( m_needCalc ) // 需要计算，这里等于执行vs
            {
                for ( int v = 0; v < vtxCnt; ++v )
                {
                    m_currVBPos[v]    = m_matWorld.transformAffine( *m_currVB->getCachePos(v) );
                    // vs里面不进行normalised，在ps中去做
                    m_currVBNormal[v] = m_matWorld.transformAffineNormal( *m_currVB->getCacheNormal(v) );//.normalisedCopy(); 

                    if ( m_needTangent ) // 需要切线
                    {
                        m_currVBNormal[v].normalise();
                        m_currVBTangent[v].asVec3() = m_matWorld.transformAffineNormal( m_currVB->getCacheTanget(v)->asVec3() ).normalisedCopy(); 
                        m_currVBTangent[v].w = m_currVB->getCacheTanget(v)->w;
                    }
                }
            }

            // for each face
            m_currIB = m_currSubMesh->getIndexBuffer();
            int primCnt = m_currSubMesh->getPrimitiveCount();

            m_event->onPerSubMeshBegin( m_currSubMesh, subIdx );

            BatchTasks tasks;
            tasks.setTask( _generalFaceStatic );
            tasks.setUserData( this );
            tasks.planTask( primCnt, threads );

            m_event->onPerSubMeshEnd( m_currSubMesh );
        }
    }

    void LightMapProcess::_generalFaceStatic( int threadID, void* para, int f )
    {
        LightMapProcess* lmp = (LightMapProcess*)para;
        lmp->_generalFace( threadID, f );
    }

    void LightMapProcess::_generalFace( int threadID, int f )
    {
        // face's index
        int i0, i1, i2;
        m_currIB->getCacheTriIndices( f, i0, i1, i2 );

        // pos0 .. pos2
        const Vector3& pos0 = m_currVBPos[i0];
        const Vector3& pos1 = m_currVBPos[i1];
        const Vector3& pos2 = m_currVBPos[i2];

        // normal0 .. normal2
        const Vector3& norm0 = m_currVBNormal[i0];
        const Vector3& norm1 = m_currVBNormal[i1];
        const Vector3& norm2 = m_currVBNormal[i2];

        // suv0 .. suv2
        const Vector2& stex0 = *m_currVB->getCacheTex(i0);
        const Vector2& stex1 = *m_currVB->getCacheTex(i1);
        const Vector2& stex2 = *m_currVB->getCacheTex(i2);

        // uv0 .. uv2
        Vector2 tex0 =  *m_currVB->getCacheTex2(i0);
        Vector2 tex1 =  *m_currVB->getCacheTex2(i1);
        Vector2 tex2 =  *m_currVB->getCacheTex2(i2);

        Vector3 faceTangent, faceBinormal, faceNormal;
        
        if ( m_needFaceTangent )
        {
            faceNormal = Math::calcNormal( pos0, pos1, pos2 );

            // 暂时不需要这些信息
            //Math::calcTangent( pos0, pos1, pos2, tex0, tex1, tex2, faceTangent, faceBinormal );

            //Vector4 t = Math::gramSchmidtOrthogonalize( faceTangent, faceBinormal, faceNormal ); 
            //faceTangent = t.asVec3() * t.w;

            //faceBinormal = faceNormal.crossProduct(faceTangent);
        }

        m_event->onPerFaceBegin( threadID, f, faceTangent, faceBinormal, faceNormal );

        // 计算uv的包围盒
        Vector2 minUV, maxUV;
        _calcUVAABB( tex0, tex1, tex2, minUV, maxUV );

        // 得到uv对应的像素范围
        IntVector2 minPix, maxPix;
        _convertPixelAABB( minUV, maxUV, minPix, maxPix );

        // 转到像素级别（计算精度更好）
        tex0 = _uv2pxy(tex0);
        tex1 = _uv2pxy(tex1);
        tex2 = _uv2pxy(tex2);

        // 枚举这片区域
        for ( int yp = minPix.y; yp <= maxPix.y; ++yp )
        {
            for ( int xp = minPix.x; xp <= maxPix.x; ++xp )
            {
                // 得到重心坐标，且在三角形内
                Vector3 uvwG;

                if ( _canDoCalc( threadID, xp, yp, tex0, tex1, tex2, uvwG ) )
                {
                    if ( m_needCalc ) // 插值计算的通知
                    {
                        // 得到插值的位置，法线, 1套纹理坐标
                        Vector3 pos  = uvwG.x * pos0  + uvwG.y * pos1  + uvwG.z * pos2;
                        Vector3 norm = uvwG.x * norm0 + uvwG.y * norm1 + uvwG.z * norm2;
                        Vector2 uv   = uvwG.x * stex0 + uvwG.y * stex1 + uvwG.z * stex2;

                        norm.normalise();

                        // 切线
                        Vector4 tang;
                        const Vector4* tang_ptr = &Vector4::ZERO;

                        if ( m_needTangent )
                        {
                            // tangent0 ... tangent2
                            const Vector4& tang0 = m_currVBTangent[i0];
                            const Vector4& tang1 = m_currVBTangent[i1];
                            const Vector4& tang2 = m_currVBTangent[i2];

                            tang = uvwG.x * tang0 + uvwG.y * tang1 + uvwG.z * tang2;
                            tang_ptr = &tang;
                        }

                        // 通知计算
                        m_event->onPerTexel( threadID, xp, yp, pos, norm, *tang_ptr, uv );
                    }
                    else // 无计算的通知
                    {
                        m_event->onPerTexel( threadID, xp, yp, Vector3::ZERO, Vector3::ZERO, Vector4::ZERO, Vector2::ZERO );
                    }
                }
            } // end for xp
        } // end for yp
    }

    bool LightMapProcess::_canDoCalc( int threadID, int xp, int yp, 
        const Vector2& tex0, const Vector2& tex1, const Vector2& tex2, 
        Vector3& uvwG ) 
    {
        // 用户选择像素机会
        int discardRet = m_event->onDiscardTexel( threadID, xp, yp );

        if ( discardRet == 0 ) // 用户放弃此像素
            return false;

        // 像素中心的uv坐标
        Vector2 uvCenter = _xy2Center( IntVector2(xp, yp) );

        // 不在三角形内，忽略
        if ( !Math::calcGravityCoord(tex0, tex1, tex2, uvCenter, &uvwG) )
        {
            khaosAssert( discardRet != 1 ); // 用户说有？
            return false;
        }

        // 注册此像素
        int texel_idx = yp * m_texWidth + xp;
        TexelInfo& ti = m_texels[texel_idx];

        bool canDo = false;

        Khaos::LockGuard guard_(m_mtxTexInfos); // texel info允许多线程

        if ( !ti.used ) // 空白的能写
        {
            ti.uvwG = uvwG;
            ti.err = _getUVWGLenErr( uvwG );
            ti.used = true;

            canDo = true;
        }
        else // 已经有了
        {
            Vector2 v_err = _getUVWGLenErr( uvwG );

            if ( _isUVWLenErrLess(v_err, ti.err) ) // 当前误差更小则覆盖
            {
                ti.uvwG = uvwG;
                ti.err = v_err;

                canDo = true;
            }
            else
            {
                khaosAssert( discardRet != 1 );
            }
        }

        if ( canDo ) // 可以注册了，这里能保证顺序和原子性
        {
            m_event->onRegisterTexel( threadID, xp, yp );
        }

        return canDo;
    }

    Vector2 LightMapProcess::_getUVWGLenErr( const Vector3& uvwG ) const
    {
        // 计算uvw重心坐标的误差系数
        float vmin = uvwG.x;
        float vmax = uvwG.x;

        for ( int i = 1; i < 3; ++i )
        {
            float v_curr = uvwG[i];

            if ( v_curr < vmin )
                vmin = v_curr;
            else if ( v_curr > vmax )
                vmax = v_curr;
        }

        return Vector2(vmin, vmax);
    }

    bool LightMapProcess::_isUVWLenErrLess( const Vector2& errA, const Vector2& errB ) const
    {
        // 首先看最小值，大的优先
        float errMin = Math::fabs( errA.x - errB.x );
        if ( errMin > 1e-4f ) // 不相等
        {
            return errA.x > errB.x;
        }

        // 最小相等，比较最大值，小的优先
        //khaosAssert( Math::fabs( errA.y - errB.y ) > 1e-4f );
        return errA.y < errB.y;
    }

    void LightMapProcess::_calcUVAABB( const Vector2& tex0, const Vector2& tex1, const Vector2& tex2,
        Vector2& minUV, Vector2& maxUV ) const
    {
        minUV = tex0;
        maxUV = tex0;

        minUV.makeFloor(tex1);
        maxUV.makeCeil(tex1);

        minUV.makeFloor(tex2);
        maxUV.makeCeil(tex2);
    }

    void LightMapProcess::_convertPixelAABB( const Vector2& minUV, const Vector2& maxUV,
        IntVector2& minPix, IntVector2& maxPix )
    {
        Vector2 minUVPix = _uv2pxy(minUV);
        Vector2 maxUVPix = _uv2pxy(maxUV);

        minPix.x = (int) Math::floor( minUVPix.x );
        minPix.y = (int) Math::floor( minUVPix.y );

        maxPix.x = (int) Math::ceil( maxUVPix.x );
        maxPix.y = (int) Math::ceil( maxUVPix.y );

        khaosAssert( minPix.x >= 0 && minPix.y >= 0 );
        khaosAssert( maxPix.x <= m_texWidth && maxPix.y <= m_texHeight );

        if ( maxPix.x >= m_texWidth )
            maxPix.x = m_texWidth -1;

        if ( maxPix.y >= m_texHeight )
            maxPix.y = m_texHeight - 1;
    }

    Vector2 LightMapProcess::_xy2Center( const IntVector2& xy ) const
    {
        // 像素转纹理坐标
        // 像素是个块，所以第x个块中心在(x, y) + (0.5, 0.5)
        return Vector2( xy.x + 0.5f, xy.y + 0.5f );
    }

    Vector2 LightMapProcess::_uv2pxy( const Vector2& uv ) const
    {
        // 纹理坐标放大到像素级别，注意这里不用-0.5
        return Vector2( uv.x * m_texWidth, uv.y * m_texHeight );
    }
}

