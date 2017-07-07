#pragma once
#include "KhaosRenderBufferPool.h"
#include "KhaosVector4.h"
#include "KhaosRect.h"
#include "KhaosRenderSetting.h"
#include "KhaosRenderDeviceDef.h"

namespace Khaos
{
    class RenderTarget;
    class Material;
    class ImageProcess;
    class ImageProcessSSAO;
    class ImageProcessHDR;
    class ImageProcessPostAA;
    class ImgProcShadowMask;
    class ImageCopyProcess;
    class ImgProcComposite;
    struct EffSetterParams;

    //////////////////////////////////////////////////////////////////////////
    class ImagePin : public AllocatedObject
    {
        typedef vector<ImagePin*>::type ImagePinList;

    public:
        ImagePin( ImageProcess* owner );
        ~ImagePin();

    public:
        ImageProcess* getOwner() const { return m_owner; }

        ImagePin* addChild( ImagePin* child );

        // ��������
        TextureObj*   getInput() const;
        void          setInput( TextureObj* obj );

        void setInputFilter( bool filterPoint );
        void setInputFilterEx( TextureFilterSet s ) { m_texFilterType = s; }
        TextureFilterSet getInputFilter() const { return m_texFilterType; }

        // �趨���
        RenderTarget* getOutput() const { return m_output; }
        void setOutput( RenderTarget* rtt ) { m_output = rtt; }
        void setOwnOutput();
        void setOwnOutputEx( uint32 clearFlag, const Color& clrClear = Color::BLACK, float z = 1.0f, int s = 0 );
        void clearOwnOutput();
        
        void setOutRTTLevel( int l ) { m_outRTTLevel = l; }
        int  getOutRTTLevel() const { return m_outRTTLevel; }

        // �Ƿ������ʱ�����rtt��Դ
        void setHoldRtt() { m_holdRtt = true; }

        // �趨�����ȴ�С
        void setResolutionSize( ResolutionSize rs ) { m_resolutionSize = rs; }
        void setResolutionSizeDerived( ResolutionSize rs );

        IntVector2 getRequestRttSize() const;
        void       setRequestRttSize( const IntVector2& sz );

        // �趨rtt��ʽ,=invalid�����Զ�����rtt
        void setRttFormat( PixelFormat fmt ) { m_rttFormat = fmt; }

        // �Ƿ�������һ�������ɫ�ȣ���Ϊfalse������ʱ����
        //void enableClearFlag( bool en ) { m_clearFlagEnabled = en; }
        bool isClearFlagEnabled() const { return m_clearFlagEnabled; }

        // shader
        int getEffectTempId() const { return m_effTempId; }
        void setEffectTempId( int id ) { m_effTempId = id; }

        Material* getMaterial() const { return m_mtr; }
        void setMaterial( Material* mtr) { m_mtr = mtr; }

        // �Զ�������
        void setFlag( void* flag ) { m_flag = flag; }
        void* getFlag() const { return m_flag; }

        // ��Դ����
        void requestRttRes();
        void freeRttRes();

        // ʹ������ʾrtt
        void useMainRTT( bool b ) { m_useMainRTT = b; }
        bool isMainRTTUsed() const { return m_useMainRTT; }

        // ʹ�������
        void useMainDepth( bool b ) { m_useMainDepth = b; }
        bool isMainDepthUsed() const { return m_useMainDepth; }

        // ʹ������ع�����λ��
        void useWPOSRender( bool b ) { m_isWPOSRender = b; }
        bool isWPOSRender() const { return m_isWPOSRender; }

        // �Ƿ������
        void enableBlendMode( bool b ) { m_isBlendEnabled = b; }
        bool isBlendModeEnabled() const { return m_isBlendEnabled; }

        // ����
        void process( EffSetterParams& params );

    private:
        void _processDerived( EffSetterParams& params );

    private:
        ImageProcess* m_owner;
        int           m_effTempId;
        TextureObj*   m_inputTex;
        RenderTarget* m_output;
        Material*     m_mtr;

        RenderBufferPool::Item* m_tmpRtt;
        
        PixelFormat   m_rttFormat;

        ImagePinList  m_children;

        ResolutionSize m_resolutionSize;
        IntVector2     m_rttSize;

        void*          m_flag;

        TextureFilterSet m_texFilterType;

        int m_outRTTLevel;

        bool          m_ownOutput   : 1;
        bool          m_ownRttSize  : 1;
        bool          m_holdRtt     : 1;

        bool          m_clearFlagEnabled : 1;
        bool          m_useMainRTT : 1;
        bool          m_useMainDepth : 1;

        bool          m_isWPOSRender : 1;
        bool          m_isBlendEnabled : 1;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcess : public AllocatedObject
    {
    public:
        ImageProcess();
        virtual ~ImageProcess();

    public:
       ImagePin* getRoot() const { return m_pinRoot; }

       virtual void process( EffSetterParams& params );

       RenderSettings* getCurrRenderSetting() const { return m_settings; }

       void setUserData( void* ud ) { m_userData = ud; }
       void* getUserData() const { return m_userData; }

    protected:
        void      _setRoot( ImagePin* pin );
        
        ImagePin* _createImagePin( int templateId ); 

        void _setResolutionSize( ResolutionSize rs ); 

        // �������
        virtual void _checkSettings();

        virtual void _getNeedSettingTypes( vector<ClassType>::type& types ) {} // ��ȡ��Ҫ������
        virtual void _onDirtySettings() {} // ��������Ϊ���ʱ�����

    protected:
        ImagePin*       m_pinRoot;
        RenderSettings* m_settings;
        void*           m_userData;
    };

    //////////////////////////////////////////////////////////////////////////
    class ImageProcessManager : public AllocatedObject
    {
    public:
        ImageProcessManager();
        ~ImageProcessManager();

    public:
        ImgProcShadowMask*  getProcessSMMask() const { return m_shadowMask; }
        ImageProcessSSAO*   getProcessSSAO() const { return m_ssao; }
        ImageProcessHDR*    getProcessHDR() const { return m_hdr; }
        ImageProcessPostAA* getProcessPostAA() const { return m_postAA; }
        ImageCopyProcess*   getProcessCopy() const { return m_copy; }
        ImgProcComposite*   getProcessComposite() const { return m_deferComposite; }

    private:
        void _init();
        void _initEffect();
        void _initProcess();
        void _clear();

    private:
        ImgProcShadowMask*  m_shadowMask;
        ImageProcessSSAO*   m_ssao;
        ImageProcessHDR*    m_hdr;
        ImageProcessPostAA* m_postAA;
        ImageCopyProcess*   m_copy;
        ImgProcComposite*   m_deferComposite;
    };

    extern ImageProcessManager* g_imageProcessManager;
}

