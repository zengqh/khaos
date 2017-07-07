#pragma once
#include "KhaosMaterial.h"
#include "KhaosRSCmd.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class GlossyAATool : public AllocatedObject
    {
    public:
        GlossyAATool();

        void build( Material* mtr );

        static void init();

    private:
        bool _needProcess();
        bool _prepareSpecularMap();
        void _modifyAllMips();
        void _saveResults();
        void _clean();

        int  _getDestMipLevels();

    private:
        Material*   m_mtr;
        TextureObj* m_specularOutRTT;
        NormalMapAttrib* m_normalMap;
    };

    //////////////////////////////////////////////////////////////////////////
    class GlossyAABatchCmd : public RSCmd
    {
    public:
        virtual void doCmd();
    };
}

