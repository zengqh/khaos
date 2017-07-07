#include "KhaosPreHeaders.h"
#include "KhaosVolumeProbeRender.h"
#include "KhaosEffectBuildStrategy.h"
#include "KhaosVolumeProbeNode.h"
#include "KhaosRenderSystem.h"
#include "KhaosSysResManager.h"
#include "KhaosCamera.h"
#include "KhaosRenderDevice.h"

namespace Khaos
{
    VolumeProbeRender* g_volumeProbeRender = 0;

    VolumeProbeRender::VolumeProbeRender()
    {
        khaosAssert(!g_volumeProbeRender);
        g_volumeProbeRender = this;
    }

    VolumeProbeRender::~VolumeProbeRender()
    {
        g_volumeProbeRender = 0;
    }

    void VolumeProbeRender::init()
    {
        // effect
        _registerEffectVSPS2HS( ET_VOLPROBACC, "volProbeAcc", "volProbeAcc", 
            ("pbrUtil", "materialUtil", "deferUtil"), VolProbeAccBuildStrategy );
    }

    void VolumeProbeRender::clear()
    {
        m_nodeList[0].clear();
        m_nodeList[1].clear();
    }

    void VolumeProbeRender::addVolumeProbe( VolumeProbeNode* node )
    {
        // 获取渲染体，判断在近平面哪一侧
        VolProbRenderable* raObj = node->getRenderable();
        Plane::Side side = g_renderSystem->_getCurrCamera()->getPlane(Frustum::PLANE_NEAR).getSide(raObj->getImmAABBWorld());
        int groupIdx = 0;

        // 注意，有一种可能就是光模型比视锥体还大，这里不考虑
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
        EffectContext* eff = g_effectTemplateManager->getEffectTemplate(ET_VOLPROBACC)->getEffectContext( raObj );
        RenderableContext* rc = m_nodeList[groupIdx].pushBack( eff, raObj );

        rc->raObj = raObj;
        rc->effContext = eff;
        rc->dist = 0;
    }

    void VolumeProbeRender::renderVolumeProbes( EffSetterParams& params )
    {
        bool hasSize = ( m_nodeList[0].hasSize() || m_nodeList[1].hasSize() );

        if ( !hasSize )
            return;

        // 开启混合，我们这里要叠加
        g_renderDevice->enableBlendState( true );

        // 绘制前面
        //可以开启深度测试，但不写，小于等于通过，可以优化掉该灯前面有东西挡掉它的情况
        g_renderDevice->setDepthStateSet( DepthStateSet::TRANS_DRAW );
        _renderList( m_nodeList[0], params );

        // 绘制背面
        // 可以开启深度测试，但不写，大于等于通过，这样只计算场景和该灯有相交的部分
        g_renderDevice->setDepthStateSet( DepthStateSet::TEST_GE );
        _renderList( m_nodeList[1], params );
    }
}

