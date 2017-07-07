#include "KhaosPreHeaders.h"
#include "KhaosBakeUtil.h"
#include "KhaosLightMapUtil.h"
#include "KhaosMeshUtil.h"
#include "KhaosSceneGraph.h"
#include "KhaosMeshManager.h"
#include "KhaosMaterialManager.h"
#include "KhaosMeshFile.h"
#include "KhaosMaterialFile.h"
#include "KhaosThread.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    BakeVertexProcess::BakeVertexProcess( Mesh* mesh, const Matrix4& matWorld ) :
        m_mesh(mesh), m_matWorld(matWorld), m_currSubMesh(0), m_callback(0)
    {
    }

    void BakeVertexProcess::general( IBakeVertexProcessCallback* ev, int threads )
    {
        m_callback = ev;
        m_mesh->cacheLocalData( false );

        int subMeshCnt = m_mesh->getSubMeshCount();

        for ( int subIdx = 0; subIdx < subMeshCnt; ++subIdx )
        {
            m_currSubMesh = m_mesh->getSubMesh(subIdx);
            m_currVB      = m_currSubMesh->getVertexBuffer();
            int vtxCnt    = m_currVB->getVertexCount();

            BatchTasks tasks;
            tasks.setTask( _bakeSatic );
            tasks.setUserData( this );
            tasks.planTask( vtxCnt, threads );
        }
    }

    void BakeVertexProcess::_bakeSatic( int threadID, void* para, int v )
    {
        BakeVertexProcess* bvp = (BakeVertexProcess*)para;

        const Vector3* pos  = bvp->m_currVB->getCachePos(v);
        const Vector3* norm = bvp->m_currVB->getCacheNormal(v);
        const Vector2* uv   = bvp->m_currVB->getCacheTex(v);

        Vector3 posWorld  = bvp->m_matWorld.transformAffine( *pos );
        Vector3 normWorld = bvp->m_matWorld.transformAffineNormal( *norm ).normalisedCopy();

        bvp->m_callback->onPerVertex( threadID, posWorld, normWorld, *uv );

    }


    //////////////////////////////////////////////////////////////////////////
    void SceneBakeUniquify::process( SceneGraph* sgIn, int config )
    {
        m_config = config;

        struct Visitor_ : public ITravelSceneGraphCallback
        {
            Visitor_( SceneBakeUniquify* s ) : m_sbu(s) {}

            SceneBakeUniquify* m_sbu;

            virtual void onVisitNode( SceneNode* node )
            {
                if ( node->isEnabled() && KHAOS_OBJECT_IS( node, MeshNode ) )
                {
                    m_sbu->_dealNode( static_cast<MeshNode*>(node) );
                }
            }
        };

        Visitor_ op(this);
        sgIn->travel( &op );
    }

    void SceneBakeUniquify::save( const String& extInfoFile, int flag )
    {
        if ( flag & SBU_SAVE_MESH )
        {
            KHAOS_FOR_EACH( MeshList, m_newMeshList, it )
            {
                Mesh* mesh = *it;
                MeshExporter meshExp;
                meshExp.exportMesh( mesh->getResFileName(), mesh );
            }
        }

#if 0
        if ( flag & SBU_SAVE_MTR )
        {
            KHAOS_FOR_EACH( MtrList, m_newMtrList, it )
            {
                Material* mtr = *it;
                MaterialExporter mtrExp;
                mtrExp.exportMaterial( mtr->getResFileName(), mtr );
            }
        }
#endif

        if ( flag & SBU_SAVE_INFO )
        {
            _saveExtInfo( extInfoFile );
        }
    }

    void SceneBakeUniquify::_saveExtInfo( const String& extInfoFile )
    {
        BinStreamWriter bsw;

        bsw.write( (int)m_meshNewInfoMap.size() );

        KHAOS_FOR_EACH( NewMeshInfoMap, m_meshNewInfoMap, it )
        {
            const String& oldName = it->first;
            const NewMeshInfo& info = it->second;

            bsw.writeString( oldName );
            bsw.writeString( info.newMeshName );
            bsw.write( info.bakeTextureWidth );
            bsw.write( info.bakeTextureHeight );
        }

        DataBuffer buff;
        buff.data = bsw.getBlock();
        buff.dataLen = bsw.getCurrentSize();

        g_fileSystem->writeFile( extInfoFile, buff );
    }

    void SceneBakeUniquify::load( const String& extInfoFile )
    {
        FileSystem::DataBufferAuto data;

        if ( !g_fileSystem->getFileData( extInfoFile, data ) )
            return;

        BinStreamReader bsr(data.data, data.dataLen);
        
        int itemCnt = 0;
        bsr.read( itemCnt );

        for ( int i = 0; i < itemCnt; ++i )
        {
            String oldName;
            bsr.readString( oldName );

            NewMeshInfo info;
            bsr.readString( info.newMeshName );
            bsr.read( info.bakeTextureWidth );
            bsr.read( info.bakeTextureHeight );

            m_meshNewInfoMap[oldName] = info;
        }
    }

    void SceneBakeUniquify::_dealNode( MeshNode* node )
    {
        _dealNodeMesh( node );
        _dealNodeMtr( node );
    }

    void SceneBakeUniquify::_dealNodeMtr( MeshNode* node )
    {
        // 由于lightmap属性在instance（也就是node）中，所以不用在创建新的不同材质了，：）
#if 0
        // 创作material
        if ( !(m_config & SBU_UNIQ_MTR) )
            return;

        // 旧新映射表(本实例内允许材质重复共享)
        typedef map<Material*, MaterialPtr>::type MtrMap; // old mtr -> new mtr
        MtrMap mtrMap; 

        for ( int subIdx = 0; subIdx < node->getSubEntityCount(); ++subIdx )
        {
            Material* mtrOld = node->getSubEntity(subIdx)->getMaterial();
            mtrMap[mtrOld];
        }

        KHAOS_FOR_EACH( MtrMap, mtrMap, it )
        {
            Material* mtrOld = it->first;
            int& currId = m_mtrUsedMap[mtrOld];

            String mainName, extName;
            splitFileName( mtrOld->getName(), mainName, extName );
            String mtrNewName = mainName + "_" + intToString( currId ) + "." + extName;
            ++currId;

            Material* mtrNew = MaterialManager::createMaterial( mtrNewName );
            mtrNew->setNullCreator();
            mtrNew->copyFrom( mtrOld );
            it->second.attach( mtrNew );

            m_newMtrList.push_back( mtrNew );
        }
        
        // 修改
        for ( int subIdx = 0; subIdx < node->getSubEntityCount(); ++subIdx )
        {
            Material* mtrOld = node->getSubEntity(subIdx)->getMaterial();
            const MaterialPtr& mtrNew = mtrMap[mtrOld];
            node->getSubEntity(subIdx)->setMaterial( mtrNew->getName() ); // 实例修改
            node->getMesh()->getSubMesh(subIdx)->setMaterialName( mtrNew->getName() ); // mesh已经实例化，这里可以保存材质名
        }
#endif
    }

    void SceneBakeUniquify::_dealNodeMesh( MeshNode* node )
    {
        Mesh* meshOld = node->getMesh();
        int& currId = m_meshUsedMap[meshOld];

        String mainName, extName;
        splitFileName( meshOld->getName(), mainName, extName );
        if ( extName.empty() )
            extName = "mesh";
        String meshNewName = mainName + "_" + intToString( currId ) + "." + extName;
        ++currId;

        // 创作mesh
        MeshPtr meshCopy;

        if ( m_config & SBU_NEED_UV2 )
        {
            meshCopy.attach( _generalUV2( node, meshNewName ) );
        }
        else if ( m_config & (SBU_NEED_SH3|SBU_NEED_SH4) )
        {
            meshCopy.attach( _generalSH( meshNewName, meshOld ) );
        }

        // 修改node
        if ( meshCopy )
        {
            vector<MaterialPtr>::type mtrs; // 保持材质不变
            node->_holdTempMtr( mtrs );

            node->setMesh( meshCopy->getName() );
            m_newMeshList.push_back( meshCopy.get() );

            for ( size_t i = 0; i < mtrs.size(); ++i )
            {
                if ( mtrs[i] )
                    node->getSubEntity(i)->setMaterial( mtrs[i]->getName() );
            }
        }
    }

    Mesh* SceneBakeUniquify::_generalUV2( MeshNode* node, const String& meshNewName )
    {
        Mesh* meshOld = node->getMesh();
        const Matrix4& matWorld = node->getDerivedMatrix();
        NewMeshInfo& info = m_meshNewInfoMap[node->getName()];

        GeneralMeshUV gen( meshOld, matWorld );
        int texSize = GeneralMeshUV::AUTO_TEX_SIZE_HIGH;
        Mesh* meshOut = gen.apply( meshNewName, &texSize );

        info.newMeshName = meshNewName;
        info.bakeTextureWidth = texSize;
        info.bakeTextureHeight = texSize;

        m_totalUV2Area += texSize * texSize;
        int totalAreaSide = (int)Math::sqrt( (float)m_totalUV2Area );

        _outputDebugStr( "SceneBakeUniquify::_generalUV2: %s => %s, %d/%d\n", 
            node->getName().c_str(), meshNewName.c_str(), texSize, totalAreaSide );

        return meshOut;
    }

    Mesh* SceneBakeUniquify::_generalSH(  const String& name, Mesh* meshOld )
    {
        Mesh* meshCopy = MeshManager::createMesh( name );
        meshCopy->setNullCreator();
        meshCopy->copyFrom( meshOld );

        if ( m_config & SBU_NEED_SH3 )
            meshCopy->expandSH( 3 );
        else if ( m_config & SBU_NEED_SH4 )
            meshCopy->expandSH( 4 );

        return meshCopy;
    }


    //////////////////////////////////////////////////////////////////////////
    void SimpleBakeInputData::addBakeInfo( SceneNode* node, const NodeBakeInfo& info )
    {
        m_datas[node] = info;
    }

    const NodeBakeInfo* SimpleBakeInputData::getBakeInfo( SceneNode* node ) const
    {
        DataMap::const_iterator it = m_datas.find( node );
        if ( it != m_datas.end() )
            return &(it->second);

        khaosAssert(0);
        return 0;
    }


    //////////////////////////////////////////////////////////////////////////
    BakeSystem::BakeSystem() : m_sg(0), m_inputData(0), m_callback(0), m_config(0), m_threads(0)
    {
    }

    void BakeSystem::init( SceneGraph* sg, IBakeInputData* id, IBakeSystemCallback* cb, int config, int threads )
    {
        m_sg = sg;
        m_inputData = id;
        m_callback = cb;
        m_config = config;
        m_threads = threads;
    }

    void BakeSystem::general()
    {
        if ( !prepare() )
            return;

        bakeOnce();
    }

    bool BakeSystem::prepare()
    {
        _gatherAllNodes();
        
        if ( !_checkMeshNodes() )
            return false;

        _checkLitNodes();
        return true;
    }

    void BakeSystem::bakeOnce()
    {
        _bakeScene();
    }

    void BakeSystem::_gatherAllNodes()
    {
        // 收集场景中需要烘焙的模型，灯光等物件
        struct _GatherOp : public ITravelSceneGraphCallback
        {
            _GatherOp( BakeSystem* bs ) : m_bs(bs) {}

            virtual void onVisitNode( SceneNode* node )
            {
                if ( node->isEnabled() )
                {
                    if ( KHAOS_OBJECT_IS( node, MeshNode ) )
                        m_bs->m_meshNodes.push_back( node );
                    else if ( KHAOS_OBJECT_IS( node, LightNode ) )
                        m_bs->m_litNodes.push_back( node );
                    else if ( KHAOS_OBJECT_IS( node, VolumeProbeNode ) )
                        m_bs->m_volProbeNodes.push_back( node );
                }
            }

            BakeSystem* m_bs;
        };

        _GatherOp op(this);
        m_sg->travel( &op );
    }

    bool BakeSystem::_checkMeshNodes()
    {
        // 对收集到的mesh node做检查

        // 1.保证模型不重复
        set<Mesh*>::type records;

        for ( size_t i = 0; i < m_meshNodes.size(); ++i )
        {
            MeshNode* node = static_cast<MeshNode*>( m_meshNodes[i] );
            Mesh* mesh = node->getMesh();

            if ( !records.insert( mesh ).second )
                return false;
        }

        // 2.打开bvh，并给予其他准备工作
        for ( size_t i = 0; i < m_meshNodes.size(); ++i )
        {
            MeshNode* node = static_cast<MeshNode*>( m_meshNodes[i] );
            Mesh* mesh = node->getMesh();

            mesh->buildBVH( false );

            // 给予外部一些其他准备
            const NodeBakeInfo* info = m_inputData->getBakeInfo( node );
            m_callback->onPrepareMesh( node, info );
        }

        return true;
    }

    void BakeSystem::_checkLitNodes()
    {
        for ( size_t i = 0; i < m_litNodes.size(); ++i )
        {
            m_callback->onPrepareLight( m_litNodes[i] );
        }

        for ( size_t i = 0; i < m_volProbeNodes.size(); ++i )
        {
            m_callback->onPrepareVolumeProbe( m_volProbeNodes[i] );
        }
    }

    void BakeSystem::_bakeScene()
    {
        // 烘焙场景
        for ( size_t i = 0; i < m_meshNodes.size(); ++i )
        {
            _bakeNode( i, m_meshNodes[i] );
        }
    }

    void BakeSystem::_bakeNode( int idx, SceneNode* node )
    {
        const NodeBakeInfo* info = m_inputData->getBakeInfo( node );
        m_callback->onBeginBakeNode( idx, node, info );

        MeshNode* meshNode = static_cast<MeshNode*>( node );
        Mesh* mesh = meshNode->getMesh();
        const Matrix4& matWorld = meshNode->getDerivedMatrix();

        int pertype = _getPerBakeType();

        if ( pertype == BSF_PER_VERTEX )
        {
            _bakeNodePerVertex( info, mesh, matWorld );
        }
        else if ( pertype == BSF_PER_TEXEL )
        {
            _bakeNodePerTexel( info, mesh, matWorld );
        }

        m_callback->onEndBakeNode( node, info );
    }

    int BakeSystem::_getPerBakeType() const
    {
        int perType = m_config & BSF_PER_MASK;
        khaosAssert( perType == BSF_PER_VERTEX || perType == BSF_PER_TEXEL );
        return perType;
    }

    void BakeSystem::_bakeNodePerVertex( const NodeBakeInfo* info, Mesh* mesh, const Matrix4& matWorld )
    {
        BakeVertexProcess bvProcess( mesh, matWorld );
        m_callback->onSetupBakeVertexProcess( &bvProcess );

        // op
        struct _ProcessOp : public IBakeVertexProcessCallback
        {
            _ProcessOp( BakeSystem* bs ) : m_bs(bs) {}

            BakeSystem* m_bs;

            virtual void onPerVertex( int threadID, const Vector3& pos, const Vector3& norm, const Vector2& uv )
            {
                m_bs->m_callback->onBakePerVertex( threadID, pos, norm, uv );
            }
        };

        _ProcessOp op(this);
        bvProcess.general( &op, m_threads );
    }

    void BakeSystem::_bakeNodePerTexel( const NodeBakeInfo* info, Mesh* mesh, const Matrix4& matWorld )
    {
        LightMapProcess lmProcess( mesh, matWorld, info->textureWidth, info->textureHeight );
        m_callback->onSetupLightMapProcess( &lmProcess );

        // op
        struct _ProcessOp : public ILightMapProcessCallback
        {
            _ProcessOp( BakeSystem* bs ) : m_bs(bs) {}

            BakeSystem* m_bs;

            virtual void onPerSubMeshBegin( SubMesh* subMesh, int subIdx )
            {
                m_bs->m_callback->onBakePerSubMeshBegin( subMesh, subIdx );
            }

            virtual void onPerFaceBegin( int threadID, int faceIdx, const Vector3& faceTangent, const Vector3& faceBinormal, const Vector3& faceNormal )
            {
                return m_bs->m_callback->onBakePerFaceBegin( threadID, faceIdx, faceTangent, faceBinormal, faceNormal );
            }

            virtual int onDiscardTexel( int threadID, int xp, int yp )
            {
                return m_bs->m_callback->onBakeDiscardTexel( threadID, xp, yp );
            }

            virtual void onRegisterTexel( int threadID, int xp, int yp )
            {
                return m_bs->m_callback->onBakeRegisterTexel( threadID, xp, yp );
            }

            virtual void onPerTexel( int threadID, int xp, int yp, const Vector3& pos, const Vector3& norm, const Vector4& tang, const Vector2& uv )
            {
                m_bs->m_callback->onBakePerTexel( threadID, xp, yp, pos, norm, tang, uv );
            }

            virtual void onPerSubMeshEnd( SubMesh* subMesh )
            {
                m_bs->m_callback->onBakePerSubMeshEnd( subMesh );
            }
        };

        _ProcessOp op(this);
        lmProcess.general( &op, m_threads );
    }
}

