#pragma once
#include "KhaosImageProcess.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class ImgProcComposite : public ImageProcess
    {
    public:
        ImgProcComposite();

        void setOutput( TextureObj* texOut );

    private:
        ImagePin*   m_pin;
    };
}

