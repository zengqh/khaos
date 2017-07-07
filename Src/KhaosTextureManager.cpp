#include "KhaosPreHeaders.h"
#include "KhaosTextureManager.h"
#include "KhaosTextureManualCreator.h"
#include "KhaosTexCfgParser.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    struct _TexCreateBuff : public AllocatedObject
    {
        _TexCreateBuff() : texType(TEXTYPE_2D), texUsage(TEXUSA_STATIC),
            mipSize(0) {}

        TextureType  texType;
        TextureUsage texUsage;
        DataBuffer   texBuff;
        int          mipSize;
    };

    TextureResAutoCreator::TextureResAutoCreator()
    {
        m_basePath = "/Texture/";
    }

    String TextureResAutoCreator::getLocateFileName( const String& name ) const
    {
        return ResAutoCreatorBase::getLocateFileName( name ) + ".tex";
    }

    bool TextureResAutoCreator::_prepareResourceImpl( Resource* res, DataBuffer& buff )
    {
        FileSystem::DataBufferAuto buffAuto(buff); // ������Σ��������ݶ�����Ҫ��

        String imgFile = ResAutoCreatorBase::getLocateFileName( res->getName() );

        // �������������ļ�
        Texture* tex = static_cast<Texture*>(res);
        TexCfgParser parser;

        if ( !parser.parse( buff.data, buff.dataLen, tex ) )
        {
            khaosLogLn( KHL_L2, "TextureResAutoCreator::_prepareResourceImpl parse failed: %s", imgFile.c_str() );
            return false;
        }

        // ��ȡ��ӦͼƬ����
        DataBuffer buffImg;

        if ( parser.getTextureUsage() == TEXUSA_STATIC )
        {
            if ( !g_fileSystem->getFileData( imgFile, buffImg ) )
            {
                khaosLogLn( KHL_L2, "TextureResAutoCreator::_prepareResourceImpl image failed: %s", imgFile.c_str() );
                return false;
            }
        }

        // �󶨻���
        _TexCreateBuff* tcb = KHAOS_NEW _TexCreateBuff;
        tcb->texType  = parser.getTextureType();
        tcb->texUsage = parser.getTextureUsage();
        tcb->mipSize  = parser.getMipSize();
        tcb->texBuff  = buffImg;
        res->_getBufferTmp().data = tcb;
        return true;
    }

    bool TextureResAutoCreator::buildResource( Resource* res ) 
    {
        // ��������
        Texture* tex = static_cast<Texture*>(res);
        ScopedPtr<_TexCreateBuff> tcb( (_TexCreateBuff*)(tex->_getBufferTmp().data) );

        if ( tcb->texUsage == TEXUSA_STATIC )
        {
            FileSystem::DataBufferAuto buffAuto(tcb->texBuff);

            TexObjLoadParas paras;

            paras.type       = tcb->texType;
            paras.data       = tcb->texBuff.data;
            paras.dataLen    = tcb->texBuff.dataLen;
            paras.mipSize    = tcb->mipSize;
            paras.needMipmap = (tex->getFilter().tfMip != TEXF_NONE);

            return tex->loadTex( paras );
        }
        
        return true;
    }
}

