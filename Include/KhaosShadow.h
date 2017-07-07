#pragma once
#include "KhaosVector2.h"
#include "KhaosVector3.h"
#include "KhaosMatrix4.h"
#include "KhaosRect.h"
#include "KhaosRenderFeature.h"

#define  MERGE_ONEMAP   1

namespace Khaos
{
    class Light;
    class DirectLight;
    class Camera;
    class TextureObj;
    class SurfaceObj;
    class TextureObjUnit;
    class RenderTarget;

    //////////////////////////////////////////////////////////////////////////
    enum ShadowType
    {
        ST_NONE,
        ST_SM,
        ST_PSSM,
    };

    const int MAX_SHADOW_TAPGROUPS = MAX_POISSON_DISK / 2;
    const int MAX_PSSM_CASCADES = 4;
    
    struct ShadowInfo : public AllocatedObject
    {
    public:
        ShadowInfo();
        ~ShadowInfo();

    public:
        void initCascades( int needCnt );
        void createCameras( int cullCasterCount, int renderCasterCount, int cullReceiverCount );
        void createMatrixs( int matVpTBCount );
        void createSMTexTargets( int count );

        void setBias( const float* sb, const float* cb, float cbs );
        void setBias( float sb, float cb );

        void _clearAll();

    public:
        Camera* getCullCaster( int i ) const;
        Camera* getRenderCaster( int i ) const;
        Camera* getCullReceiver() const { return camCullReceiver; }

        //Matrix4* getMatViewProjTexBias( int i ) const { khaosAssert( 0 <= i ); return matViewProjTexBias+i; }
        Matrix4* getMatViewProjTexBiasFor( int i ) const { khaosAssert( 0 <= i ); return matViewProjTexBiasFor+i; }

        RenderTarget*   getRenderTarget( int i ) const;
        TextureObjUnit* getSMTexture( int i ) const;

        Vector2 getFade() const;
        float   getBlurUnits() const;
        float   getShadowWeight() const;

    public:
        void updateViewport( const IntRect* rects );

        void linkResPre( int i, TextureObj* rtt, SurfaceObj* depth );
        void linkResPost( int i, TextureObj* rtt );

    public:
        ShadowType  type; // 阴影类型
        int         resolution; // 阴影贴图分辨率
        int         tapGroups; // 采样组数量
        float       visibleRange; // 阴影可见范围
        float       fadeRange; // 消隐起始距离
        float       splitSchemeWeight; // pssm划分权重
        float       blurSize; // 模糊量
        float       strength; // 阴影强度
        float       randomSampleCovers; // 每阴影纹素覆盖多少随机采样

        Camera* camCullCaster;     // shadowmap camera for culling caster
        Camera* camRenderCaster;   // shadowmap camera for rendering caster
        Camera* camCullReceiver;   // shadowmap camera for culling receiver

        //Matrix4* matViewProjTexBias; // shadowmap: (tex&bias) * proj * view
        Matrix4* matViewProjTexBiasFor; // forward

        TextureObjUnit* textures;      // shadowmap texture
        RenderTarget*   renderTargets; // shadowmap render target

        float*  slopeBias; // depth bias
        float*  constBias;

        float*  splitDist; // pssm分配距离

        int     maxCascadesCount; // csm初始化时数目
        int     cascadesCount; // csm数目
        int     needRttSize; // 可能和resolution不同

        bool    isEnabled;       // 是否开启
        bool    isCurrentActive; // 是否活动
    };

    //////////////////////////////////////////////////////////////////////////
    class ShadowTechBase : public AllocatedObject
    {
    public:
        ShadowTechBase() : m_light(0), m_shadowInfo(0) {}
        virtual ~ShadowTechBase() {}

        virtual void init( Light* lit, ShadowInfo* si )
        {
            m_light = lit;
            m_shadowInfo = si; 
        }

        virtual void updateCascades()
        {
            if ( m_shadowInfo->cascadesCount > m_shadowInfo->maxCascadesCount )
                m_shadowInfo->cascadesCount = m_shadowInfo->maxCascadesCount;
        }

        virtual void updateResolution()
        {
            m_shadowInfo->needRttSize = m_shadowInfo->resolution;
            m_shadowInfo->updateViewport(0);
        }

        virtual void updateShadowInfo( Camera* mainCamera ) = 0;

