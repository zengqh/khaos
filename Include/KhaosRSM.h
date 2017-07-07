#pragma once
#include "KhaosMatrix4.h"


namespace Khaos
{
    class Camera;
    class TextureObj;
    class SurfaceObj;
    class TextureObjUnit;
    class RenderTarget;
    
    struct RSMData : public AllocatedObject
    {
    public:
        RSMData();
        ~RSMData();

    public:
        void initCascades( int needCnt );
        void _clearAll();

    public:
        Camera* getCullCaster( int i ) const;
        Camera* getRenderCaster( int i ) const;

        Matrix4* getMatViewProjTexBiasFor( int i ) const;

        RenderTarget*   getRenderTarget( int i ) const;
        TextureObjUnit* getPositionBuffer( int i ) const;
        TextureObjUnit* getNormalBuffer( int i ) const;
        TextureObjUnit* getColorBuffer( int i ) const;

    public:
        void updateViewport();

        void linkResPre( int i, TextureObj** rtt, SurfaceObj* depth );
        void linkResPost( int i, TextureObj** rtt );

    public:
        int         resolution; // 阴影贴图分辨率
        int         maxCascadesCount; // csm初始化时数目
        int         cascadesCount; // csm数目
        float       visibleRange; // 阴影可见范围

        Camera* camCullCaster;     // shadowmap camera for culling caster
        Camera* camRenderCaster;   // shadowmap camera for rendering caster

        Matrix4* matViewProjTexBiasFor; // forward

        TextureObjUnit* positionBuffers; // shadowmap texture
        TextureObjUnit* normalBuffers;
        TextureObjUnit* colorBuffers;
        RenderTarget*   renderTargets; // shadowmap render target

        bool    isEnabled;       // 是否开启
        bool    isCurrentActive; // 是否活动
    };

   
}

