#include "KhaosPreHeaders.h"
#include "KhaosLightmapUtil.h"
#include "KhaosRenderDevice.h"
#include "KhaosTextureObj.h"
#include "KhaosBinStream.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    void LightMapPostBase::_init( ILightMapSource* lmSrc )
    {
        m_source = lmSrc;
        m_width = lmSrc->getWidth();
        m_height = lmSrc->getHeight();
        m_layerCnt = lmSrc->getLayerCount();
    }

    bool LightMapPostBase::_hasVal( int x, int y ) const
    {
        if ( x < 0 || x >= m_width ||
            y < 0 || y >= m_height )
            return false;

        return m_source->isUsed(x, y);
    }

    void LightMapPostBase::_copyTexelList( const TexelList& texList )
    {
        for ( size_t i = 0; i < texList.size(); ++i )
        {
            const Texel& tex = texList[i];

            for ( size_t lv = 0; lv < tex.vals.size(); ++lv )
                m_source->setVal( lv, tex.x, tex.y, tex.vals[lv] );
        }
    }


    //////////////////////////////////////////////////////////////////////////
    void LightMapRemoveZero::filter( ILightMapSource* lmSrc, FUNC func, float blackVal, float deltaVal, bool useSingle )
    {
        _init( lmSrc );
        m_func = func;
        m_blackVal = blackVal;
        m_deltaVal = deltaVal;
        m_useSingle = useSingle;

        for ( int y = 0; y < m_height; ++y )
        {
            for ( int x = 0; x < m_width; ++x )
            {
                if ( m_useSingle )
                    _checkZero(x, y);
                else
                    _checkEdge(x, y);
            }
        }

        if ( !m_useSingle )
            _blurEdge();

        _copyTexelList( m_tempData );
    }

    bool LightMapRemoveZero::_isBlackPoint( const Vector3& val ) const
    {
        if ( val.x <= m_blackVal && val.y <= m_blackVal && val.z <= m_blackVal )
            return true;

        return false;
    }

    bool LightMapRemoveZero::_isLargeSelf( const Vector3& neighRef, const Vector3& selfRef ) const
    {
        for ( int i = 0; i < 3; ++i )
        {
            if ( neighRef[i] - selfRef[i] > m_deltaVal )
                return true;
        }

        return false;
    }

    void LightMapRemoveZero::_checkZero( int x, int y )
    {
        // �޲�ʹ�õĵ�
        if ( !m_source->isUsed(x, y) )
            return;

        // �����ǰ���
        Vector3 selfRef = m_func( this, m_source, x, y ); // �Լ��Ĳο�ֵ
        if ( !_isBlackPoint(selfRef) )
            return;

        // �����ܱ�
        int offset[8][2] =
        {
            {x-1, y-1},  {x, y-1}, {x+1, y-1},
            {x-1, y},              {x+1, y},
            {x-1, y+1},  {x, y+1}, {x+1, y+1}
        };

        float weights[8] =
        {
            1, 2, 1,
            2,    2,
            1, 2, 1
        };

        float weights_total = 0;
        float total[256] = {};
        int neighCnt = 0;

        for ( int i = 0; i < 8; ++i )
        {
            int px = offset[i][0];
            int py = offset[i][1];

            if ( _hasVal(px, py) ) // �ܱ��е�
            {
                Vector3 neighRef = m_func( this, m_source, px, py ); // �ܱߵĲο�ֵ

                if ( _isLargeSelf(neighRef, selfRef) ) // �����Լ�
                {
                    for ( int lv = 0; lv < m_layerCnt; ++lv ) // �ۼ����в����ֵ
                    {
                        total[lv] += m_source->getVal( lv, px, py ) * weights[i];
                    }

                    weights_total += weights[i];
                    ++neighCnt;
                }
            }
        }

        // �б��Ҵ�ܶ�ĵ㣬ִ���滻
        if ( neighCnt > 0 )
        {
            Texel texel;

            texel.x = x;
            texel.y = y;

#if 0
            texel.vals.push_back( 1.0f );
            texel.vals.push_back( 0.0f );
            texel.vals.push_back( 1.0f );
            texel.vals.push_back( 0.0f );
#else
            for ( int lv = 0; lv < m_layerCnt; ++lv )
            {
                float newval = total[lv] / weights_total;
                //newval = Math::lerp( newval, m_source->getVal(lv, x, y), 0.5f );
                texel.vals.push_back( newval );
            }
#endif
            m_tempData.push_back( texel );
        }
    }

    void LightMapRemoveZero::_checkEdge( int x, int y )
    {
        // �޲�ʹ�õĵ�
        if ( !m_source->isUsed(x, y) )
            return;

        // �����ǰ���
        Vector3 selfRef = m_func( this, m_source, x, y ); // �Լ��Ĳο�ֵ
        if ( !_isBlackPoint(selfRef) )
            return;

        // �����ܱ�
        int offset[8][2] =
        {
            {x-1, y-1},  {x, y-1}, {x+1, y-1},
            {x-1, y},              {x+1, y},
            {x-1, y+1},  {x, y+1}, {x+1, y+1}
        };

        bool findBlack = false;

        for ( int i = 0; i < 8; ++i )
        {
            int px = offset[i][0];
            int py = offset[i][1];

            if ( _hasVal(px, py) ) // �ܱ��е�
            {
                Vector3 neighRef = m_func( this, m_source, px, py ); // �ܱߵĲο�ֵ

                if ( _isLargeSelf(neighRef, selfRef) ) // �����Լ�
                {
                    // �Ǹ�����
                    // ��¼�ܱߵĵ�
                    m_edgePoints.insert( Position(px, py) );
                    findBlack = true;
                }
            }
        }

        if ( findBlack )
            m_edgePoints.insert( Position(x, y) );
    }

    void LightMapRemoveZero::_blurEdge()
    {
        KHAOS_FOR_EACH( PositionSet, m_edgePoints, it )
        {
            const Position& pos = *it;
            _blurPos( pos.x, pos.y );
        }
    }

    void LightMapRemoveZero::_blurPos( int x, int y )
    {
        int offset[9][2] =
        {
            {x-1, y-1},  {x, y-1}, {x+1, y-1},
            {x-1, y},    {x, y},   {x+1, y},
            {x-1, y+1},  {x, y+1}, {x+1, y+1}
        };

        float weights[9] =
        {
            1, 2, 1,
            2, 4, 2,
            1, 2, 1
        };

        float weights_total = 0;
        float total[256] = {};
        int neighCnt = 0;

        for ( int i = 0; i < 8; ++i )
        {
            int px = offset[i][0];
            int py = offset[i][1];

            if ( _hasVal(px, py) ) // �ܱ��е�
            {
                for ( int lv = 0; lv < m_layerCnt; ++lv ) // �ۼ����в����ֵ
                {
                    total[lv] += m_source->getVal( lv, px, py ) * weights[i];
                }

                weights_total += weights[i];
                ++neighCnt;
            }
        }

        // ִ���滻
        Texel texel;

        texel.x = x;
        texel.y = y;

        for ( int lv = 0; lv < m_layerCnt; ++lv )
        {
            float newval = total[lv] / weights_total;
            texel.vals.push_back( newval );
        }

        m_tempData.push_back( texel );
    }


    //////////////////////////////////////////////////////////////////////////
    void LightMapPostFill::fill( ILightMapSource* lmSrc, int times )
    {
        _init( lmSrc );

        khaosAssert( m_layerCnt <= 256 );
        float neigVal[256] = {};

        for ( int t = 0; t < times; ++t ) // ִ��2���������
        {
            // һ��
            vector<int>::type usedListX, usedListY;

            for ( int y = 0; y < m_height; ++y )
            {
                for ( int x = 0; x < m_width; ++x )
                {
                    if ( _getNeighborVal(x, y, neigVal) ) // �Ǹ��ڽӵĿյ�
                    {
                        for ( int l = 0/*1*/; l < m_layerCnt; ++l ) // ������в�
                            lmSrc->setVal( l, x, y, neigVal[l] /*(l == m_layerCnt-1) ? 1.0f : 0.0f*/ );

                        //lmSrc->setVal( 1, x, y, 0.0f );
                        //lmSrc->setVal( 2, x, y, 0.0f );
                        //lmSrc->setVal( 3, x, y, 1.0f );

                        usedListX.push_back( x );
                        usedListY.push_back( y );
                    }
                }
            }

            // ��ɺ����ñ��
            for ( size_t i = 0; i < usedListX.size(); ++i )
            {
                lmSrc->setUsed( usedListX[i], usedListY[i], true ); // ���ʹ��
            }
        }        
    }

    bool LightMapPostFill::_getNeighborVal( int x, int y, float* val ) const
    {
        // �Ҹ��Լ��յ�
        if ( m_source->isUsed( x, y ) )
            return false;

        for ( int i = 0; i < m_layerCnt; ++i )
            val[i] = 0;

        int num = 0;

        // �ܱ���������һ��
        int offset[4][2] =
        {
            x,     y - 1,
            x,     y + 1,
            x - 1, y,
            x + 1, y
        };

        for ( int i = 0; i < 4; ++i )
        {
            int xc = offset[i][0];
            int yc = offset[i][1];

            if ( _hasVal(xc, yc) ) // �ܱ��е�
            {
                for ( int i = 0; i < m_layerCnt; ++i ) // ÿ�������
                    val[i] += m_source->getVal( i, xc, yc );

                ++num;
            }
        }

        if ( num == 0 ) // �����ڽӵ�
            return false;

        // �Ǹ��ڽӵ㣬�ܱ��ĵ���ƽ��
        float s = 1.0f / (float)num;
        for ( int i = 0; i < m_layerCnt; ++i ) // ÿ������ƽ��
            val[i] *= s;

        return true;
    }


    //////////////////////////////////////////////////////////////////////////
    void LightMapPostBlur::filter( ILightMapSource* lmSrc, ILightMapSource* lmDest )
    {
        m_source = lmSrc;

#define _defw(i) (i/16.0f)

        const float weights[] =
        {
            _defw(1), _defw(2), _defw(1),
            _defw(2), _defw(4), _defw(2),
            _defw(1), _defw(2), _defw(1)
        };

        const int offsets[][2] =
        {
            {-1,-1}, {0,-1}, {1,-1},
            {-1, 0}, {0, 0}, {1, 0},
            {-1, 1}, {0, 1}, {1, 1}
        };

        for ( int y = 0; y < lmSrc->getHeight(); ++y )
        {
            for ( int x = 0; x < lmSrc->getWidth(); ++x )
            {
                for ( int l = 0; l < lmSrc->getLayerCount(); ++l )
                {
                    // ģ��
                    float tot = 0;

                    for ( int k = 0; k < KHAOS_ARRAY_SIZE(weights); ++k )
                    {
                        int   cx = x + offsets[k][0];
                        int   cy = y + offsets[k][1];
                        float w  = weights[k];

                        tot += _getVal(l, cx, cy) * w;
                    }

                    lmDest->setVal( l, x, y, tot );
                } // end of layer
            } // end of x
        } // end of y
    }

    float LightMapPostBlur::_getVal( int layer, int x, int y ) const
    {
        if ( x < 0 || x >= m_source->getWidth() ||
            y < 0 || y >= m_source->getHeight() )
            return 0.0f;

        return m_source->getVal( layer, x, y );
    }
}

