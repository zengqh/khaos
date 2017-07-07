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
        int         resolution; // ��Ӱ��ͼ�ֱ���
        int         maxCascadesCount; // csm��ʼ��ʱ��Ŀ
        int         cascadesCount; // csm��Ŀ
        float       visibleRange; // ��Ӱ�ɼ���Χ

        Camera* camCullCaster;     // shadowmap camera for culling caster
        Camera* camRenderCaster;   // shadowmap camera for rendering caster

        Matrix4* matViewProjTexBiasFor; // forward

        TextureObjUnit* positionBuffers; // shadowmap texture
        TextureObjUnit* normalBuffers;
        TextureObjUnit* colorBuffers;
        RenderTarget*   renderTargets; // shadowmap render target

        bool    isEnabled;       // �Ƿ���
        bool    isCurrentActive; // �Ƿ�
    };

   
}

