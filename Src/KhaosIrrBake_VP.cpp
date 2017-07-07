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
    const float e_dot = 0.00873f; // 89.5度 

    //////////////////////////////////////////////////////////////////////////
    // bake probe
    void IrrBake::onPrepareVolumeProbe( SceneNode* node )
    {
        // 创建资源
        VolumeProbeNode* volProbeNode = static_cast<VolumeProbeNode*>( node );
        VolumeProbe* volProbe = volProbeNode->getProbe();
        volProbe->unload(); // free exist first
        volProbe->setVolID( -1 );

        VolumeMapSet* lms = _getVolMapSet( node );

        lms->createMaps( volProbe->getResolutionX(), volProbe->getResolutionY(), volProbe->getResolutionZ() );
        lms->setOutMaps( m_resBaseVolName, m_volmapNextID );

        if ( m_prebuildVolDataPath.size() )
        {
            char preName[1024] = {};
            sprintf( preName, m_prebuildVolDataPath.c_str(), m_volmapNextID );
            lms->preFileName = preName;
        }

        ++m_volmapNextID;
    }

    void IrrBake::_finishVolMapDone()
    {
        KHAOS_FOR_EACH( VolumeMapSetMap, m_volMapSet, it )
        {
            _saveVolResult( it->first, it->second );
        }
    }

    void IrrBake::_saveVolResult( SceneNode* node, VolumeMapSet* vms )
    {
        static const char* strChannel[3] = { "r", "g", "b" };

        for ( int i = 0; i < 3; ++i )
        {
            SimpleVolumeMap* svm = vms->indIrrVolMaps[i];

#if 0
            {
                for ( int z = 0; z < svm->getDepth(); ++z )
                {
                    for ( int y = 0; y < svm->getHeight(); ++y )
                    {
                        for ( int x = 0; x < svm->getWidth(); ++x )
                        {
                            Color clr = Color::BLACK;
                            clr[i] = x / (float)(svm->getWidth() - 1);
                            clr.a = 1;
                            svm->setColor( x, y, z, clr );
                        }
                    }
                }
            }
#endif

            // save volume map
            char file[4096] = {};
            sprintf( file, vms->outFileNameFmt.c_str(), vms->mapID, strChannel[i] );
            String resFullFileName = g_fileSystem->getFullFileName(file);
            svm->saveFile( PIXFMT_A32B32G32R32F, resFullFileName.c_str() );

            TexCfgSaver::saveSimple( file, TEXTYPE_VOLUME, PIXFMT_A32B32G32R32F,
                TEXADDR_CLAMP, TEXF_LINEAR, TEXF_NONE, false, 1 );
        }

        _outputDebugStr( "IrrBake: volume save[%s] <= %d\n", node->getName().c_str(), vms->mapID );
    }

    void IrrBake::_initProbeSampleInfos()
    {
        m_sphVolMapper.setRandsDistribution( &m_randFull, SphereSampleMapper::EntireSphereUniform );
        m_sphVolMapper.setCommConfig( true, true );
        m_sphVolMapper.general();

        m_probeSampleInfos.resize( m_sphVolMapper.getNumSamples() );

        for ( int i = 0; i < m_sphVolMapper.getNumSamples(); ++i )
        {
            // 计算当前方向的半球潜在可见采样集合
            HemiInfoList& info = m_probeSampleInfos[i];
            info.resize( m_sphVolMapper.getNumSamples() );

            const Vector3& currNormDir = m_sphVolMapper.getSample(i).dir;

            for ( int j = 0; j < m_sphVolMapper.getNumSamples(); ++j )
            {
                const Vector3& inComingDir = m_sphVolMapper.getSample(j).dir;
                float NdotL = currNormDir.dotProduct( inComingDir );

                if ( NdotL > e_dot ) // 这里只考虑半球可见，场景可见需要后续自行计算
                {
                    info[j] = HemiInfo(j, NdotL, true);
                }
                else
                {
                    info[j] = HemiInfo(j, 0, false);
                }
            }
        }
    }

    void IrrBake::_buildAllVolumeProbes()
    {
        const BakeSystem::SceneNodeList& litNodes = this->m_bakeSys.getVolumeProbeNodeList();
        KHAOS_FOR_EACH_CONST( BakeSystem::SceneNodeList, litNodes, it )
        {
            m_currVolProbNode = static_cast<VolumeProbeNode*>(*it);
            _buildOneVolumeProbe();
        }
    }

    void IrrBake::_buildOneVolumeProbe()
    {
        _outputDebugStr( "IrrBake: bake volume probe - %s\n", m_currVolProbNode->getName().c_str() );

        VolumeMapSet* vms = this->_getVolMapSet( m_currVolProbNode );
        VolumeProbe* probe = m_currVolProbNode->getProbe();

        // 预处理文件设置
        khaosAssert( !m_preVolHeadFile );
        _createPreBuildFiles( m_preVolHeadFile, m_preVolFile, vms->preFileName, 
            probe->getResolutionX(), probe->getResolutionY(),  probe->getResolutionZ(),
            m_bakeSys.getThreadsCount(), m_randFull.getNumSamples(), m_onlyUseVol );

        // 执行每个体素的bake
        int voxelCnt = probe->getResolutionX() * probe->getResolutionY() * probe->getResolutionZ();

        BatchTasks tasks;
        tasks.setTask( _bakeVolProbeSatic );
        tasks.setUserData( this );
        tasks.planTask( voxelCnt, m_bakeSys.getThreadsCount() );

        // 关闭预处理文件
        int count = m_bakeSys.getThreadsCount();
        for ( int i = 0; i < count; ++i )
        {
            KHAOS_DELETE m_preVolFile[i];
            m_preVolFile[i] = 0;
        }

        KHAOS_DELETE m_preVolHeadFile;
        m_preVolHeadFile = 0;
    }

    void IrrBake::_bakeVolProbeSatic( int threadID, void* para, int voxelID )
    {
        IrrBake* baker = (IrrBake*)para;

        if ( baker->m_status == IBS_PREBUILD )
        {
            baker->_bakeVolProbePre( threadID, voxelID );
        }
        else
        {
            khaosAssert( baker->m_status == IBS_VOLPROB );
            baker->_bakeVolProbe( threadID, voxelID );
        }
    }

    void IrrBake::_voxelIDToXYZ( VolumeProbe* probe, int voxelID, int& x, int& y, int& z ) const
    {
        // 从id转换到x,y,z
        const int planeSize = probe->getResolutionX() * probe->getResolutionY();

        int xy = voxelID % planeSize;
        z = voxelID / planeSize;

        x = xy % probe->getResolutionX();
        y = xy / probe->getResolutionX();
    }

    void IrrBake::_bakeVolProbePre( int threadID, int voxelID )
    {
        // 当前光探
        VolumeProbe* probe = m_currVolProbNode->getProbe();

        // 当前线程预处理文件
        LightMapPreBuildFile& preFile = *(m_preVolFile[threadID]);
        
        // voxelID得到坐标
        int x, y, z;
        _voxelIDToXYZ( probe, voxelID, x, y, z );

        // 光探世界位置
        Vector3 centerPos = probe->getCenterWorldPos( x, y, z );

        // 信息对象
        LightMapPreBuildHeadFile::ItemHeadInfo* headInfo = m_preVolHeadFile->getItemInfo( x, y, z );
        LightMapPreBuildFile::TexelItem* texelItem = preFile.setCurrTexelItem( x, y, z );
  
        headInfo->subIdxInMesh  = voxelID;
        headInfo->faceIdxInMesh = -1;
       
        // 收集此光探的射线信息
        for ( int i = 0; i < m_sphVolMapper.getNumSamples(); ++i )
        {
            const SphereSampleMapper::Sample& smpl = m_sphVolMapper.getSample( i );

            Ray ray( centerPos, smpl.dir );

            SceneBVH::Result result;

            if ( !m_bakeSys.getSG()->getBVH()->intersect( ray, result ) )
                continue; // 不相交
            
            const SBVHFaceInfo* faceInfo = m_bakeSys.getSG()->getBVH()->getFaceInfo( result.faceID );

            SceneNode* node = m_bakeSys.getSG()->getBVH()->getSceneNode( faceInfo->instanceID );
            MeshNode* meshNode = static_cast<MeshNode*>(node);
            SubMesh*  subMesh  = meshNode->getMesh()->getSubMesh(faceInfo->subIndex);

            // 是否交在正面上
            Vector3  normalOther = subMesh->getTriNormal( faceInfo->faceIdx, result.gravity );
            normalOther = meshNode->getDerivedMatrix().transformAffineNormal( normalOther );

            if ( normalOther.dotProduct(smpl.dir) < 0 ) // 应该法线相对
            {
                LightMapPreBuildFile::RayInfo* rayInfo = texelItem->addRayInfo( i );
                rayInfo->globalFaceID = result.faceID; // 记录全局面id
                rayInfo->flag = 0; // 正面
            }
        }

        // 写入信息
        preFile.writeCurrTexelItem();
    }

    float _reconstructSH4( float* b, const Vector3& s )
    {
        static const float c0 = 1.0f / (2.0f * Math::sqrt(Math::PI));
        static const float c1 = Math::sqrt(3.0f) / (2.0f * Math::sqrt(Math::PI));
        static const Vector4 y = Vector4(c0, -c1, c1, -c1);

        Vector4 by = (*(Vector4*)b) * y;
        float dd = Vector4(1, s.z, s.y, s.x).dotProduct( by );
        return Math::maxVal( dd, 0.0f );
    }

    void IrrBake::_bakeVolProbe( int threadID, int voxelID )
    {
        _outputDebugStr( "IrrBake::_bakeVolProbe: vox-%d, tid-%d\n", voxelID, threadID );

        // 当前光探
        VolumeProbe* probe = m_currVolProbNode->getProbe();

        // 当前线程预处理文件
        LightMapPreBuildFile& preFile = *(m_preVolFile[threadID]);

        // voxelID得到坐标
        int x, y, z;
        _voxelIDToXYZ( probe, voxelID, x, y, z );

        // 光探世界位置
        Vector3 centerPos = probe->getCenterWorldPos( x, y, z );

        // 信息对象
        LightMapPreBuildFile::TexelItem* texelItem = preFile.readNextTexelItem( x, y, z );
        khaosAssert( texelItem );

        // 计算这个光探的每个方向的irradiance
        const float integScale = (4.0f * Math::PI) / m_sphVolMapper.getNumSamples();

        vector<Vector3>::type irrs( m_sphVolMapper.getNumSamples() );

        for ( int i = 0; i < m_sphVolMapper.getNumSamples(); ++i )
        {
            const HemiInfoList& hil = m_probeSampleInfos[i]; // 此方向的已经预计算的一些信息
            Color totalVal(Color::BLACK);

            // 遍历每个潜在样本
            for ( int j = 0; j < m_sphVolMapper.getNumSamples(); ++j ) 
            {
                const HemiInfo& hi = hil[j]; 
                if ( !hi.isFront ) // 获取一个该方向的正半球的样本
                    continue;

                const LightMapPreBuildFile::RayInfo* rayInfo = texelItem->getRayInfo( j );
                if ( !rayInfo ) // 与世界有交
                    continue;

                Ray ray( centerPos, m_sphVolMapper.getSample(j).dir );

                SceneBVH::Result result;
                m_bakeSys.getSG()->getBVH()->intersectTriAlways( ray, rayInfo->globalFaceID, result ); // 必定和此三角形相交

                const SBVHFaceInfo* faceInfo = m_bakeSys.getSG()->getBVH()->getFaceInfo( rayInfo->globalFaceID );

                int subIdx = faceInfo->subIndex;
                SceneNode* node = m_bakeSys.getSG()->getBVH()->getSceneNode( faceInfo->instanceID );

                MeshNode* meshNode = static_cast<MeshNode*>(node);
                SubMesh*  subMesh  = meshNode->getMesh()->getSubMesh(subIdx);

                Material*  mtrOther = meshNode->getSubEntity(subIdx)->getMaterial();
                LitMapSet* lmsOther = _getLitMapSet(node); // 取它的图集

                Vector2 uv1 = subMesh->getTriUV( faceInfo->faceIdx, result.gravity ); // 交点处的uv1
                Vector2 uv2 = subMesh->getTriUV2( faceInfo->faceIdx, result.gravity ); // 交点处的uv2

                // irr = (other emit + other diffuse) * NdotL
                Color otherOutLight = lmsOther->initEmit->getColorByUV(uv2) + // other emit
                    lmsOther->indIrrNext->getColorByUV(uv2) * mtrOther->_readDiffuseColor( uv1 ); // other diffuse

                totalVal += otherOutLight * hi.NdotL; // add this incoming light
            }

            totalVal *= integScale;
            irrs[i].setValue( totalVal.ptr() );
        }

        // 投影到SH
        float shClr[3][4];

        SphereSamples theSS;
        theSS.setSamples( &m_sphVolMapper );
        theSS.projectSH( _projectProbeToSHStatic, &irrs, &shClr[0][0], 4, 3 );        

        VolumeMapSet* vms = _getVolMapSet( m_currVolProbNode );
        vms->indIrrVolMapR->setColor( x, y, z, *(Color*)&shClr[0][0] );
        vms->indIrrVolMapG->setColor( x, y, z, *(Color*)&shClr[1][0] );
        vms->indIrrVolMapB->setColor( x, y, z, *(Color*)&shClr[2][0] );

        // 测试
#if 0
        for ( int i = 0; i < m_sphVolMapper.getNumSamples(); ++i )
        {
            const Vector3& oldVal = irrs[i];
            const Vector3& norm = m_sphVolMapper.getSample(i).dir;
            
            float newVal;
            SHMath::reconstructFunction4( &shClr[0][0], 1, norm.x, norm.y, norm.z, &newVal );
            if ( newVal < 0 ) newVal = 0;

            float newVal2 = _reconstructSH4( &shClr[0][0], norm );

            float pp = (newVal - oldVal.x) / oldVal.x * 100;
            float pp2 = (newVal2 - oldVal.x) / oldVal.x * 100;

            _outputDebugStr( "diff = (%f, %f)\n", pp, pp2 );
        }
#endif
    }

    void IrrBake::_projectProbeToSHStatic( SphereSamples* theSS, const SphereSamples::Sample& smpl, 
        void* context, float* vals, int groupCnt )
    {
        vector<Vector3>::type& datas = *(vector<Vector3>::type*)context;

        *(Vector3*)vals = datas[smpl.idx];
    }
}

