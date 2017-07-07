#pragma once

namespace Khaos
{
    class RenderableSharedData : public AllocatedObject
    {
    public:
        RenderableSharedData() : m_lightMapID(-1), m_shadowMapID(-1), m_envProbeID(-1) {}

    public:
        void setLightMapID( int id ) { m_lightMapID = id; }
        int  getLightMapID() const { return m_lightMapID; }

        void setShadowMapID( int id ) { m_shadowMapID = id; }
        int  getShadowMapID() const { return m_shadowMapID; }

        void setEnvProbeID( int id ) { m_envProbeID = id; }
        int  getEnvProbeID() const { return m_envProbeID; }

    private:
        int m_lightMapID;
        int m_shadowMapID;
        int m_envProbeID;
    };
}

