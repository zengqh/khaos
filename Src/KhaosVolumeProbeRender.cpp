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
        // ��ȡ��Ⱦ�壬�ж��ڽ�ƽ����һ��
        VolProbRenderable* raObj = node->getRenderable();
        Plane::Side side = g_renderSystem->_getCurrCamera()->getPlane(Frustum::PLANE_NEAR).getSide(raObj->getImmAABBWorld());
        int groupIdx = 0;

        // ע�⣬��һ�ֿ��ܾ��ǹ�ģ�ͱ���׶�廹�����ﲻ����
        if ( side == Plane::POSITIVE_SIDE ) // �ڽ�ƽ�����棬����׶����
        {
            raObj->_setImmMtr( g_sysResManager->getMtrFrontDrawAdd() ); // ֻ������
            groupIdx = 0;
        }
        else
        {
            raObj->_setImmMtr( g_sysResManager->getMtrBackDrawAdd() ); // ֻ�����棨��Ϊ���汻������õ��ˣ�
            groupIdx = 1;
        }

        // �õ�effect
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

        // ������ϣ���������Ҫ����
        g_renderDevice->enableBlendState( true );

        // ����ǰ��
        //���Կ�����Ȳ��ԣ�����д��С�ڵ���ͨ���������Ż����õ�ǰ���ж��������������
        g_renderDevice->setDepthStateSet( DepthStateSet::TRANS_DRAW );
        _renderList( m_nodeList[0], params );

        // ���Ʊ���
        // ���Կ�����Ȳ��ԣ�����д�����ڵ���ͨ��������ֻ���㳡���͸õ����ཻ�Ĳ���
        g_renderDevice->setDepthStateSet( DepthStateSet::TEST_GE );
        _renderList( m_nodeList[1], params );
    }
}

