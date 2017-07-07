#pragma once
#include "KhaosPreHeaders.h"
#include "KhaosRenderable.h"
#include "KhaosSysResManager.h"
#include "KhaosRenderDevice.h"
#include "KhaosRenderSystem.h"
#include "KhaosLight.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    RenderableSharedData* Renderable::getRDSharedData()
    {
        static RenderableSharedData s_null;
        return &s_null;
    }

    Material* Renderable::getImmMaterial()
    {
        Material* mtr = _getImmMaterial();
        return mtr->isLoaded() ? mtr : g_sysResManager->getDefaultMaterial();
    }

    const void* Renderable::findCurrDirLitInfoItem()
    {
        const LightsInfo* litInfo = getImmLightInfo();

        // 平行光情况
        if ( const LightsInfo::LightListInfo* dirLits = litInfo->getLightListInfo(LT_DIRECTIONAL) )
        {
            // 查找是否当前活动平行光
            for ( int i = 0; i < dirLits->maxN; ++i )
            {
                const LightsInfo::LightItem& curItem = dirLits->litList[i];

                if ( curItem.lit == g_renderSystem->_getCurrDirLit() ) // 是系统当前活跃的平行光
                {
                    return &curItem;
                }
            }
        }

        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    void MeshRectDebugRenderable::setRect( int x, int y, int w, int h )
    {
        m_mesh = g_sysResManager->getDebugMeshRect( x, y, w, h );
    }

    void MeshRectDebugRenderable::setTexture( TextureObj* tex )
    {
        g_sysResManager->getTexPointTmp()->bindTextureObj( tex );
    }

    Material* MeshRectDebugRenderable::_getImmMaterial()
    {
        return g_sysResManager->getMtrPointTmp();
    }

    void MeshRectDebugRenderable::render()
    {
        m_mesh->drawSub(0);
    }

    //////////////////////////////////////////////////////////////////////////
    void FullScreenDSRenderable::setCamera( Camera* cam, float z )
    {
        m_cam = cam;
        m_z   = 0;
    }

    void FullScreenDSRenderable::render()
    {
        if ( m_cam )
            g_sysResManager->getMeshFullScreenWPOS( m_cam, m_z )->drawSub(0);
        else
            g_sysResManager->getMeshFullScreenDS()->drawSub(0);
    }
}

