#include "KhaosPreHeaders.h"
#include "KhaosLightRender.h"
#include "KhaosEffectBuildStrategy.h"
#include "KhaosLightNode.h"
#include "KhaosRenderSystem.h"
#include "KhaosSysResManager.h"
#include "KhaosCamera.h"
#include "KhaosRenderDevice.h"

namespace Khaos
{
    LightRender* g_lightRender = 0;

    LightRender::LightRender()
    {
        khaosAssert(!g_lightRender);
        g_lightRender = this;
    }

    LightRender::~LightRender()
    {
        g_lightRender = 0;
    }

    void LightRender::init()
    {
        // effect
        _registerEffectVSPS2HS( ET_LITACC, "litAcc", "litAcc", 
            ("pbrUtil", "materialUtil", "deferUtil"), LitAccBuildStrategy );
    }

    void LightRender::clear()
    {
        for ( int i = 0; i < 2; ++i )
        {
            m_points[i].clear();
            m_spots[i].clear();
        }
    }

    void LightRender::addLight( LightNode* litNode )
    {
        KindRenderableList* raLists = 0;

        LightType litType = litNode->getLightType();

        switch (litType)
        {
        case LT_POINT:
            raLists = m_points;
            break;

        case LT_SPOT:
            raLists = m_spots;
            break;
        }

        if ( !raLists )
             return;

        // 获取渲染体，判断在近平面哪一侧
        VolumeRenderable* raObj = litNode->getVolumeRenderable();
        Plane::Side side = g_renderSystem->_getCurrCamera()->getPlane(Frustum::PLANE_NEAR).getSide(raObj->getImmAABBWorld());
        int groupIdx = 0;

        // 注意，有一种可能就是光模型比视锥体还大，这里暂时不考虑
        if ( side == Plane::POSITIVE_SIDE ) // 在近平面正面，即视锥体内
        {
            raObj->_setImmMtr( g_sysResManager->getMtrFrontDrawAdd() ); // 只画正面
            groupIdx = 0;
        }
        else
        {
            raObj->_setImmMtr( g_sysResManager->getMtrBackDrawAdd() ); // 只画背面（因为正面被近裁面裁掉了）
            groupIdx = 1;
        }

        // 得到effect
        EffectContext* eff = g_effectTemplateManager->getEffectTemplate(ET_LITACC)->getEffectContext( raObj );
        RenderableContext* rc = raLists[groupIdx].pushBack( eff, raObj );

        rc->raObj = raObj;
        rc->effContext = eff;
        rc->dist = 0;
    }

    void LightRender::renderLights( EffSetterParams& params )
    {
        bool hasSize = ( m_points[0].hasSize() || m_points[1].hasSize() || 
            m_spots[0].hasSize() || m_spots[1].hasSize() );

        if ( !hasSize )
            return;

        // 开启混合，我们这里要叠加
        g_renderDevice->enableBlendState( true );

        // 绘制前面
        //可以开启深度测试，但不写，小于等于通过，可以优化掉该灯前面有东西挡掉它的情况
        g_renderDevice->setDepthStateSet( DepthStateSet::TRANS_DRAW );
        _renderList( m_points[0], params );
        _renderList( m_spots[0], params );

        // 绘制背面
        // 可以开启深度测试，但不写，大于等于通过，这样只计算场景和该灯有相交的部分
        g_renderDevice->setDepthStateSet( DepthStateSet::TEST_GE );
        _renderList( m_points[1], params );
        _renderList( m_spots[1], params );
    }
}

