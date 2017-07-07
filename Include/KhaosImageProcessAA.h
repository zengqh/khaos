#pragma once
#include "KhaosImageProcess.h"
#include "KhaosImageProcessUtil.h"
#include "KhaosMaterial.h"
#include "KhaosMatrix4.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class ImgProcAAEdgeDetection : public ImageProcess
    {
    public:
        ImgProcAAEdgeDetection();

        void setColorBuffer( TextureObj* texClrGamma );
        void setOutput( TextureObj* texOut );

        virtual void process( EffSetterParams& params );

    private:
        ImagePin*   m_pin;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImgProcAABlendingWeightsCalculation : public ImageProcess
    {
    public:
        ImgProcAABlendingWeightsCalculation();

        void setEdgesBuffer( TextureObj* texEdges );
        void setOutput( TextureObj* texOut );

        virtual void process( EffSetterParams& params );

    private:
        ImagePin*   m_pin;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImgProcAAFinal : public ImageProcess
    {
    public:
        ImgProcAAFinal();

        virtual void process( EffSetterParams& params );

    private:
        ImagePin*   m_pin;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImgProcAAFinal2 : public ImageProcess
    {
    public:
        ImgProcAAFinal2();

        virtual void process( EffSetterParams& params );

    private:
        ImagePin*   m_pin;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImgProcAATemporal : public ImageProcess
    {
    public:
        ImgProcAATemporal();
        virtual ~ImgProcAATemporal();

        void process( EffSetterParams& params, TextureObj* inSceneBuff, TextureObj* inSceneLastBuff, 
            TextureObj* outSceneBuff, bool needSRGB, bool isPriorToFXAA );

        TextureObj* getCurrSceneBuff() const { return m_inSceneBuff; }
        TextureObj* getLastSceneBuff() const { return m_inSceneLastBuff; }

        bool isWriteLuma() const { return m_isWriteLuma; }

    private:
        ImagePin*   m_pin;
        ImagePin*   m_pinTemp;

        TextureObj* m_inSceneBuff;
        TextureObj* m_inSceneLastBuff;

        bool m_isWriteLuma;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessPostAA : public AllocatedObject
    {
    public:
        ImageProcessPostAA();
        ~ImageProcessPostAA();

        void applyJitter( Camera* cam );

        const float* getSubIdx() const;
        Vector2 getJitterUV( Camera* cam );
        Vector3 getJitterWorld( Camera* cam );

        TextureObj* _getColorBuf() const { return m_colorBuf; }
        //TextureObj* _getEdgesAABuf() const { return m_edgesAABuf; }
        TextureObj* _getBlendAABuf() const { return m_blendAABuf; }

        TextureObj* getFrame( bool curr ) const;

        const Matrix4& getMatViewProjPre() const { return m_matViewProjPre; }

        bool _isTempWriteLuma() const { return m_temporal.isWriteLuma(); }

    public:
        void processMainAA( EffSetterParams& params, TextureObj* inSceneBuff );

        void processMainAA2( EffSetterParams& params, TextureObj* inSceneBuff );

        void processTempAA( EffSetterParams& params, TextureObj* inSceneBuff, 
            TextureObj* inSceneLastBuff, TextureObj* outSceneBuff, bool needSRGB, bool isPriorToFXAA );

    private:
        Vector2 _getJitter( Camera* cam );
        int     _getCurrID() const;

    private:
        // smaa
        ImgProcAAEdgeDetection m_passEdgeDetection;
        ImgProcAABlendingWeightsCalculation m_passBlendingWeights;
        ImgProcAAFinal m_final;

        // fxaa
        ImgProcAAFinal2 m_final2;

        // temp aa
        ImgProcAATemporal m_temporal;

        // buffer
        TextureObj*     m_colorBuf;
        TextureObj*     m_edgesAABuf;
        TextureObj*     m_blendAABuf;

        Matrix4     m_matViewProjPre;
        Matrix4     m_matLastView;
        Matrix4     m_matLastProj;
        bool        m_inited;
    };
}

