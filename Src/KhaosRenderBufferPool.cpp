#include "KhaosPreHeaders.h"
#include "KhaosRenderBufferPool.h"
#include "KhaosRenderDevice.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    // RenderBufferPool
    void RenderBufferPool::destroyAllBuffers()
    {
        _destroyBuffers( m_rttCubeMap, true );
        _destroyBuffers( m_rttMap, true );
        _destroyBuffers( m_depthMap, false );
    }

    void RenderBufferPool::resetAllBufers()
    {
        _resetBuffers( m_rttCubeMap );
        _resetBuffers( m_rttMap );
        _resetBuffers( m_depthMap );
    }

    RenderBufferPool::Item* RenderBufferPool::getRTTBufferTemp( PixelFormat fmt, int width, int height, const String* name )
    {
        Item* item = _getRTTBuffer(fmt, width, height);
        if ( name )
            m_itemMap[*name] = item;
        return item;
    }

    TextureObj* RenderBufferPool::createRTTBuffer( PixelFormat fmt, int width, int height )
    {
        _checkWidthHeight( width, height );
        return _createRTTBuffer( fmt, width, height );
    }

    TextureObj* RenderBufferPool::getRTTBuffer( PixelFormat fmt, int width, int height, const String* name )
    {
        return getRTTBufferTemp( fmt, width, height, name )->rtt;
    }

    RenderBufferPool::Item* RenderBufferPool::getRTTCubeBufferTemp( PixelFormat fmt, int size, const String* name )
    {
        Item* item = _getRTTCubeBuffer(fmt, size);
        if ( name )
            m_itemMap[*name] = item;
        return item;
    }

    TextureObj* RenderBufferPool::getRTTCubeBuffer( PixelFormat fmt, int size, const String* name )
    {
        return getRTTCubeBufferTemp( fmt, size, name )->rtt;
    }

    RenderBufferPool::Item* RenderBufferPool::getDepthBufferTemp( int width, int height )
    {
        return _getDepthBuffer(width, height, true);
    }

    RenderBufferPool::Item* RenderBufferPool::_getRTTCubeBuffer( PixelFormat fmt, int size )
    {
        uint32 key = size;
        ItemList& itemList = m_rttCubeMap[key];

        KHAOS_FOR_EACH( ItemList, itemList, it )
        {
            Item* item = *it;
            if ( item->unused )
            {
                item->setUsed();
                return item;
            }
        }

        TextureObj* newRTT = g_renderDevice->createTextureObj();

        TexObjCreateParas paras;
        paras.type   = TEXTYPE_CUBE;
        paras.usage  = TEXUSA_RENDERTARGET;
        paras.format = fmt;
        paras.levels = 1;
        paras.width  = size;
        paras.height = size;

        if ( !newRTT->create( paras ) )
        {
            KHAOS_DELETE newRTT;
            khaosLogLn( KHL_L1, "RenderBufferPool::_getRTTCubeBuffer failed" );
            return 0;
        }

        itemList.push_back( KHAOS_NEW Item(newRTT) );
        itemList.back()->setUsed();
        return itemList.back();
    }

    void RenderBufferPool::_checkWidthHeight( int& width, int& height )
    {
        if ( width == 0 || height == 0 )
        {
            width  = g_renderDevice->getWindowWidth();
            height = g_renderDevice->getWindowHeight();
        }
    }

    TextureObj* RenderBufferPool::_createRTTBuffer( PixelFormat fmt, int width, int height )
    {
        TextureObj* newRTT = g_renderDevice->createTextureObj();

        TexObjCreateParas paras;
        paras.type   = TEXTYPE_2D;
        paras.usage  = TEXUSA_RENDERTARGET;
        paras.format = fmt;
        paras.levels = 1;
        paras.width  = width;
        paras.height = height;

        if ( fmt == PIXFMT_D24S8 )
            paras.usage = TEXUSA_DEPTHSTENCIL;

        if ( !newRTT->create( paras ) )
        {
            KHAOS_DELETE newRTT;
            khaosLogLn( KHL_L1, "RenderBufferPool::_createRTTBuffer failed" );
            return 0;
        }

        return newRTT;
    }

    RenderBufferPool::Item* RenderBufferPool::_getRTTBuffer( PixelFormat fmt, int width, int height )
    {
        _checkWidthHeight( width, height );

        uint64 key = ((uint64)fmt << 32) | (width << 16) | height;
        ItemList& itemList = m_rttMap[key];

        KHAOS_FOR_EACH( ItemList, itemList, it )
        {
            Item* item = *it;
            if ( item->unused )
            {
                item->setUsed();
                return item;
            }
        }

        TextureObj* newRTT = _createRTTBuffer( fmt, width, height );
        if ( !newRTT )
            return 0;

        itemList.push_back( KHAOS_NEW Item(newRTT) );
        itemList.back()->setUsed();
        return itemList.back();
    }

    RenderBufferPool::Item* RenderBufferPool::_getDepthBuffer( int width, int height, bool isTemp )
    {
        _checkWidthHeight( width, height );

        // 临时用可以使用主深度，也许主深度缓存直接满足了
        if ( isTemp && m_mainDepth.unused )
        {
            SurfaceObj* depth = g_renderDevice->getMainDepthStencil()->getSurface(0);
            m_mainDepth.depth = depth;

            if ( g_renderDevice->isDepthAcceptSize( depth->getWidth(), depth->getHeight(), width, height ) )
            {
                m_mainDepth.setUsed();
                return &m_mainDepth;
            }
        }

        // 测试池内是否满足
        KHAOS_FOR_EACH( ItemListOrderMap, m_depthMap, it )
        {
            uint32 wh = it->first;
            int depthW = wh >> 16;
            int depthH = wh & 0xffff;

            if ( g_renderDevice->isDepthAcceptSize( depthW, depthH, width, height ) )
            {
                ItemList& itemList = it->second;
                KHAOS_FOR_EACH( ItemList, itemList, il )
                {
                    Item* item = *il;
                    if ( item->unused )
                    {
                        item->setUsed();
                        return item;
                    }
                }
            }
        }

        // 创建新的
        SurfaceObj* newDepth = g_renderDevice->createSurfaceObj();
        if ( !newDepth->create( width, height, TEXUSA_DEPTHSTENCIL, PIXFMT_D24S8 ) )
        {
            KHAOS_DELETE newDepth;
            khaosLogLn( KHL_L1, "RenderBufferPool::_getDepthBuffer failed" );
            return 0;
        }

        uint32 key = (width << 16) | height;
        ItemList& itemList = m_depthMap[key];
        itemList.push_back( KHAOS_NEW Item(newDepth) );
        itemList.back()->setUsed();
        return itemList.back();
    }

    RenderBufferPool::Item* RenderBufferPool::getItem( const String& name ) const
    {
        ItemMap::const_iterator it = m_itemMap.find( name );
        if ( it != m_itemMap.end() )
            return it->second;
        return 0;
    }

    template<class T>
    void RenderBufferPool::_destroyBuffers( T& items, bool rtt )
    {
        for ( typename T::iterator it = items.begin(), ite = items.end(); it != ite; ++it )
        {
            _destroyBuffers( it->second, rtt );
        }

        items.clear();
    }

    template<class T>
    void RenderBufferPool::_resetBuffers( T& items )
    {
        for ( typename T::iterator it = items.begin(), ite = items.end(); it != ite; ++it )
        {
            _resetBuffers( it->second );
        }
    }

    void RenderBufferPool::_destroyBuffers( ItemList& items, bool rtt )
    {
        KHAOS_FOR_EACH( ItemList, items, il )
        {
            Item* item = *il;

            if ( rtt )
                KHAOS_DELETE item->rtt;
            else
                KHAOS_DELETE item->depth;

            KHAOS_DELETE item;
        }

        items.clear();
    }

    void RenderBufferPool::_resetBuffers( ItemList& items )
    {
        KHAOS_FOR_EACH( ItemList, items, il )
        {
            Item* item = *il;
            item->setUnused();
        }
    }
}

