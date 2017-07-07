#pragma once
#include "KhaosTexture.h"

namespace Khaos
{
    enum
    {
        InvalidInstSharedDataID = -1
    };

    //////////////////////////////////////////////////////////////////////////
    class EnvProbeData : public AllocatedObject
    {
    public:
        enum
        {
            STATIC_DIFFUSE    = 0x1,
            STATIC_SH_DIFFUSE = 0x2,
            DYNAMIC_DIFFUSE   = 0x3,

            STATIC_SPECULAR   = 0x1 << 8,
            DYNAMIC_SPECULAR  = 0x2 << 8
        };

    public:
        EnvProbeData();

        void setType( uint type );
        uint getType() const { return m_type; }

        uint getDiffuseType() const { return m_type & 0xFF; }
        uint getSpecularType() const { return m_type & 0xFF00; }

        uint getPreparedDiffuseType() const;
        uint getPreparedSpecularType() const;

        void setDiffuseName( const String& name );
        const String& getDiffuseName() const { return m_nameDiffuse; }
        Texture* getDiffuseTex() const;

        void setSpecularName( const String& name );
        const String& getSpecularName() const { return m_nameSpecular; }
        Texture* getSpecularTex() const;

    private:
        uint       m_type;

        String     m_nameDiffuse;
        TexturePtr m_texDiffuse;

        String     m_nameSpecular;
        TexturePtr m_texSpecular;
    };

    //////////////////////////////////////////////////////////////////////////
    class TexturePairItem : public AllocatedObject
    {
    public:
        void setName( const String& name );
        const String& getName() const { return m_name; }

        Texture* getTexture() const;
        Texture* getPreparedTexture() const;

    private:
        String m_name;
        TexturePtr m_tex;
    };

    //////////////////////////////////////////////////////////////////////////
    class LightmapItem : public AllocatedObject
    {
    public:
        enum
        {
            LMT_AO,
            LMT_BASIC,
            LMT_FULL,
        };

    public:
        LightmapItem() : m_type(0) {}

        TexturePairItem& getMap() { return m_map; }
        TexturePairItem& getMapB() { return m_mapB; }

        void setType( int t ) { m_type = t; }
        int  getType() const { return m_type; }

        bool isPrepared() const;

    private:
        int             m_type;
        TexturePairItem m_map;
        TexturePairItem m_mapB;
    };

    //////////////////////////////////////////////////////////////////////////
    class VolumeProbeData : public AllocatedObject
    {
    public:
        void load( const String& files );
        void unload();

        TexturePairItem& getMapR() { return m_mapR; }
        TexturePairItem& getMapG() { return m_mapG; }
        TexturePairItem& getMapB() { return m_mapB; }

        bool isPrepared() const;

    private:
        TexturePairItem m_mapR;
        TexturePairItem m_mapG;
        TexturePairItem m_mapB;
    };

    //////////////////////////////////////////////////////////////////////////
    class InstanceSharedDataMgr : public AllocatedObject
    {
        typedef vector<LightmapItem*>::type LightMapItems;
        typedef vector<EnvProbeData*>::type EnvProbeMap;
        typedef vector<VolumeProbeData*>::type VolumeProbeMap;

    public:
        ~InstanceSharedDataMgr();

        LightmapItem* createLightmap( int id );
        LightmapItem* getLightmap( int id ) const;
        void          removeLightmap( int id );

        EnvProbeData* createEnvProbe( int id );
        EnvProbeData* getEnvProbe( int id ) const;
        void          removeEnvProbe( int id );

        VolumeProbeData* createVolumeProbe( int id );
        VolumeProbeData* getVolumeProbe( int id ) const;
        void             removeVolumeProbe( int id );

        void clean();

    private:
        template<class T>
        T* _createObj( typename vector<T*>::type& container, int id );

        template<class T>
        T* _getObj( const typename vector<T*>::type& container, int id ) const;

        template<class T>
        void _removeObj( typename vector<T*>::type& container, int id );

        template<class T>
        void _clearAllObjs( typename vector<T*>::type& container );

    private:
        LightMapItems  m_lightMaps;
        EnvProbeMap    m_envProbeMap;
        VolumeProbeMap m_volProbeMap;
    };
}

