#include "KhaosPreHeaders.h"
#include "KhaosRSM.h"
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
    RSMData::RSMData() :
        resolution(256), 
        maxCascadesCount(0), cascadesCount(0), 
        visibleRange(100.0f), 
        camCullCaster(0), camRenderCaster(0),
        matViewProjTexBiasFor(0),
        positionBuffers(0), normalBuffers(0), colorBuffers(0), renderTargets(0),
        isEnabled(true), isCurrentActive(false)
    {
    }

    RSMData::~RSMData()
    {
        _clearAll();
    }

    void RSMData::initCascades( int needCnt )
    {
        _clearAll();

        maxCascadesCount = needCnt;

        // cameras
        camCullCaster = KHAOS_NEW Camera[needCnt];
        camRenderCaster = KHAOS_NEW Camera[needCnt];

        // matrix
        matViewProjTexBiasFor = KHAOS_MALLOC_ARRAY_T(Matrix4, needCnt);

        // texture resource
        renderTargets   = KHAOS_NEW RenderTarget[needCnt];
        positionBuffers = KHAOS_NEW TextureObjUnit[needCnt];
        normalBuffers   = KHAOS_NEW TextureObjUnit[needCnt];
        colorBuffers    = KHAOS_NEW TextureObjUnit[needCnt];

        for ( int i = 0; i < needCnt; ++i )
        {
            Viewport* vp = renderTargets[i].createViewport(0); // 1个rtt用整个图
            vp->setClearFlag( RCF_TARGET|RCF_DEPTH );

            positionBuffers[i].setFilter( TextureFilterSet::NEAREST );
            positionBuffers[i].setAddress( TextureAddressSet::CLAMP );
            normalBuffers[i].setFilter( TextureFilterSet::NEAREST );
            normalBuffers[i].setAddress( TextureAddressSet::CLAMP );
            colorBuffers[i].setFilter( TextureFilterSet::NEAREST );
            colorBuffers[i].setAddress( TextureAddressSet::CLAMP );
        }
    }

    void RSMData::_clearAll()
    {
        if ( !maxCascadesCount )
            return;

        KHAOS_DELETE []camCullCaster;

        if ( camCullCaster != camRenderCaster ) // camCullCaster和camRenderCaster可能一样(无snap情况)
            KHAOS_DELETE []camRenderCaster;

        camCullCaster = 0;
        camRenderCaster = 0;

        KHAOS_FREE(matViewProjTexBiasFor);
        matViewProjTexBiasFor = 0;

        KHAOS_DELETE []renderTargets;
        renderTargets = 0;

        KHAOS_DELETE []positionBuffers;
        positionBuffers = 0;

        KHAOS_DELETE []normalBuffers;
        normalBuffers = 0;

        KHAOS_DELETE []colorBuffers;
        colorBuffers = 0;

        maxCascadesCount = 0;
    }

    Camera* RSMData::getCullCaster( int i ) const 
    { 
        khaosAssert( 0 <= i && i < maxCascadesCount ); 
        return camCullCaster+i; 
    }

    Camera* RSMData::getRenderCaster( int i ) const 
    { 
        khaosAssert( 0 <= i && i < maxCascadesCount ); 
        return camRenderCaster+i; 
    }

    Matrix4* RSMData::getMatViewProjTexBiasFor( int i ) const 
    {
        khaosAssert( 0 <= i && i < maxCascadesCount );
        return matViewProjTexBiasFor+i; 
    }

    RenderTarget* RSMData::getRenderTarget( int i ) const
    {
        khaosAssert( 0 <= i && i < maxCascadesCount ); 
        return renderTargets+i;
    }

    TextureObjUnit* RSMData::getPositionBuffer( int i ) const
    {
        khaosAssert( 0 <= i && i < maxCascadesCount ); 
        return positionBuffers+i;
    }

    TextureObjUnit* RSMData::getNormalBuffer( int i ) const
    {
        khaosAssert( 0 <= i && i < maxCascadesCount ); 
        return normalBuffers+i;
    }
    
    TextureObjUnit* RSMData::getColorBuffer( int i ) const
    {
        khaosAssert( 0 <= i && i < maxCascadesCount ); 
        return colorBuffers+i;
    }

    void RSMData::updateViewport()
    {
        IntRect vpDefault(0, 0, resolution, resolution);

        for ( int i = 0; i < cascadesCount; ++i )
        {
            Viewport* vp = renderTargets[i].getViewport(0);
            vp->setRect( vpDefault );
            vp->linkCamera( getRenderCaster(i) );
        }
    }

    void RSMData::linkResPre( int i, TextureObj** rtt, SurfaceObj* depth )
    {
        renderTargets[i].linkRTT( 0, rtt[0] ); // depth, normal, color
        renderTargets[i].linkRTT( 1, rtt[1] );
        renderTargets[i].linkRTT( 1, rtt[2] );
        renderTargets[i].linkDepth( depth );
    }

    void RSMData::linkResPost( int i, TextureObj** rtt )
    {
        positionBuffers[i].bindTextureObj( rtt[0] );
        normalBuffers[i].bindTextureObj( rtt[1] );
        colorBuffers[i].bindTextureObj( rtt[2] );
    }

}

