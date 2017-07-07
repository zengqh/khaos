#include "KhaosPreHeaders.h"
#include "KhaosObjImport.h"
#include "KhaosStrConv.h"
#include "KhaosSceneGraph.h"
#include "KhaosMeshManager.h"
#include "KhaosMaterialManager.h"
#include "KhaosTextureManager.h"
#include "KhaosTexCfgParser.h"
#include "KhaosMaterialFile.h"
#include "KhaosMeshFile.h"
#include "KhaosNameDef.h"
//#include "KhaosGlossyTool.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    bool ObjImporter::SubMesh::convert( ObjImporter::Mesh* mesh )
    {
        if ( m_indices.empty() )
            return false;

        VertexPNT vtx;
        int mappedIdx = 0;
        map<VertexIndex, int>::type newIdxMap;

        for ( size_t i = 0; i < m_indices.size(); ++i )
        {
            const VertexIndex& vi = m_indices[i];            
            map<VertexIndex, int>::type::iterator it = newIdxMap.find(vi);

            if ( it == newIdxMap.end() ) // 新的点
            {
                // vb
                vtx.pos() = mesh->m_posTemp[vi.pos];
                vtx.norm() = mesh->m_normalTemp[vi.normal];
                vtx.tex() = mesh->m_texTemp[vi.tex];

                m_vb.push_back( vtx );

                // ib
                mappedIdx = (int)newIdxMap.size();
                newIdxMap[vi] = mappedIdx; // 旧->新
            }
            else // 已经存在的点
            {
                mappedIdx = it->second;
            }

            m_ib.push_back( mappedIdx ); // 转换后的索引
        }

        return true;
    }

    void ObjImporter::SubMesh::buildFrom( const SubMeshList& smList )
    {
        this->m_material = smList.front()->m_material;

        int curVBCnt = 0;

        for ( size_t i = 0; i < smList.size(); ++i )
        {
            SubMesh* subMesh = smList[i];

            // 添加vb
            m_vb.insert( m_vb.end(), subMesh->m_vb.begin(), subMesh->m_vb.end() );

            // 添加ib，索引往后加
            for ( size_t j = 0; j < subMesh->m_ib.size(); ++j )
            {
                m_ib.push_back( subMesh->m_ib[j] + curVBCnt );
            }

            curVBCnt += (int)subMesh->m_vb.size();
        }
    }

    void ObjImporter::Mesh::genNewName()
    {
        static int nameIdx = 0;
        m_name = intToString( nameIdx );
        ++nameIdx;
    }

    bool ObjImporter::Mesh::convert()
    {
        for ( SubMeshList::iterator it = m_subMeshes.begin(); it != m_subMeshes.end(); )
        {
            SubMesh* sm = *it;

            if ( sm->convert( this ) )
            {
                ++it;
            }
            else
            {
                KHAOS_DELETE sm;
                it = m_subMeshes.erase(it);
            }
        }

        return m_subMeshes.size() > 0;
    }

    //////////////////////////////////////////////////////////////////////////
    ObjImporter::ObjImporter() : m_mesh(0), m_subMesh(0), m_mergeOne(false)
    {
    }

    ObjImporter::~ObjImporter()
    {
    }

    bool ObjImporter::parse( const String& objFile, IObjImportListener* listener )
    {
        FileSystem::DataBufferAuto objData;
        
        if ( !g_fileSystem->getFileData( objFile, objData ) )
            return false;

        return parse( objData, listener );
    }

    bool ObjImporter::parse( const DataBuffer& objData, IObjImportListener* listener )
    {
        int status = PSTATE_VB;
        int gpStat = PSTATE_GNAME;

        m_mesh = KHAOS_NEW Mesh;

        TxtStreamReader reader( (const char*)objData.data, objData.dataLen );
       
        String buff;
        StringVector items;

        while ( reader.getLine(buff) )
        {
            // skip
            if ( _isSkip(buff) )
                continue;

            trim(buff);

            // check
            splitString<String>( items, buff, " ", true );
            if ( items.size() < 2 )
            {
                khaosAssert(0);
                continue;
            }

            if ( _isMaterialDefine(items) )
                continue;

            // vb
_PARSE_VB_WORK:
            if ( status == PSTATE_VB )
            {
                if ( _isPos(items) )
                    continue;

                if ( _isNormal(items) )
                    continue;

                if ( _isTex(items) )
                    continue;

                status = PSTATE_GROUP;
            }
           
            // group
            if ( status == PSTATE_GROUP )
            {
                if ( _isGroupName(items) )
                    continue;

                if ( _isMaterialName(items) )
                    continue;

                if ( _isFace(items) )
                    continue;

                if ( m_mergeOne ) // 需要合并
                {
                    //m_meshAll.push_back( m_mesh ); // 保存
                    //m_mesh = KHAOS_NEW Mesh; // 创建新的
                    // 什么都不用干，继续用这个mesh
                }
                else // 不需要合并，有一个是一个
                {
                    if ( m_mesh->convert() ) // 收集完毕创建一个mesh
                    {
                        m_mesh->genNewName();
                        listener->onCreateMesh( m_mesh );
                    }
                    
                    m_mesh->clearSubMeshes(); // 清除临时，再次从vb开始
                }
                

                // test
#if 0
                static int ii = 0;
                ++ii;
                if ( ii > 10 )
                {
                    break;
                }
#endif
                status = PSTATE_VB;
                goto _PARSE_VB_WORK; // 依然继续vb这行处理
            }

            // unknown
            khaosAssert(0);
        }

        //////////////////////////////////////////////////////////////////////////
        // 结束工作
        if ( m_mergeOne ) // 需要合并
        {
            m_meshAll.push_back( m_mesh ); // 保存
            _mergeAll();
            listener->onCreateMesh( m_mesh );
        }
        else // 不需要合并
        {
            // 文件读完最后一个转换
            if ( m_mesh->convert() ) // 收集完毕创建一个mesh
            {
                m_mesh->genNewName();
                listener->onCreateMesh( m_mesh );
            }
            
            m_mesh->clearSubMeshes(); // 清除临时，再次从vb开始
        }
        
        // 释放
        for ( size_t i = 0; i < m_meshAll.size(); ++i )
        {
            Mesh* mesh = m_meshAll[i];
            KHAOS_DELETE mesh;
        }

        KHAOS_DELETE m_mesh; // 一个个转删除一个够了
        return true;
    }

    void ObjImporter::_mergeAll()
    {
        // 将m_meshAll合并为m_mesh一个

        // step1.转为标准vertex
        for ( size_t i = 0; i < m_meshAll.size(); ++i )
        {
            Mesh* mesh = m_meshAll[i];
            mesh->convert(); // 收集完毕创建一个mesh
        }

        // step2. 归类相同材质sub mesh
        MtrSubMeshsMap msmMap;
        for ( size_t i = 0; i < m_meshAll.size(); ++i )
        {
            Mesh* mesh = m_meshAll[i];
            for ( size_t si = 0; si < mesh->m_subMeshes.size(); ++si )
            {
                SubMesh* subMesh = mesh->m_subMeshes[si];
                msmMap[subMesh->m_material].push_back( subMesh );
            }
        }

        // step3.材质一样合并
        m_mesh = KHAOS_NEW Mesh; // 创建新的
        m_mesh->m_name = "all";

        for ( MtrSubMeshsMap::iterator it = msmMap.begin(), ite = msmMap.end(); it != ite; ++it )
        {
            SubMesh* subMesh = m_mesh->createSub();
            subMesh->buildFrom( it->second );
        }
    }

    bool ObjImporter::_isSkip( const String& str ) const
    {
        if ( str.empty() )
            return true;

        switch ( str[0] )
        {
        case '#':
        case 's':
        case '\n':
        case '\r':
            return true;
        }

        return false;
    }

    bool ObjImporter::_isPos( const StringVector& items )
    {
        if ( items[0] != "v" )
            return false;

        m_mesh->m_posTemp.push_back( strsToVector3( &items[1] ) );
        return true;
    }

    bool ObjImporter::_isNormal( const StringVector& items )
    {
        if ( items[0] != "vn" )
            return false;

        m_mesh->m_normalTemp.push_back( strsToVector3( &items[1] ) );
        return true;
    }

    static float _flipV( float v )
    {
        //if ( v <= 1.0f+0.01f )
            return 1.0f - v;

        //float v_up = Math::ceil( v );
        //return v_up - v;
    }

    bool ObjImporter::_isTex( const StringVector& items )
    {
        if ( items[0] != "vt" )
            return false;

        Vector2 uv = strsToVector2( &items[1] );
        uv.y = _flipV( uv.y );
        m_mesh->m_texTemp.push_back( uv );
        return true;
    }

    bool ObjImporter::_isMaterialDefine( const StringVector& items )
    {
        if ( items[0] != "mtllib" )
            return false;

        return true;
    }

    bool ObjImporter::_isMaterialName( const StringVector& items )
    {
        if ( items[0] != "usemtl" )
            return false;

        m_subMesh = m_mesh->createSub();
        m_subMesh->m_material = items[1];
        return true;
    }

    bool ObjImporter::_isGroupName( const StringVector& items )
    {
        if ( items[0] != "g" )
            return false;

        m_subMesh = m_mesh->createSub();
        // no material
        return true;
    }

    bool ObjImporter::_isFace( const StringVector& items )
    {
        if ( items[0] != "f" )
            return false;

        int cnt = (int)items.size() - 1;
        khaosAssert( cnt == 3 || cnt == 4 );

        VertexIndex vi[4];
        StringVector vistr;

        for ( int i = 0; i < cnt; ++i )
        {
            splitString( vistr, items[i+1], "/", false );
            khaosAssert( vistr.size() == 3 );
            
            vi[i].pos = stringToInt( vistr[0].c_str() ) - 1;
            vi[i].tex = stringToInt( vistr[1].c_str() ) - 1;
            vi[i].normal = stringToInt( vistr[2].c_str() ) - 1;

            khaosAssert( 0 <= vi[i].pos && vi[i].pos < (int)m_mesh->m_posTemp.size() );
            khaosAssert( 0 <= vi[i].normal && vi[i].normal < (int)m_mesh->m_normalTemp.size() );
            khaosAssert( 0 <= vi[i].tex && vi[i].tex < (int)m_mesh->m_texTemp.size() );
        }
        
        if ( cnt == 3 )
        {
            m_subMesh->m_indices.push_back( vi[0] );
            m_subMesh->m_indices.push_back( vi[1] );
            m_subMesh->m_indices.push_back( vi[2] );
        }
        else
        {
            m_subMesh->m_indices.push_back( vi[0] );
            m_subMesh->m_indices.push_back( vi[1] );
            m_subMesh->m_indices.push_back( vi[2] );

            m_subMesh->m_indices.push_back( vi[2] );
            m_subMesh->m_indices.push_back( vi[3] );
            m_subMesh->m_indices.push_back( vi[0] );
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    ObjSceneImporter::ObjSceneImporter() : m_parentNode(0), m_castShadow(false), m_receShadow(false),
        m_saveMesh(true), m_mergeOne(false)
    {
    }

    void ObjSceneImporter::import( const String& objFile, SceneNode* parentNode )
    {
        m_parentNode = parentNode;

        ObjImporter oi;
        oi.setMergeOne( m_mergeOne );
        oi.parse( objFile, this );
    }

    void ObjSceneImporter::import( const DataBuffer& objData, SceneNode* parentNode )
    {
        m_parentNode = parentNode;

        ObjImporter oi;
        oi.setMergeOne( m_mergeOne );
        oi.parse( objData, this );
    }

    void ObjSceneImporter::onCreateMesh( void* objMesh )
    {
        MeshPtr mesh = _convertMesh( objMesh );

        if ( m_parentNode )
        {
            MeshNode* node = m_parentNode->getSceneGraph()->createSceneNode<MeshNode>( mesh->getName() );
            node->setMesh( mesh->getName() );
            node->setCastShadow( m_castShadow );
            node->setReceiveShadow( m_receShadow );
            m_parentNode->addChild( node );
        }
    }

    MeshPtr ObjSceneImporter::_convertMesh( void* objMesh1 )
    {
        ObjImporter::Mesh* objMesh =  (ObjImporter::Mesh*) objMesh1;

        String meshName = m_meshPreName + objMesh->m_name + ".mesh";
        MeshPtr mesh( MeshManager::createMesh( meshName ) );

        mesh->setNullCreator();

        for ( size_t i = 0; i < objMesh->m_subMeshes.size(); ++i )
        {
            ObjImporter::SubMesh* objSM = objMesh->m_subMeshes[i];
            SubMesh* sm = mesh->createSubMesh();
            sm->setPrimitiveType( PT_TRIANGLELIST );
            
            VertexBuffer* vb = sm->createVertexBuffer();
            vb->create( objSM->m_vb.size() * sizeof(objSM->m_vb[0]), HBU_STATIC );
            vb->fillData( &objSM->m_vb[0] );
            vb->setDeclaration( g_vertexDeclarationManager->getDeclaration(VertexPNT::ID) );

            IndexBuffer* ib = sm->createIndexBuffer();
            ib->create( objSM->m_ib.size() * sizeof(int), HBU_STATIC, IET_INDEX32 );
            ib->fillData( &objSM->m_ib[0] );

            String mtrName;
            if ( objSM->m_material.size() )
                mtrName = m_resBase + objSM->m_material + ".mtr";
            else
                mtrName = m_mtrDefaultName;
            sm->setMaterialName( mtrName );            
        }

        mesh->updateAABB( true );

        if ( m_saveMesh )
        {
            MeshExporter meshExp;
            String file = g_resourceManager->getGroup<Mesh>()->getAutoCreator()->getLocateFileName( meshName );
            meshExp.exportMesh( file, mesh.get() );
        }
  
        return mesh;
    }

    //////////////////////////////////////////////////////////////////////////
    ObjMtlImporter::ObjMtlImporter() : m_listener(0), m_mtr(0)
    {
    }

    ObjMtlImporter::~ObjMtlImporter()
    {
    }

    bool ObjMtlImporter::parse( const String& objFile, IObjMtlImportListener* listener )
    {
        FileSystem::DataBufferAuto objData;

        if ( !g_fileSystem->getFileData( objFile, objData ) )
            return false;

        return parse( objData, listener );
    }

    bool ObjMtlImporter::parse( const DataBuffer& objData, IObjMtlImportListener* listener )
    {
        m_listener = listener;

        TxtStreamReader reader( (const char*)objData.data, objData.dataLen );

        String buff;
        StringVector items;
        bool isFirst = true;

        while ( reader.getLine(buff) )
        {
            // skip
            if ( _isSkip(buff) )
                continue;

            trim(buff);

            // check
            splitString<String>( items, buff, " ", true );
            if ( items.size() < 2 )
            {
                khaosAssert(0);
                continue;
            }

            if ( _isMaterial( items ) )
                continue;

            if ( _isClrDiff( items ) )
                continue;

            if ( _isClrSpec( items ) )
                continue;

            if ( _isSpecPower( items ) )
                continue;

            if ( _isMapDiff( items ) )
                continue;

            if ( _isMapSpec( items ) )
                continue;

            if ( _isMapOpacity( items ) )
                continue;

            if ( _isMapBump( items ) )
                continue;

            if ( _isMapBakedAO( items ) )
                continue;
        }

        _completeMtr();
        return true;
    }

    void ObjMtlImporter::_completeMtr()
    {
        if ( m_mtr )
        {
            m_listener->onGetObjMtl( m_mtr );
            KHAOS_DELETE m_mtr;
            m_mtr = 0;
        }
    }

    bool ObjMtlImporter::_isSkip( const String& str ) const
    {
        if ( str.empty() )
            return true;

        switch ( str[0] )
        {
        case '#':
        case '\n':
        case '\r':
            return true;
        }

        return false;
    }

    bool ObjMtlImporter::_isMaterial( const StringVector& items )
    {
        if ( items[0] != "newmtl" )
            return false;

        _completeMtr();

        m_mtr = KHAOS_NEW Material;
        m_mtr->name = items[1];
        
        return true;
    }

    bool ObjMtlImporter::_isClrDiff( const StringVector& items )
    {
        if ( items[0] != "Kd" )
            return false;
        
        khaosAssert( items.size() == 4 );
        m_mtr->diff = strsToColor( &items[1], 3 );
        return true;
    }

    bool ObjMtlImporter::_isClrSpec( const StringVector& items )
    {
        if ( items[0] != "Ks" )
            return false;

        khaosAssert( items.size() == 4 );
        m_mtr->spec = strsToColor( &items[1], 3 );
        return true;
    }

    bool ObjMtlImporter::_isSpecPower( const StringVector& items )
    {
        if ( items[0] != "Ns" )
            return false;

        khaosAssert( items.size() == 2 );
        m_mtr->power = stringToFloat( items[1].c_str() );
        return true;
    }

    bool ObjMtlImporter::_isMapItem( const StringVector& items, String& item, const char* name )
    {
        if ( items[0] != name )
            return false;

        khaosAssert( items.size() == 2 );
        item = items[1];
        return true;
    }

    bool ObjMtlImporter::_isMapDiff( const StringVector& items )
    {
        return _isMapItem( items, m_mtr->mapDiff, "map_Kd" );
    }

    bool ObjMtlImporter::_isMapSpec( const StringVector& items )
    {
        return _isMapItem( items, m_mtr->mapSepc, "map_Ks" );
    }

    bool ObjMtlImporter::_isMapBump( const StringVector& items )
    {
        return _isMapItem( items, m_mtr->mapBump, "map_bump" );
    }

    bool ObjMtlImporter::_isMapOpacity( const StringVector& items )
    {
        return _isMapItem( items, m_mtr->mapOpacity, "map_d" );
    }

    bool ObjMtlImporter::_isMapBakedAO( const StringVector& items )
    {
        return _isMapItem( items, m_mtr->mapBakedAO, "map_ao" );
    }

    //////////////////////////////////////////////////////////////////////////
    void ObjMtlImporterBase::import( const String& objMtlFile )
    {
        ObjMtlImporter oi;
        oi.parse( objMtlFile, this );
    }

    void ObjMtlImporterBase::import( const DataBuffer& objMtlData )
    {
        ObjMtlImporter oi;
        oi.parse( objMtlData, this );
    }

    ObjMtlResImporter::ObjMtlResImporter()
    {
        setMethod( IMT_SPECULARMAP_ARBITRARY );
        setDiffMapArbiVal( Color(1.0f, 1.0f, 1.0f) );
        setSpecMapArbiVal( 1.0f );
        setOpacityMapVal( 4, 0.4f );
        setPBRVal( Color(0.5f, 0.5f, 0.5f), 0.0f, 0.5f, 0.5f, true, false, true );
    }

    String ObjMtlResImporter::_getAbsFile( ClassType clsType, const String& name, String& resName )
    {
        resName = m_resBase + name;
        String resFullName = g_resourceManager->getGroup(clsType)->getAutoCreator()->getLocateFileName( resName );
        return g_fileSystem->getFullFileName( resFullName );
    }

    void ObjMtlResImporter::_convertTexture( const String& mapDiff, bool srgb, int mipSize )
    {
        String texName;
        String absFile = _getAbsFile( KHAOS_CLASS_TYPE(Texture), mapDiff, texName );

        TexturePtr texTmp( TextureManager::createTexture(texName) );
        if ( !texTmp )
            return;

        texTmp->setNullCreator();

        TexObjCreateParas paras;
        paras.width  = 64; // 随便设，宽和高不会记录在配置文件内
        paras.height = 64;
        texTmp->createTex( paras );
        texTmp->setSRGB( srgb );

        TexCfgSaver saver;
        saver.save( texTmp.get(), absFile, mipSize );
    }

    void ObjMtlResImporter::_simpleSetTexture( const String& texMapName, bool srgb, MaterialPtr& mtrDest, int attrID, int mipSize )
    {
        if ( texMapName.size() )
        {
            String texName;
            _getAbsFile( KHAOS_CLASS_TYPE(Texture), texMapName, texName );

            if ( g_fileSystem->isExist(texName) )
            {
                _convertTexture( texMapName, srgb, mipSize );
                static_cast<TextureAttribBase*>( mtrDest->useAttrib(attrID) )->setTexture( texName );
            }
            else
            { 
                khaosLogLn( KHL_L3, "[ObjMtlResImporter::onGetObjMtl] Not found texture: %s", texName.c_str() );
            }
        }
    }

    void _oldMtrToNewMtr( const Color& oldDiffuse, const Color& oldSpecular, float oldPower,
            Color& newBase, float& newMetallic, float& newDSpecular, float& newRoughness )
    {
        // NB: 这只是个简单转换，非严格处理
        if ( oldSpecular.isChromatic() )
        {
            // 彩色的高光当纯金属
            newBase = oldSpecular;
            newMetallic = 0.9f;
            newDSpecular = 0.1f;
        }
        else // 单色的高光当纯绝缘体
        {
            newBase = oldDiffuse;
            newMetallic = 0;
            newDSpecular = oldSpecular.g;
        }

        // power => glossy
        oldPower = Math::clamp( oldPower, 2.0f, 2048.0f );
        float glossy = (Math::log2(oldPower) - 1) * 0.1f;
        newRoughness = (1 - glossy); // * 故意的，看看效果
    }

    void ObjMtlResImporter::onGetObjMtl( void* mtl )
    {
        ObjMtlImporter::Material* mtrSrc = (ObjMtlImporter::Material*)(mtl);
   
        String mtrName;
        /*String absFile =*/ _getAbsFile( KHAOS_CLASS_TYPE(Material), mtrSrc->name + ".mtr", mtrName );

        MaterialPtr mtrDest( MaterialManager::createMaterial( mtrName ) );
        if ( !mtrDest )
            return;

        mtrDest->setNullCreator();
        
        Color newBase;
        float newMetallic, newDSpecular, newRoughness;

        if ( m_method & IMT_PBRMTR )
        {
            newBase      = m_defaultBaseClr;
            newMetallic  = m_defaultMetallic;
            newDSpecular = m_defaultDSpecular;
            newRoughness = m_defaultRoughness;
        }
        else
        {
            _oldMtrToNewMtr( mtrSrc->diff, mtrSrc->spec, mtrSrc->power,
                newBase, newMetallic, newDSpecular, newRoughness );
        }

        mtrDest->useAttrib<BaseColorAttrib>()->setValue( newBase );
        mtrDest->useAttrib<MetallicAttrib>()->setValue( newMetallic );
        mtrDest->useAttrib<DSpecularAttrib>()->setValue( newDSpecular );
        mtrDest->useAttrib<RoughnessAttrib>()->setValue( newRoughness );

        // diffuse map, diffuse图假定是srgb
        _simpleSetTexture( mtrSrc->mapDiff, true, mtrDest, BaseMapAttrib::ID, TexObjLoadParas::MIP_AUTO );

        if ( BaseMapAttrib* baseMapAttr = mtrDest->getAttrib<BaseMapAttrib>() )
        {
            if ( m_method & IMT_DIFFUSEMAP_ARBITRARY ) // 武断的方法
                mtrDest->useAttrib<BaseColorAttrib>()->setValue( m_diffMapArbiVal );
            else if ( m_method & IMT_PBRMTR )
                mtrDest->useAttrib<BaseColorAttrib>()->setValue( Color::WHITE );
        }
        
        // spec map 假定srgb
        bool specSRGB = m_method & IMT_PBRMTR ? false : true;
        _simpleSetTexture( mtrSrc->mapSepc, specSRGB, mtrDest, SpecularMapAttrib::ID, TexObjLoadParas::MIP_AUTO );
        
        if ( SpecularMapAttrib* specMapAttr = mtrDest->getAttrib<SpecularMapAttrib>() )
        {
            if ( m_method & IMT_PBRMTR )
            {
                if ( m_usedMetallic )
                {
                    mtrDest->useAttrib<MetallicAttrib>()->setValue( 1.0f );
                    specMapAttr->setMetallicEnabled( true );
                }

                if ( m_usedDSpecular )
                {
                    mtrDest->useAttrib<DSpecularAttrib>()->setValue( 1.0f );
                    specMapAttr->setDSpecularEnabled( true );
                }

                if ( m_usedRoughness )
                {
                    mtrDest->useAttrib<RoughnessAttrib>()->setValue( 1.0f );
                    specMapAttr->setRoughnessEnabled( true );
                }
            }
            else
            {
                // 我们只考虑非金属的高光图，使用g通道
                specMapAttr->setDSpecularEnabled( true );

                if ( m_method & IMT_SPECULARMAP_ARBITRARY ) // 武断的方法
                {
                    mtrDest->useAttrib<DSpecularAttrib>()->setValue( m_specMapArbiVal ); // 把之前的忽略掉，始终1
                    //mtrDest->useAttrib<RoughnessAttrib>()->setValue( 0.25f ); // test 
                }
            }
        }

        // opacity map
        _simpleSetTexture( mtrSrc->mapOpacity, false, mtrDest, OpacityMapAttrib::ID, m_opacityMapMips );
        if ( mtrDest->getAttrib<OpacityMapAttrib>() )
        {
            mtrDest->useAttrib<AlphaTestAttrib>()->setValue( m_opacityMapTestRef );
        }

        // normal map, normal map图假定是线性空间
        _simpleSetTexture( mtrSrc->mapBump, false, mtrDest, NormalMapAttrib::ID, TexObjLoadParas::MIP_AUTO );

        // baked ao map
        bool bakedAOSRGB = true;
        _simpleSetTexture( mtrSrc->mapBakedAO, bakedAOSRGB, mtrDest, BakedAOMapAttrib::ID, TexObjLoadParas::MIP_AUTO );

        // 调整roughness
        //GlossyAATool tool;
        //tool.build( mtrDest.get() );

        MaterialExporter mtrExp;
        mtrExp.exportMaterial( mtrDest->getResFileName()/*absFile*/, mtrDest.get() );

        m_mtrMap[ mtrDest->getName() ] = mtrDest;
    }
}

