#include "KhaosPreHeaders.h"
#include "KhaosIrrBake.h"
#include "KhaosMesh.h"
#include "KhaosMeshNode.h"
#include "KhaosMaterialManager.h"
#include "KhaosTextureManager.h"
#include "KhaosTexCfgParser.h"
#include "KhaosMaterialFile.h"
#include "KhaosRayQuery.h"
#include "KhaosTimer.h"
#include "KhaosLightNode.h"
#include "KhaosVolumeProbeNode.h"
#include "KhaosSceneGraph.h"
#include "KhaosInstObjSharedData.h"
#include "KhaosNameDef.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    IrrBake::LitMapSet::~LitMapSet()
    {
        KHAOS_DELETE bentNormalMap;
        KHAOS_DELETE initEmit;
        KHAOS_DELETE initAmb;
        KHAOS_DELETE indIrrPre;
        KHAOS_DELETE indIrrNext;
    }

    void IrrBake::LitMapSet::createLitMap( int w, int h, bool needBentNormal, bool needAmb )
    {
        if ( initEmit )
        {
            khaosAssert( indIrrPre );
            khaosAssert( indIrrNext );
            return;
        }

        if ( needBentNormal )
        {
            bentNormalMap = KHAOS_NEW SimpleLightMap; // ��ʼ��BentNormal
            bentNormalMap->setSize( w, h, 3 );
        }

        {
            initEmit = KHAOS_NEW SimpleLightMap; // ��ʼ���Է���ͨ��
            initEmit->setSize( w, h, 3 );
        }

        if ( needAmb )
        {
            initAmb = KHAOS_NEW SimpleLightMap; // ��ʼ��������ͨ��
            initAmb->setSize( w, h, 3 );
        }

        indIrrPre = KHAOS_NEW SimpleLightMap; // ��ʼ����һ�ε����Ĺ���ͼ
        indIrrPre->setSize( w, h, 3 );

        indIrrNext = KHAOS_NEW SimpleLightMap; // ��ʼ����һ�ε����Ĺ���ͼ
        indIrrNext->setSize( w, h, 3 );
    }

    void IrrBake::LitMapSet::setOutMaps( const String& lma, const String& lmb, int id )
    {
        outLightMapA = lma;
        outLightMapB = lmb;
        lightMapID   = id;
    }

    void IrrBake::LitMapSet::swapPreNext()
    {
        swapVal( indIrrPre, indIrrNext ); // ��������
    }

    //////////////////////////////////////////////////////////////////////////
    IrrBake::VolumeMapSet::~VolumeMapSet()
    {
        KHAOS_DELETE indIrrVolMapR;
        KHAOS_DELETE indIrrVolMapG;
        KHAOS_DELETE indIrrVolMapB;
    }

    void IrrBake::VolumeMapSet::createMaps( int w, int h, int d )
    {
        for ( int i = 0; i < 3; ++i )
        {
            khaosAssert( !indIrrVolMaps[i] );
            indIrrVolMaps[i] = KHAOS_NEW SimpleVolumeMap;
            indIrrVolMaps[i]->init( w, h, d, Color::ZERO );
        }
    }

    void IrrBake::VolumeMapSet::setOutMaps( const String& fileNameFmt, int id )
    {
        outFileNameFmt = fileNameFmt;
        mapID = id;
    }

    //////////////////////////////////////////////////////////////////////////
    IrrBake::LitMapSet* IrrBake::_getLitMapSet( SceneNode* node )
    {
        LitMapSet*& lms = m_litMapSet[node];
        if ( lms )
            return lms;

        lms = KHAOS_NEW LitMapSet;
        return lms;
    }

    IrrBake::VolumeMapSet* IrrBake::_getVolMapSet( SceneNode* node )
    {
        VolumeMapSet*& lms = m_volMapSet[node];
        if ( lms )
            return lms;

        lms = KHAOS_NEW VolumeMapSet;
        return lms;
    }

    void IrrBake::_freeAllLitMapSet()
    {
        KHAOS_FOR_EACH( LitMapSetMap, m_litMapSet, it )
        {
            LitMapSet* lms = it->second;
            KHAOS_DELETE lms;
        }

        m_litMapSet.clear();
    }

    void IrrBake::_freeAllVolMapSet()
    {
        KHAOS_FOR_EACH( VolumeMapSetMap, m_volMapSet, it )
        {
            VolumeMapSet* lms = it->second;
            KHAOS_DELETE lms;
        }

        m_volMapSet.clear();
    }

    //////////////////////////////////////////////////////////////////////////
    void IrrBake::_createPreBuildFiles( LightMapPreBuildHeadFile*& headFile, LightMapPreBuildFile** preFiles,
        const String& name, int w, int h, int d, int threadCount, int sampleCnt, bool onlyRead )
    {
        if ( threadCount == 0 ) // ֻ�����߳�
            threadCount = 1;

        // ��������Ϣ�ļ�
        headFile = KHAOS_NEW LightMapPreBuildHeadFile;

        if ( onlyRead )
        {
            headFile->openFile( name );
        }
        else
        {
            headFile->setFile( name );
            headFile->setSize( w, h, d );
            headFile->setMaxRayCount( sampleCnt );
        }

        // ÿ���߳�һ�������ļ���д������������
        for ( int i = 0; i < threadCount; ++i )
        {
            LightMapPreBuildFile* preFile = KHAOS_NEW LightMapPreBuildFile(headFile, i); 
            preFiles[i] = preFile;

            String file = name + "@" + intToString(i);

            if ( onlyRead )
                preFile->openExistHeadFile( file );
            else
                preFile->openNewHeadFile( file );
        }
    }
}

