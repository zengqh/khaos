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

        // ��ȡ��Ⱦ�壬�ж��ڽ�ƽ����һ��
        VolumeRenderable* raObj = litNode->getVolumeRenderable();
        Plane::Side side = g_renderSystem->_getCurrCamera()->getPlane(Frustum::PLANE_NEAR).getSide(raObj->getImmAABBWorld());
        int groupIdx = 0;

        // ע�⣬��һ�ֿ��ܾ��ǹ�ģ�ͱ���׶�廹��������ʱ������
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

        // ������ϣ���������Ҫ����
        g_renderDevice->enableBlendState( true );

        // ����ǰ��
        //���Կ�����Ȳ��ԣ�����д��С�ڵ���ͨ���������Ż����õ�ǰ���ж��������������
        g_renderDevice->setDepthStateSet( DepthStateSet::TRANS_DRAW );
        _renderList( m_points[0], params );
        _renderList( m_spots[0], params );

        // ���Ʊ���
        // ���Կ�����Ȳ��ԣ�����д�����ڵ���ͨ��������ֻ���㳡���͸õ����ཻ�Ĳ���
        g_renderDevice->setDepthStateSet( DepthStateSet::TEST_GE );
        _renderList( m_points[1], params );
        _renderList( m_spots[1], params );
    }
}

