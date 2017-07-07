#include "KhaosPreHeaders.h"
#include "KhaosInstObjSharedData.h"
#include "KhaosTextureManager.h"
#include "KhaosNameDef.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    EnvProbeData::EnvProbeData() : m_type(0)
    {
    }

    void EnvProbeData::setType( uint type )
    {
        m_texDiffuse.release();
        m_texSpecular.release();
        m_type = type;
    }

    uint EnvProbeData::getPreparedDiffuseType() const
    {
        uint type = getDiffuseType();

        if ( type == STATIC_DIFFUSE )
        {
            if ( !m_texDiffuse || !m_texDiffuse->isLoaded() )
                type = 0;
        }

        return type;
    }

    uint EnvProbeData::getPreparedSpecularType() const
    {
        uint type = getSpecularType();

        if ( type == STATIC_SPECULAR )
        {
            if ( !m_texSpecular || !m_texSpecular->isLoaded() )
                type = 0;
        }

        return type;
    }

    void EnvProbeData::setDiffuseName( const String& name )
    {
        m_texDiffuse.release();
        m_nameDiffuse = name;

        if ( getDiffuseType() == STATIC_DIFFUSE )
            _bindResourceRoutine( m_texDiffuse, m_nameDiffuse, 0 );
    }

    Texture* EnvProbeData::getDiffuseTex() const
    {
        switch ( getDiffuseType() )
        {
        case STATIC_DIFFUSE:
            return m_texDiffuse.get();

        case DYNAMIC_DIFFUSE:
            return 0;
        }

        return 0;
    }

    void EnvProbeData::setSpecularName( const String& name )
    {
        m_texSpecular.release();
        m_nameSpecular = name;

        if ( getSpecularType() == STATIC_SPECULAR )
            _bindResourceRoutine( m_texSpecular, m_nameSpecular, 0 );
    }

    Texture* EnvProbeData::getSpecularTex() const
    {
        switch ( getSpecularType() )
        {
        case STATIC_SPECULAR:
            return m_texSpecular.get();

        case DYNAMIC_SPECULAR:
            return 0;
        }
        
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    void TexturePairItem::setName( const String& name )
    {
        m_tex.release();
        m_name = name;

        _bindResourceRoutine( m_tex, m_name, 0 );
    }

    Texture* TexturePairItem::getTexture() const
    {
        return m_tex.get();
    }

    Texture* TexturePairItem::getPreparedTexture() const
    {
        if ( Texture* tex = m_tex.get() )
        {
            if ( tex->isLoaded() )
                return tex;
        }

        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    bool LightmapItem::isPrepared() const
    {
        if ( m_type == LMT_AO || m_type == LMT_BASIC )
        {
            return m_map.getPreparedTexture() != 0;
        }
        else if ( m_type == LMT_FULL )
        {
            return m_map.getPreparedTexture() && m_mapB.getPreparedTexture();
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    void VolumeProbeData::load( const String& files )
    {
        char filename[4096] = {};

        sprintf( filename, files.c_str(), "r" );
        m_mapR.setName( filename );

        sprintf( filename, files.c_str(), "g" );
        m_mapG.setName( filename );

        sprintf( filename, files.c_str(), "b" );
        m_mapB.setName( filename );
    }

    void VolumeProbeData::unload()
    {
        m_mapR.setName( STRING_EMPTY );
        m_mapG.setName( STRING_EMPTY );
        m_mapB.setName( STRING_EMPTY );
    }

    bool VolumeProbeData::isPrepared() const
    {
        return m_mapR.getPreparedTexture() && m_mapG.getPreparedTexture() && m_mapB.getPreparedTexture();
    }

    //////////////////////////////////////////////////////////////////////////
    InstanceSharedDataMgr::~InstanceSharedDataMgr()
    {
        clean();
    }

    void InstanceSharedDataMgr::clean()
    {
        _clearAllObjs<LightmapItem>(m_lightMaps);
        _clearAllObjs<EnvProbeData>(m_envProbeMap);
        _clearAllObjs<VolumeProbeData>(m_volProbeMap);
    }

    template<class T>
    T* InstanceSharedDataMgr::_createObj( typename vector<T*>::type& container, int id )
    {
        khaosAssert( id >= 0 );
        if ( id >= (int)container.size() )
        {
            container.resize( id+1, 0 );
        }

        if ( container[id] )
        {
            khaosAssert(0);
            return 0;
        }

        container[id] = KHAOS_NEW T;
        return container[id];
    }

    template<class T>
    T* InstanceSharedDataMgr::_getObj( const typename vector<T*>::type& container, int id ) const
    {
        if ( 0 <= id && id < (int)container.size() )
            return container[id];
        return 0;
    }
    
    template<class T>
    void InstanceSharedDataMgr::_removeObj( typename vector<T*>::type& container, int id )
    {
        if ( 0 <= id && id < (int)container.size() )
        {
            T* obj = container[id];
            KHAOS_DELETE obj;
            container[id] = 0;
        }
    }

    template<class T>
    void InstanceSharedDataMgr::_clearAllObjs( typename vector<T*>::type& container )
    {
        KHAOS_FOR_EACH( typename vector<T*>::type, container, it )
        {
            T* item = *it;
            KHAOS_DELETE item;
        }

        container.clear();
    }

    LightmapItem* InstanceSharedDataMgr::createLightmap( int id )
    {
        return _createObj<LightmapItem>( m_lightMaps, id );
    }

    LightmapItem* InstanceSharedDataMgr::getLightmap( int id ) const
    {
        return _getObj<LightmapItem>( m_lightMaps, id );
    }

    void InstanceSharedDataMgr::removeLightmap( int id )
    {
        _removeObj<LightmapItem>( m_lightMaps, id );
    }

    EnvProbeData* InstanceSharedDataMgr::createEnvProbe( int id )
    {
        return _createObj<EnvProbeData>( m_envProbeMap, id );
    }

    EnvProbeData* InstanceSharedDataMgr::getEnvProbe( int id ) const
    {
        return _getObj<EnvProbeData>( m_envProbeMap, id );
    }

    void InstanceSharedDataMgr::removeEnvProbe( int id )
    {
        _removeObj<EnvProbeData>( m_envProbeMap, id );
    }

    VolumeProbeData* InstanceSharedDataMgr::createVolumeProbe( int id )
    {
        return _createObj<VolumeProbeData>( m_volProbeMap, id );
    }

    VolumeProbeData* InstanceSharedDataMgr::getVolumeProbe( int id ) const
    {
        return _getObj<VolumeProbeData>( m_volProbeMap, id );
    }

    void InstanceSharedDataMgr::removeVolumeProbe( int id )
    {
        _removeObj<VolumeProbeData>( m_volProbeMap, id );
    }
}

