#include "KhaosPreHeaders.h"
#include "KhaosImageProcessComposite.h"
#include "KhaosRenderTarget.h"
#include "KhaosEffectID.h"

namespace Khaos
{
    ImgProcComposite::ImgProcComposite()
    {
        m_pin = _createImagePin( ET_COMPOSITE );
        m_pin->setOwnOutputEx( RCF_NULL ); // �������κ�
        m_pin->useMainDepth( true ); // ������Ҫģ���Ż�
        m_pin->useWPOSRender( true ); // ������Ҫ����ع�λ��
        m_pin->enableBlendMode( true ); // ��ϵ�scene buffer
        _setRoot( m_pin );
    }

    void ImgProcComposite::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
    }
}

