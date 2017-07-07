#pragma once
#include "KhaosMaterial.h"
#include "KhaosFileSystem.h"
#include "KhaosBinStream.h"

namespace Khaos
{
    
    //////////////////////////////////////////////////////////////////////////
    class MaterialImporter : public AllocatedObject
    {
    public:
        bool importMaterial( const String& file, Material* mtr );
        bool importMaterial( const DataBuffer& data, Material* mtr );
    };

    //////////////////////////////////////////////////////////////////////////
    class MaterialExporter : public AllocatedObject
    {
    public:
        bool exportMaterial( const String& file, const Material* mtr );
        bool exportMaterial( BinStreamWriter& writer, const Material* mtr );
    };
}

