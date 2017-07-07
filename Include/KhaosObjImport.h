#pragma once
#include "KhaosFileSystem.h"
#include "KhaosStringUtil.h"
#include "KhaosVector2.h"
#include "KhaosVector3.h"
#include "KhaosVertexIndexBuffer.h"
#include "KhaosMesh.h"
#include "KhaosMaterial.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    struct IObjImportListener
    {
        virtual void onCreateMesh( void* mesh ) = 0;
    };

    class ObjImporter : public AllocatedObject
    {
    public:
        enum
        {
            PSTATE_VB,
            PSTATE_GROUP,

            PSTATE_GNAME,
            PSTATE_GMTR,
            PSTATE_GFACE
        };

        typedef vector<int>::type IntArray;
        typedef vector<Vector2>::type Vector2Array;
        typedef vector<Vector3>::type Vector3Array;
        typedef vector<VertexPNT>::type VertexArray;

        struct VertexIndex
        {
            VertexIndex() : pos(0), normal(0), tex(0) {}
            VertexIndex( int p, int n, int t ) : pos(p), normal(n), tex(t) {}

            bool operator<( const VertexIndex& rhs )const
            {
                return memcmp( this, &rhs, sizeof(int)*3 ) < 0;
            }

            int pos;
            int normal;
            int tex;
        };

        typedef vector<VertexIndex>::type VertexIndexArray;

        struct Mesh;
        struct SubMesh;

        typedef vector<SubMesh*>::type SubMeshList;

        struct SubMesh : public AllocatedObject
        {
            bool convert( Mesh* mesh );
            void buildFrom( const SubMeshList& smList );

            String m_material;
            VertexIndexArray m_indices;
            VertexArray m_vb;
            IntArray m_ib;
        };

        struct Mesh : public AllocatedObject
        {
            ~Mesh()
            {
                clearSubMeshes();
            }

            void genNewName();

            SubMesh* createSub()
            {
                SubMesh* sm = KHAOS_NEW SubMesh;
                m_subMeshes.push_back(sm);
                return sm;
            }

            void clearSubMeshes()
            {
                for ( size_t i = 0; i < m_subMeshes.size(); ++i )
                    KHAOS_DELETE m_subMeshes[i];
                m_subMeshes.clear();
            }

            bool convert();

            String       m_name;

            Vector3Array m_posTemp;
            Vector3Array m_normalTemp;
            Vector2Array m_texTemp;

            SubMeshList  m_subMeshes;
        };
       
        typedef vector<Mesh*>::type MeshList;
        typedef map<String, SubMeshList>::type MtrSubMeshsMap;

    public:
        ObjImporter();
        ~ObjImporter();

        bool parse( const String& objFile, IObjImportListener* listener );
        bool parse( const DataBuffer& objData, IObjImportListener* listener );

        void setMergeOne( bool b ) { m_mergeOne = b; }

    private:
        void _parseVB();

        bool _isSkip( const String& str ) const;

        bool _isPos( const StringVector& items );
        bool _isNormal( const StringVector& items );
        bool _isTex( const StringVector& items );

        bool _isMaterialDefine( const StringVector& items );
        bool _isMaterialName( const StringVector& items );
        bool _isGroupName( const StringVector& items );
        bool _isFace( const StringVector& items );

        void _mergeAll();

    private:
        Mesh*               m_mesh;
        SubMesh*            m_subMesh;
        MeshList            m_meshAll;
        bool                m_mergeOne;
    };

    //////////////////////////////////////////////////////////////////////////
    class SceneGraph;
    class SceneNode;

    class ObjSceneImporter : public AllocatedObject, public IObjImportListener
    {
    public:
        ObjSceneImporter();

        void setMergeOne( bool b ) { m_mergeOne = b; }
        void setDefaultMaterialName( const String& name ) { m_mtrDefaultName = name; }
        void setMeshPreName( const String& name ) { m_meshPreName = name; }
        void setResBase( const String& path ) { m_resBase = normalizePath(path); }
        void setShadow( bool cast, bool rece ) { m_castShadow = cast; m_receShadow = rece; }

        void import( const String& objFile, SceneNode* parentNode );
        void import( const DataBuffer& objData, SceneNode* parentNode );

        void setSaveMesh( bool save )  { m_saveMesh = save; }

    private:
        virtual void onCreateMesh( void* mesh );

        MeshPtr _convertMesh( void* objMesh );

    private:
        SceneNode*  m_parentNode;
        String      m_meshPreName;
        String      m_mtrDefaultName;
        String      m_resBase;
        bool        m_castShadow;
        bool        m_receShadow;
        bool        m_saveMesh;
        bool        m_mergeOne;
    };

    //////////////////////////////////////////////////////////////////////////
    struct IObjMtlImportListener
    {
        virtual void onGetObjMtl( void* mtl ) = 0;
    };

    class ObjMtlImporter : public AllocatedObject
    {
    public:
        struct Material : public AllocatedObject
        {
            Material() : power(0) {}

            String  name;
            Color   diff;
            Color   spec;
            float   power;
            String  mapDiff;
            String  mapSepc;
            String  mapBump;
            String  mapOpacity;
            String  mapBakedAO;
        };

    public:
        ObjMtlImporter();
        ~ObjMtlImporter();

        bool parse( const String& objFile, IObjMtlImportListener* listener );
        bool parse( const DataBuffer& objData, IObjMtlImportListener* listener );

    private:
        bool _isSkip( const String& str ) const;

        bool _isMaterial( const StringVector& items );
        bool _isClrDiff( const StringVector& items );
        bool _isClrSpec( const StringVector& items );
        bool _isSpecPower( const StringVector& items );

        bool _isMapItem( const StringVector& items, String& item, const char* name );
        bool _isMapDiff( const StringVector& items );
        bool _isMapSpec( const StringVector& items );
        bool _isMapBump( const StringVector& items );
        bool _isMapOpacity( const StringVector& items );
        bool _isMapBakedAO( const StringVector& items );

        void _completeMtr();

    private:
        IObjMtlImportListener* m_listener;
        Material* m_mtr;
    };

    //////////////////////////////////////////////////////////////////////////
    class ObjMtlImporterBase : public AllocatedObject, public IObjMtlImportListener
    {
    public:
        void setResBase( const String& path ) { m_resBase = normalizePath(path); }

        void import( const String& objMtlFile );
        void import( const DataBuffer& objMtlData );

    protected:
        String  m_resBase;
    };

    class ObjMtlResImporter : public ObjMtlImporterBase
    {
    public:
        enum 
        {
            IMT_DIFFUSEMAP_ARBITRARY  = 0x1,
            IMT_SPECULARMAP_ARBITRARY = 0x2,
            IMT_PBRMTR                = 0x4
        };

        typedef map<String, MaterialPtr>::type MtrMap;

    public:
        ObjMtlResImporter();

        void setMethod( uint val ) { m_method = val; }
        void setDiffMapArbiVal( const Color& val ) { m_diffMapArbiVal = val; }
        void setSpecMapArbiVal( float val ) { m_specMapArbiVal = val; }

        void setOpacityMapVal( int mips, float testRef ) 
        {
            m_opacityMapMips = mips;
            m_opacityMapTestRef = testRef;
        }

        void setPBRVal( const Color& baseClr, float m, float s, float r,
            bool usedMetallic, bool usedDSpecular, bool usedRoughness )
        {
            m_defaultBaseClr   = baseClr;
            m_defaultMetallic  = m;
            m_defaultDSpecular = s;
            m_defaultRoughness = r;

            m_usedMetallic  = usedMetallic;
            m_usedDSpecular = usedDSpecular;
            m_usedRoughness = usedRoughness;
        }

    public:
        MtrMap& getMtrMap() { return m_mtrMap; }

    private:
        virtual void onGetObjMtl( void* mtl );

        String _getAbsFile( ClassType clsType, const String& name, String& resName );
        void _convertTexture( const String& mapDiff, bool srgb, int mipSize );
        void _simpleSetTexture( const String& texMapName, bool srgb, MaterialPtr& mtrDest, int attrID, int mipSize );

    private:
        MtrMap m_mtrMap;
        uint   m_method;

        Color  m_diffMapArbiVal;
        float  m_specMapArbiVal;

        int    m_opacityMapMips;
        float  m_opacityMapTestRef;

        Color  m_defaultBaseClr;
        float  m_defaultMetallic;
        float  m_defaultDSpecular;
        float  m_defaultRoughness;
        bool   m_usedMetallic;
        bool   m_usedDSpecular;
        bool   m_usedRoughness;
    };
}

