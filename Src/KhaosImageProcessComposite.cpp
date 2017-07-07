#include "KhaosPreHeaders.h"
#include "KhaosImageProcessComposite.h"
#include "KhaosRenderTarget.h"
#include "KhaosEffectID.h"

namespace Khaos
{
    ImgProcComposite::ImgProcComposite()
    {
        m_pin = _createImagePin( ET_COMPOSITE );
        m_pin->setOwnOutputEx( RCF_NULL ); // 不清理任何
        m_pin->useMainDepth( true ); // 我们需要模版优化
        m_pin->useWPOSRender( true ); // 我们需要深度重构位置
        m_pin->enableBlendMode( true ); // 混合到scene buffer
        _setRoot( m_pin );
    }

    void ImgProcComposite::setOutput( TextureObj* texOut )
    {
        m_pin->getOutput()->linkRTT( texOut );
    }
}

