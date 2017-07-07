#pragma once
#include "KhaosStdTypes.h"

namespace Khaos
{
    class TextureObj;
    class SurfaceObj;
    enum  PixelFormat;

    //////////////////////////////////////////////////////////////////////////
    // RenderBufferPool
    class RenderBufferPool
    {
    public:
        struct Item : public AllocatedObject
        {
        public:
            Item() : rtt(0), unused(true) {}
            explicit Item( TextureObj* tex ) : rtt(tex), unused(true) {}
            explicit Item( SurfaceObj* dep ) : depth(dep), unused(true) {}

        public:
            void setUsed()   { unused = false; }
            void setUnused() { unused = true; }
            
        public:
            union
            {
                TextureObj* rtt;
                SurfaceObj* depth;
            };

            bool unused;
        };

        struct ItemTemp
        {
            ItemTemp( Item* i = 0 ) : item(i) {}
            ~ItemTemp() { clear(); }

            void swap( ItemTemp& rhs )
            {
                swapVal( this->item, rhs.item );
            }

            void attach( Item* i )
            {
                khaosAssert(!item);
                item = i;
            }

            void clear()
            {
                if ( item ) 
                {
                    item->setUnused();
                    item = 0;
                }
            }

            Item* operator->() const { return item; }
            operator TextureObj*() const { return item->rtt; }
            operator SurfaceObj*() const { return item->depth; }

        private:
            Item* item;
        };

        typedef vector<Item*>::type                     ItemList;
        typedef unordered_map<uint64, ItemList>::type   ItemListMap;
        typedef unordered_map<uint32, ItemList>::type   ItemListShortMap;
        typedef map<uint32, ItemList>::type             ItemListOrderMap;
        typedef unordered_map<String, Item*>::type      ItemMap;

    public:
        ~RenderBufferPool() { destroyAllBuffers(); }

    public:
        void  destroyAllBuffers();
        void  resetAllBufers();

        // get*请求各种资源，有效期只在1帧内有效，
        // *Temp是用完直接回收，否则会一直保留到1帧结束
        // name是可选项，可以在之后用getItem获取
        // create*持久有效
        Item*       getRTTBufferTemp( PixelFormat fmt, int width, int height, const String* name = 0 );
        TextureObj* getRTTBuffer( PixelFormat fmt, int width, int height, const String* name = 0 );
        TextureObj* createRTTBuffer( PixelFormat fmt, int width, int height );

        Item*       getRTTCubeBufferTemp( PixelFormat fmt, int size, const String* name = 0 );
        TextureObj* getRTTCubeBuffer( PixelFormat fmt, int size, const String* name = 0 );

        Item*       getDepthBufferTemp( int width, int height );

        Item*       getItem( const String& name ) const;

    private:
        TextureObj* _createRTTBuffer( PixelFormat fmt, int width, int height );

        Item* _getRTTCubeBuffer( PixelFormat fmt, int size );
        Item* _getRTTBuffer( PixelFormat fmt, int width, int height );
        Item* _getDepthBuffer( int width, int height, bool isTemp );

        void _checkWidthHeight( int& width, int& height );

        template<class T>
        void  _destroyBuffers( T& items, bool rtt );

        template<class T>
        void  _resetBuffers( T& items );

        void  _destroyBuffers( ItemList& items, bool rtt );
        void  _resetBuffers( ItemList& items );

    private:
        ItemMap          m_itemMap;
        ItemListMap      m_rttMap;
        ItemListShortMap m_rttCubeMap;
        ItemListOrderMap m_depthMap;
        Item             m_mainDepth;
    };
}

