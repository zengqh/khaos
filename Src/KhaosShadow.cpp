#include "KhaosPreHeaders.h"
#include "KhaosShadow.h"
#include "KhaosCamera.h"
#include "KhaosTextureObj.h"
#include "KhaosRenderDevice.h"
#include "KhaosObjectAlignedBox.h"
#include "KhaosRenderTarget.h"
#include "KhaosLight.h"
#include "KhaosSysResManager.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    ShadowInfo::ShadowInfo() : 
        type(ST_NONE),
        resolution(1024), tapGroups(1),
        visibleRange(50.0f), fadeRange(45.0f), splitSchemeWeight(0.5f),
        blurSize(1.0f), strength(0.5f), randomSampleCovers(8),
        camCullCaster(0), camRenderCaster(0), camCullReceiver(0),
        /*matViewProjTexBias(0),*/ matViewProjTexBiasFor(0),
        textures(0), renderTargets(0), 
        slopeBias(0), constBias(0),
        splitDist(0),
        maxCascadesCount(0), cascadesCount(0), needRttSize(1024), 
        isEnabled(true), isCurrentActive(false)
    {
    }

    ShadowInfo::~ShadowInfo()
    {
        _clearAll();
    }

    void ShadowInfo::initCascades( int needCnt )
    {
        _clearAll();

        // cascade init
        khaosAssert( needCnt <= MAX_PSSM_CASCADES );
        maxCascadesCount = needCnt;
        splitDist = KHAOS_MALLOC_ARRAY_T(float, needCnt+1);

        // bias init
        slopeBias = KHAOS_MALLOC_ARRAY_T(float, needCnt);
        constBias = KHAOS_MALLOC_ARRAY_T(float, needCnt);

        const float arrDepthSlopeBias[] = { 3.0f, 2.0f, 1.5f, 1.5f };
        const float arrDepthConstBias[] = { 1.0f, 1.0f, 1.9f, 3.0f };
        const float constBiasScale = 0.0002f;

        int oldCas = cascadesCount; // 只为初始化bias
        cascadesCount = maxCascadesCount;
        setBias( arrDepthSlopeBias, arrDepthConstBias, constBiasScale );
        cascadesCount = oldCas;
    }

    void ShadowInfo::createCameras( int cullCasterCount, int renderCasterCount, int cullReceiverCount )
    {
        khaosAssert( !camCullCaster && !camRenderCaster && !camCullReceiver );
        camCullCaster = KHAOS_NEW Camera[cullCasterCount];
        camRenderCaster = KHAOS_NEW Camera[renderCasterCount];
        camCullReceiver = KHAOS_NEW Camera;
    }

    void ShadowInfo::createMatrixs( int matVpTBCount )
    {
        khaosAssert( /*!matViewProjTexBias &&*/ !matViewProjTexBiasFor );
        //matViewProjTexBias = KHAOS_MALLOC_ARRAY_T(Matrix4, matVpTBCount);
        matViewProjTexBiasFor = KHAOS_MALLOC_ARRAY_T(Matrix4, matVpTBCount);
    }

    void ShadowInfo::createSMTexTargets( int count )
    {
        khaosAssert( !renderTargets && !textures );

        renderTargets = KHAOS_NEW RenderTarget[count];
        textures = KHAOS_NEW TextureObjUnit[count];

        for ( int i = 0; i < count; ++i )
        {
            Viewport* vp = renderTargets[i].createViewport(0); // 1个rtt用整个图
            vp->setClearFlag( RCF_DEPTH ); // 只清除深度，我们用硬件sm

            textures[i].setFilter( TextureFilterSet::BILINEAR ); // 硬件shadowmap
            textures[i].setAddress( TextureAddressSet::CLAMP/*TextureAddressSet::BORDER*/ );
            //textures[i].setBorderColor( Color::WHITE );
        }
    }

    void ShadowInfo::setBias( const float* sb, const float* cb, float cbs )
    {
        for ( int i = 0; i < cascadesCount; ++i )
        {
            slopeBias[i] = sb[i];
            constBias[i] = cb[i] * cbs;
        }
    }

    void ShadowInfo::setBias( float sb, float cb )
    {
        for ( int i = 0; i < cascadesCount; ++i )
        {
            slopeBias[i] = sb;
            constBias[i] = cb;
        }
    }

    void ShadowInfo::_clearAll()
    {
        if ( !maxCascadesCount )
            return;

        //KHAOS_FREE(matViewProjTexBias);
        //matViewProjTexBias = 0;

        KHAOS_FREE(matViewProjTexBiasFor);
        matViewProjTexBiasFor = 0;

        KHAOS_DELETE []renderTargets;
        renderTargets = 0;

        KHAOS_DELETE []textures;
        textures = 0;

        KHAOS_DELETE []camCullCaster;
        
        if ( camCullCaster != camRenderCaster ) // camCullCaster和camRenderCaster可能一样(无snap情况)
            KHAOS_DELETE []camRenderCaster;

        camCullCaster = 0;
        camRenderCaster = 0;

        KHAOS_DELETE camCullReceiver;
        camCullReceiver = 0;

        KHAOS_FREE(splitDist);
        splitDist = 0;

        KHAOS_FREE(slopeBias);
        slopeBias = 0;

        KHAOS_FREE(constBias);
        constBias = 0;

        maxCascadesCount = 0;
    }

    Camera* ShadowInfo::getCullCaster( int i ) const 
    { 
        khaosAssert( 0 <= i ); 
        return camCullCaster+i; 
    }

    Camera* ShadowInfo::getRenderCaster( int i ) const 
    { 
        khaosAssert( 0 <= i ); 
        return camRenderCaster+i; 
    }

    RenderTarget* ShadowInfo::getRenderTarget( int i ) const
    {
        khaosAssert( 0 <= i ); 
        return renderTargets+i;
    }

    TextureObjUnit* ShadowInfo::getSMTexture( int i ) const
    {
        khaosAssert( 0 <= i ); 
        return textures+i;
    }

    Vector2 ShadowInfo::getFade() const
    {
        float zn = camCullReceiver->getZNear();
        return Vector2( zn + fadeRange, zn + visibleRange );
    }

    float ShadowInfo::getBlurUnits() const
    {
        return blurSize / (float)needRttSize;
    }

    float ShadowInfo::getShadowWeight() const
    {
        return 0.25f * strength / tapGroups;
    }

    void ShadowInfo::updateViewport( const IntRect* rects )
    {
        IntRect vpDefault(0, 0, resolution, resolution);

        for ( int i = 0; i < cascadesCount; ++i )
        {
            Viewport* vp = renderTargets[i].getViewport(0);
            vp->setRect( rects ? rects[i] : vpDefault );
            vp->linkCamera( getRenderCaster(i) );
        }
    }

    void ShadowInfo::linkResPre( int i, TextureObj* rtt, SurfaceObj* depth )
    {
        renderTargets[i].linkRTT( rtt );
        renderTargets[i].linkDepth( depth );
    }

    void ShadowInfo::linkResPost( int i, TextureObj* rtt )
    {
        textures[i].bindTextureObj( rtt );
    }

    //////////////////////////////////////////////////////////////////////////
    ShadowObject::ShadowObject() : 
        m_shadowInfo(0), m_shadowTech(0), 
        m_dirtyUpdateCascades(false), m_dirtyUpdateResolution(false)
    {
    }

    ShadowObject::~ShadowObject()
    {
        KHAOS_SAFE_DELETE( m_shadowTech );
        KHAOS_SAFE_DELETE( m_shadowInfo );
    }

    ShadowInfo* ShadowObject::_getOrCreateShadowInfo()
    {
        if ( m_shadowInfo )
            return m_shadowInfo;

        m_shadowInfo = KHAOS_NEW ShadowInfo;
        return m_shadowInfo;
    }

    void ShadowObject::setShadowType( ShadowType type )
    {
        // 删除之前的
        KHAOS_SAFE_DELETE( m_shadowTech );

        if ( m_shadowInfo ) // 仅仅标记空
            m_shadowInfo->type = ST_NONE;

        if ( type == ST_NONE )
            return;

        // 创建新的技术
        m_shadowTech = _createShadowTech( type );
        khaosAssert( m_shadowTech );

        _getOrCreateShadowInfo()->type = type;
        m_shadowTech->init( _getHost(), m_shadowInfo );

        if ( !m_dirtyUpdateCascades ) // 没有设置过cascades，同最大初始设置
        {
            m_shadowInfo->cascadesCount = m_shadowInfo->maxCascadesCount;
            m_dirtyUpdateCascades = true;
        }

        m_dirtyUpdateResolution = true; // need update
    }

    void ShadowObject::setShadowEnabled( bool b )
    {
        _getOrCreateShadowInfo()->isEnabled = b;
    }

    void ShadowObject::setCascadesCount( int casCnt )
    {
        _getOrCreateShadowInfo()->cascadesCount = casCnt;
        m_dirtyUpdateCascades = true;
    }

    void ShadowObject::setShadowResolution( int resolution )
    {
        _getOrCreateShadowInfo()->resolution = Math::maxVal(resolution, 0);
        m_dirtyUpdateResolution = true; // need update
    }

    void ShadowObject::setShadowTapGroups( int tapGroups )
    {
        _getOrCreateShadowInfo()->tapGroups = Math::clamp( tapGroups, 1, MAX_SHADOW_TAPGROUPS );
    }

    void ShadowObject::setShadowVisableRange( float range )
    {
        _getOrCreateShadowInfo()->visibleRange = Math::maxVal( range, 0.0f );
        m_shadowInfo->fadeRange = Math::clamp( m_shadowInfo->fadeRange, 0.0f, m_shadowInfo->visibleRange );
    }

    void ShadowObject::setShadowFadeRange( float range )
    {
        _getOrCreateShadowInfo()->fadeRange = Math::clamp( range, 0.0f, _getOrCreateShadowInfo()->visibleRange );
    }

    void ShadowObject::setShadowSplitSchemeWeight( float weight )
    {
        _getOrCreateShadowInfo()->splitSchemeWeight = Math::clamp( weight, 0.0f, 1.0f );
    }

    void ShadowObject::setShadowBlurSize( float bs )
    {
        _getOrCreateShadowInfo()->blurSize =  Math::maxVal( 0.0f, bs );
    }

    void ShadowObject::setShadowStrength( float strength )
    {
        _getOrCreateShadowInfo()->strength = Math::clamp( strength, 0.0f, 1.0f );
    }

    void ShadowObject::setShadowRndSmplCovers( float s )
    {
        _getOrCreateShadowInfo()->randomSampleCovers = s;
    }

    void ShadowObject::setBias( const float* sb, const float* cb, float cbs )
    {
        _getOrCreateShadowInfo()->setBias( sb, cb, cbs );
    }

    void ShadowObject::setBias( float sb, float cb )
    {
        _getOrCreateShadowInfo()->setBias( sb, cb );
    }

    float ShadowObject::getSlopeBias( int i ) const
    {
        if ( m_shadowInfo )
            return m_shadowInfo->slopeBias[i];
        return 0;
    }

    float ShadowObject::getConstBias( int i ) const
    {
        if ( m_shadowInfo )
            return m_shadowInfo->constBias[i];
        return 0;
    }

    bool ShadowObject::_isShadowCurrActive() const 
    {
        if ( m_shadowInfo )
            return m_shadowInfo->isEnabled && m_shadowInfo->isCurrentActive;

        return false; 
    }

    void ShadowObject::updateShadowInfo( Camera* mainCamera )
    {
        // 根据主视角，更新阴影相关信息
        if ( !m_shadowInfo )
            return;

        if ( m_shadowInfo->type == ST_NONE )
        {
            m_shadowInfo->isCurrentActive = false;
            return;
        }

        if ( !m_shadowInfo->isEnabled )
            return;

        // 执行更新，更新会导致设置isCurrentActive
        khaosAssert( m_shadowTech );
        
        if ( m_dirtyUpdateCascades )
        {
            m_shadowTech->updateCascades();
            m_dirtyUpdateCascades = false;
        }

        if ( m_dirtyUpdateResolution )
        {
            m_shadowTech->updateResolution();
            m_dirtyUpdateResolution = false;
        }

        m_shadowTech->updateShadowInfo( mainCamera );
    }

    //////////////////////////////////////////////////////////////////////////
    ShadowTechDirect::ShadowTechDirect() : m_mainCamera(0) 
    {
        KHAOS_CLEAR_ARRAY(m_alignOffsetX);
        KHAOS_CLEAR_ARRAY(m_alignOffsetY);
    }

    void ShadowTechDirect::init( Light* lit, ShadowInfo* si )
    {
        ShadowTechBase::init( lit, si );

        if ( si->type != ST_PSSM ) // 平行光必须是pssm
        {
            khaosAssert(0);
            si->type = ST_PSSM;
        }

        int casCnt = MAX_PSSM_CASCADES; // 初始化最大4级

        m_shadowInfo->initCascades( casCnt );
        m_shadowInfo->createCameras( casCnt, casCnt, 1 );
        m_shadowInfo->createMatrixs( casCnt );
        m_shadowInfo->createSMTexTargets( casCnt );
    }

    void ShadowTechDirect::updateCascades()
    {
        // 只处理3和4级
        m_shadowInfo->cascadesCount = 
            Math::clamp( m_shadowInfo->cascadesCount, 
            m_shadowInfo->maxCascadesCount-1, m_shadowInfo->maxCascadesCount );
    }

    void ShadowTechDirect::updateResolution()
    {
#if MERGE_ONEMAP
        if ( m_shadowInfo->type == ST_SM )
        {
             m_shadowInfo->needRttSize = m_shadowInfo->resolution;
             m_shadowInfo->updateViewport( 0 );
        }
        else
        {
            int halfSize = m_shadowInfo->resolution;
            int fullSize = halfSize * 2;

            m_shadowInfo->needRttSize = fullSize;

            IntRect rc[MAX_PSSM_CASCADES] =
            {
                IntRect( 0,        0,        halfSize, halfSize ),
                IntRect( halfSize, 0,        fullSize, halfSize ),
                IntRect( 0,        halfSize, halfSize, fullSize ),
                IntRect( halfSize, halfSize, fullSize, fullSize )
            };

            m_shadowInfo->updateViewport( rc );

            // 子区域uv范围限制，避免溢血
            float hp = 0.5f / fullSize;
            float ri = 0.5f - hp;
            float ro = 0.5f + hp;
            m_clampRanges[0] = Vector4(0.0, 0.0, ri, ri);
            m_clampRanges[1] = Vector4(ro, 0.0, 1.0, ri);
            m_clampRanges[2] = Vector4(0.0, ro, ri, 1.0);
            m_clampRanges[3] = Vector4(ro, ro, 1.0, 1.0);
        }
#else
        ShadowTechBase::updateResolution();
#endif
    }

    Vector4 ShadowTechDirect::getSplitInfo() const
    {
        // 我们只用最后一级别
        float zfar = m_shadowInfo->splitDist[m_shadowInfo->cascadesCount];
        zfar *= zfar; // 用作平方比较
        return Vector4(zfar);
    }

    Vector4 ShadowTechDirect::getShadowInfo() const
    {
        Vector2 sz = Vector2((float)m_shadowInfo->resolution) / _getRandDesiredSize();
        return Vector4( sz.x, sz.y, m_shadowInfo->getBlurUnits(), m_shadowInfo->getShadowWeight() );
    }

    void ShadowTechDirect::_calcCamCullReceiver()
    {
        // 计算剔除接受阴影物体的相机
        Camera* cullReceiver = m_shadowInfo->getCullReceiver();
        cullReceiver->setTransform( m_mainCamera->getWorldMatrix() );
        cullReceiver->setPerspective( m_mainCamera->getFov(), m_mainCamera->getAspect(),
            m_mainCamera->getZNear(), m_mainCamera->getZNear() + m_shadowInfo->visibleRange );
    }

    void ShadowTechDirect::_calcSplitSchemeWeight()
    {
        // Practical split scheme:
        //
        // CLi = n*(f/n)^(i/numsplits)
        // CUi = n + (f-n)*(i/numsplits)
        // Ci = CLi*(lambda) + CUi*(1-lambda)
        //
        // lambda scales between logarithmic and uniform
        //

        Camera* cullReceiver = m_shadowInfo->getCullReceiver();

        if ( m_shadowInfo->type != ST_PSSM )
        {
            m_shadowInfo->splitDist[0] = cullReceiver->getZNear();

            for ( int i = 1; i <= m_shadowInfo->maxCascadesCount; ++i )
                m_shadowInfo->splitDist[i] = cullReceiver->getZFar();
            return;
        }

        float weight       = m_shadowInfo->splitSchemeWeight;
        float oneSubWeight = 1 - weight;

        float zNear       = cullReceiver->getZNear();
        float zFar        = cullReceiver->getZFar();
        float zFarDivNear = zFar / zNear;
        float zFarSubNear = zFar - zNear;

        for ( int i = 1; i < m_shadowInfo->cascadesCount; ++i )
        {
            float fIDM     = i / (float)m_shadowInfo->cascadesCount;
            float fLog     = zNear * Math::pow(zFarDivNear, fIDM);
            float fUniform = zNear + zFarSubNear * fIDM;

            m_shadowInfo->splitDist[i] = fLog * weight + fUniform * oneSubWeight;
        }

        // make sure border values are accurate
        m_shadowInfo->splitDist[0] = zNear;

        for ( int i = m_shadowInfo->cascadesCount; i <= m_shadowInfo->maxCascadesCount; ++i )
            m_shadowInfo->splitDist[i] = zFar;
    }

    void ShadowTechDirect::_calcCamCullCaster( int splits, Vector2* rangeX, Vector2* rangeY, Vector2* rangeZ )
    {
        // 计算剔除投射阴影的物体的相机
        const Vector3& right   = m_light->getRight();
        const Vector3& up      = m_light->getUp();
        Vector3        zdir    = -m_light->getDirection();

        float maxLen = m_mainCamera->getZFar() / Math::cos( m_mainCamera->getFov() * 0.5f ); // 避免遮挡物在远处，选主相机长边
        Vector3 maxPos = m_mainCamera->getEyePos() + zdir * maxLen; // 光方向外延到此处
        float zMax = zdir.dotProduct( maxPos ); // 在zdir上投影距离

        Matrix4 matCull;
        matCull.setColumn( 0, right, 0 );
        matCull.setColumn( 1, up, 0 );
        matCull.setColumn( 2, zdir, 0 );
        matCull.setColumn( 3, Vector3::ZERO, 1 );

        Camera camCullReceiveSub; // 联级阴影中分段的子剔除接受阴影物的相机
        camCullReceiveSub.setTransform( m_mainCamera->getWorldMatrix() );

        for ( int i = 0; i < splits; ++i )
        {
            // 每个联级
            camCullReceiveSub.setPerspective( m_mainCamera->getFov(), m_mainCamera->getAspect(),
                m_shadowInfo->splitDist[i], m_shadowInfo->splitDist[i+1] );

            // 在光空间中的aabb范围
            const Vector3* corners = camCullReceiveSub.getCorners();
            ObjectAlignedBox::getRange( right, up, zdir, corners, 8, rangeX[i], rangeY[i], rangeZ[i] ); // 剔除caster相机的范围
            rangeZ[i].makeRange( zMax ); // 修正z最大范围

            // 每个级剔除投射的相机设置就是每个联级在光空间的范围
            Camera* cullCaster = m_shadowInfo->getCullCaster(i);
            cullCaster->setTransform( matCull );
            cullCaster->setOrtho( rangeX[i].x, rangeY[i].y, rangeX[i].y, rangeY[i].x, -rangeZ[i].y, -rangeZ[i].x );
        }
    }

    Vector2 ShadowTechDirect::_getRandDesiredSize() const
    {
        // 原始大小64，保证被阴影图最小一般512整除
        float randTexWidth  = (float) g_sysResManager->getTexRandom()->getWidth();
        float randTexHeight = (float) g_sysResManager->getTexRandom()->getHeight();

        // 我们这里缩小些，使得一个阴影纹素能覆盖几个随机纹素
        const float randScale = m_shadowInfo->randomSampleCovers;
        return Vector2( randTexWidth / randScale, randTexHeight / randScale );
    }

    void ShadowTechDirect::_calcCamRenderCaster( int splits, const Vector2* rangeX, const Vector2* rangeY, const Vector2* rangeZ )
    {
        Vector2 randSize = _getRandDesiredSize();

        // 计算渲染投射阴影物的相机（可能和剔除投射阴影相机不同，因为要做snap处理）
        for ( int i = 0; i < splits; ++i )
        {
            // 总是累计当前最大的包围范围
            Vector3 currFullSize( rangeX[i].getRange(), rangeY[i].getRange(), rangeZ[i].getRange() );
            m_maxFullSize[i].makeCeil( currFullSize ); // stable size

            // 每纹素的分辨率
            Vector2 unitXY( m_maxFullSize[i].x / m_shadowInfo->resolution, 
                m_maxFullSize[i].y / m_shadowInfo->resolution ); // distance per texel

            // 先对齐到较小的边，注意这个时候较大边可能被拉过去变小了
            Vector2 newRangeX, newRangeY;
            newRangeX.x = Math::floor(rangeX[i].x / unitXY.x) * unitXY.x; // snap min side
            newRangeY.x = Math::floor(rangeY[i].x / unitXY.y) * unitXY.y;
            newRangeX.y = newRangeX.x + m_maxFullSize[i].x; // snap max side
            newRangeY.y = newRangeY.x + m_maxFullSize[i].y;

            // 所以我们检查新的范围是否比旧范围小，是的话加上一个单位即可，然后下次继续调整
            if ( newRangeX.y < rangeX[i].y ) // check if new max side less than old max side
            {
                newRangeX.y += unitXY.x; // add a unit
                m_maxFullSize[i].x = newRangeX.getRange(); // will adjust next frame
            }

            if ( newRangeY.y < rangeY[i].y )
            {
                newRangeY.y += unitXY.y;
                m_maxFullSize[i].y = newRangeY.getRange();
            }

            // 计算渲染投射物相机，也就是和剔除相机比，投影矩阵有微弱调整
            Camera* renderCaster = m_shadowInfo->getRenderCaster(i);
            renderCaster->setTransform( m_shadowInfo->getCullCaster(i)->getWorldMatrix() );
            renderCaster->setOrtho( newRangeX.x, newRangeY.y, newRangeX.y, newRangeY.x, -rangeZ[i].y, -rangeZ[i].x );

            // 计算对齐偏移
            float startTexelX = newRangeX.x / unitXY.x; // 无限大阴影图上全局纹素的起始位置
            float startTexelY = -newRangeY.y / unitXY.y;

            startTexelX = Math::modF( startTexelX, randSize.x ); // 在随机图上的第几个纹素
            startTexelY = Math::modF( startTexelY, randSize.y );

            m_alignOffsetX[i] = startTexelX / randSize.x; // 转换为uv
            m_alignOffsetY[i] = startTexelY / randSize.y;
        }
    }

    void ShadowTechDirect::_calcMatrixViewProjTexBias( int splits )
    {
        // 投影空间变换到阴影图空间
        Matrix4 matTexBias;
        int vpSize = m_shadowInfo->resolution;
        g_renderDevice->makeMatrixProjToTex( matTexBias, 0, vpSize, vpSize, true ); // bias在生成阴影图过程，不在此

        // 由于延迟渲染拿到的pos是main camera的view space，
        // 所以 V(in shadowmap) =  shadowviewproj * inverse(mainview) * V(in mainview)

        //Matrix4 mainCamViewInv = m_mainCamera->getViewMatrixRD().inverseAffine();

        if ( m_shadowInfo->type == ST_PSSM )
        {
#if MERGE_ONEMAP
            static Matrix4 splitOff[4];
            static bool inited = false;

            if ( !inited )
            {
                inited = true;
                splitOff[0] = Matrix4::getTransScale( Vector3(0.5f, 0.5f, 1.0f), Vector3(0.0f, 0.0f, 0.0f) );
                splitOff[1] = Matrix4::getTransScale( Vector3(0.5f, 0.5f, 1.0f), Vector3(0.5f, 0.0f, 0.0f) );
                splitOff[2] = Matrix4::getTransScale( Vector3(0.5f, 0.5f, 1.0f), Vector3(0.0f, 0.5f, 0.0f) );
                splitOff[3] = Matrix4::getTransScale( Vector3(0.5f, 0.5f, 1.0f), Vector3(0.5f, 0.5f, 0.0f) );
            }
#endif

            for ( int i = 0; i < splits; ++i )
            {
                *(m_shadowInfo->getMatViewProjTexBiasFor(i)) =
#if 0//MERGE_ONEMAP
                    splitOff[i] * 
#endif
                    matTexBias * 
                    m_shadowInfo->getRenderCaster(i)->getViewProjMatrixRD();
            }
        }
        else
        {
            khaosAssert( splits == 1 );
            *(m_shadowInfo->getMatViewProjTexBiasFor(0)) = 
                matTexBias * m_shadowInfo->getRenderCaster(0)->getViewProjMatrixRD();
        }

        // for 延迟阴影
        //for ( int i = 0; i < splits; ++i )
        //{
        //    *(m_shadowInfo->getMatViewProjTexBias(i)) =
        //        *(m_shadowInfo->getMatViewProjTexBiasFor(i)) * mainCamViewInv;
        //}
    }

    void ShadowTechDirect::updateShadowInfo( Camera* mainCamera )
    {
        // 临时设置
        m_mainCamera = mainCamera;

        // 0.阴影是否在可见范围内活动
        m_shadowInfo->isCurrentActive = true; // 平行光总是有效

        // 1.计算剔除receiver用相机
        _calcCamCullReceiver();

        // 2.计算联级距离分布
        _calcSplitSchemeWeight();

        int splits = m_shadowInfo->cascadesCount;

        // 3.计算剔除caster的相机
        Vector2 rangeX[MAX_PSSM_CASCADES], rangeY[MAX_PSSM_CASCADES], rangeZ[MAX_PSSM_CASCADES];
        _calcCamCullCaster( splits, rangeX, rangeY, rangeZ );

        // 4.计算渲染caster的相机
        _calcCamRenderCaster( splits, rangeX, rangeY, rangeZ );

        // 5.计算偏移矩阵
        _calcMatrixViewProjTexBias( splits );
    }
}

