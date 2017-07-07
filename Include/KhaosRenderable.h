#pragma once
#include "KhaosMatrix4.h"
#include "KhaosAxisAlignedBox.h"
#include "KhaosRenderableSharedData.h"

namespace Khaos
{
    class Material;
    class Mesh;
    class LightsInfo;
    class TextureObj;
    class Camera;

    //////////////////////////////////////////////////////////////////////////
    class Renderable : public AllocatedObject
    {
    public:
        virtual ~Renderable() {}

    public:
        // basic info
        virtual const Matrix4&        getImmMatWorld()  { return Matrix4::IDENTITY; }
        virtual const AxisAlignedBox& getImmAABBWorld() { return AxisAlignedBox::BOX_INFINITE; }
        virtual Material*             _getImmMaterial() { return 0; }
        virtual LightsInfo*           getImmLightInfo() { return 0; }
        virtual bool                  isReceiveShadow() { return false; }

        virtual RenderableSharedData* getRDSharedData();

    public:
        virtual void render() {}
        
        const void* findCurrDirLitInfoItem();

    public:
        Material* getImmMaterial();
    };

    //////////////////////////////////////////////////////////////////////////
    class MeshRectDebugRenderable : public Renderable
    {
    public:
        MeshRectDebugRenderable() : m_mesh(0) {}

        void setRect( int x, int y, int w, int h );
        void setTexture( TextureObj* tex );

        virtual Material* _getImmMaterial();
        virtual void render();

    private:
        Mesh* m_mesh;
    };

    //////////////////////////////////////////////////////////////////////////
    class FullScreenDSRenderable : public Renderable
    {
    public:
        FullScreenDSRenderable() : m_mtr(0), m_cam(0), m_data(0), m_z(0) {}

        void setMaterial( Material* mtr ) { m_mtr = mtr; }
        void setCamera( Camera* cam, float z = 0 );

        virtual Material* _getImmMaterial() { return m_mtr; }
        virtual void render();

        void setData( void* data ) { m_data = data; }
        void* getData() const { return m_data; }

    private:
        Material* m_mtr;
        Camera*   m_cam;
        void*     m_data;
        float     m_z;
    };
}