    protected:
        Light*      m_light;
        ShadowInfo* m_shadowInfo;
    };

    class ShadowObject
    {
    protected:
        ShadowObject();
        virtual ~ShadowObject();

    public:
        // shadow property
        void setShadowType( ShadowType type );
        void setCascadesCount( int casCnt );
        void setShadowResolution( int resolution );
        void setShadowTapGroups( int tapGroups );
        void setShadowVisableRange( float range );
        void setShadowFadeRange( float range );
        void setShadowSplitSchemeWeight( float weight );
        void setShadowBlurSize( float bs );
        void setShadowStrength( float strength );
        void setShadowRndSmplCovers( float s );

        void setBias( const float* sb, const float* cb, float cbs = 0.0001f );
        void setBias( float sb, float cb );

        void setShadowEnabled( bool b );

        ShadowType getShadowType()          const { return m_shadowInfo ? m_shadowInfo->type : ST_NONE; }
        int   getCascadesCount()            const { return m_shadowInfo ? m_shadowInfo->cascadesCount : 0; }
        int   getShadowResolution()         const { return m_shadowInfo ? m_shadowInfo->resolution : 0; }
        int   getShadowTapGroups()          const { return m_shadowInfo ? m_shadowInfo->tapGroups : 0; }
        float getShadowVisibleRange()       const { return m_shadowInfo ? m_shadowInfo->visibleRange : 0; }
        float getShadowFadeRange()          const { return m_shadowInfo ? m_shadowInfo->fadeRange : 0; }
        float getShadowSplitSchemeWeight()  const { return m_shadowInfo ? m_shadowInfo->splitSchemeWeight : 0; }
        float getShadowBlurSize()           const { return m_shadowInfo ? m_shadowInfo->blurSize : 0; }
        float getShadowStrength()           const { return m_shadowInfo ? m_shadowInfo->strength : 0; }
        float getShadowRndSmplCovers()      const { return m_shadowInfo ? m_shadowInfo->randomSampleCovers : 0; }

        float getSlopeBias( int i ) const;
        float getConstBias( int i ) const;

        bool isShadowEnabled() const { return m_shadowInfo ? m_shadowInfo->isEnabled : false; }

    public:
        void updateShadowInfo( Camera* mainCamera );

        bool        _isShadowCurrActive() const;

        ShadowInfo* _getShadowInfo() { return m_shadowInfo; }
        ShadowTechBase* _getShadowTeck() { return m_shadowTech; }

    protected:
        virtual Light*          _getHost() = 0;
        virtual ShadowTechBase* _createShadowTech( ShadowType type ) { return 0; }

        ShadowInfo* _getOrCreateShadowInfo();

    protected:
        ShadowInfo*     m_shadowInfo;
        ShadowTechBase* m_shadowTech;

        bool            m_dirtyUpdateCascades   : 1;
        bool            m_dirtyUpdateResolution : 1;
    };


    //////////////////////////////////////////////////////////////////////////
    class ShadowTechDirect : public ShadowTechBase
    {
    public:
        ShadowTechDirect();

        virtual void init( Light* lit, ShadowInfo* si );
        virtual void updateCascades();
        virtual void updateResolution();
        virtual void updateShadowInfo( Camera* mainCamera );

        const float* getAlignOffsetX() const { return m_alignOffsetX; }
        const float* getAlignOffsetY() const { return m_alignOffsetY; }
        
        const Vector4* getClampRanges() const { return m_clampRanges; }

        Vector4 getSplitInfo() const;
        Vector4 getShadowInfo() const;

    private:
        void _calcCamCullReceiver();
        void _calcSplitSchemeWeight();
        void _calcCamCullCaster( int splits, Vector2* rangeX, Vector2* rangeY, Vector2* rangeZ );
        void _calcCamRenderCaster( int splits, const Vector2* rangeX, const Vector2* rangeY, const Vector2* rangeZ );
        void _calcMatrixViewProjTexBias( int splits );

        Vector2 _getRandDesiredSize() const;

    private:
        Camera* m_mainCamera;
        Vector3 m_maxFullSize[MAX_PSSM_CASCADES];
        float   m_alignOffsetX[MAX_PSSM_CASCADES];
        float   m_alignOffsetY[MAX_PSSM_CASCADES];
        Vector4 m_clampRanges[MAX_PSSM_CASCADES];
    };
}

