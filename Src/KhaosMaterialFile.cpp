#include "KhaosPreHeaders.h"
#include "KhaosMaterialFile.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    bool MaterialImporter::importMaterial( const String& file, Material* mtr )
    {
        FileSystem::DataBufferAuto data;

        if ( !g_fileSystem->getFileData( file, data ) )
            return false;

        return importMaterial( data, mtr );
    }

    bool MaterialImporter::importMaterial( const DataBuffer& data, Material* mtr )
    {
        BinStreamReader reader(data.data, data.dataLen);

        // 基本属性
        int commState = 0;
        reader.read( commState );
        mtr->setCommState( Material::StateSet( commState ) );

        int mtrState = 0;
        reader.read( mtrState );
        mtr->setMaterialState( MaterialStateSet(mtrState) );
       
        int blendState = 0;
        reader.read( blendState );
        mtr->setBlendState( BlendStateSet(blendState) );

        int blendEnabled = 0;
        reader.read( blendEnabled );
        mtr->enableBlend( blendEnabled != 0 );

        // 读取每个属性
        int attribSize = 0;
        reader.read( attribSize );

        for ( int i = 0; i < attribSize; ++i )
        {
            // 读一个类型
            int attribType = 0;
            reader.read( attribType );

            // 创建
            MtrAttrib* attr = mtr->useAttrib( (MtrAttribType) attribType );

            // 属性自己去读
            attr->_importAttrib( reader );
        }

        // 读取属性标记
        int attribFlag = 0;
        reader.read( attribFlag );
        mtr->setAttribsFlag( MtrAttribMap::FlagType(attribFlag) );
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool MaterialExporter::exportMaterial( const String& file, const Material* mtr )
    {
        BinStreamWriter writer;

        if ( !exportMaterial(writer, mtr) )
            return false;

        DataBuffer buff;
        buff.data = writer.getBlock();
        buff.dataLen = writer.getCurrentSize();

        return g_fileSystem->writeFile( file, buff );
    }

    bool MaterialExporter::exportMaterial( BinStreamWriter& writer, const Material* mtr )
    {
        // 基本属性
        int commState = mtr->getCommState().getValue();
        writer.write( commState );

        int mtrState = mtr->getMaterialState().data;
        writer.write( mtrState );

        int blendState = mtr->getBlendState().data;
        writer.write( blendState );

        int blendEnabled = mtr->isBlendEnabled();
        writer.write( blendEnabled );

        // 每个属性
        int attribSize = mtr->getAttribSize();
        writer.write( attribSize );

        Material::Iterator it = mtr->getAllAttribs();
        while ( it.hasNext() )
        {
            MtrAttrib* attr = it.get().second;

            // 一个类型
            int attribType = attr->getType();
            writer.write( attribType );

            // 属性自己去写
            attr->_exportAttrib( writer );
        }

        // 属性标记
        int attribFlag = mtr->getAttribsFlag().getValue();
        writer.write( attribFlag );
        return true;
    }
}

