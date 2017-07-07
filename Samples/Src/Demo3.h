#pragma once
#include "SampleFrame.h"
#include <KhaosRoot.h>

class Demo3Impl : public SampleFrame
{
public:
    Demo3Impl();
    virtual ~Demo3Impl();

    virtual void onKeyUp( int key );
    virtual void onMouseUp( int key, int x, int y );

protected:
    virtual bool _onCreateScene();
    virtual void _onDestroyScene();
    virtual bool _onUpdate();

    void _setupMaterials( Khaos::map<Khaos::String, Khaos::MaterialPtr>::type& mtrs );
    void _buildVolumeProbes( Khaos::SceneGraph* sg );
    void _buildNodes( Khaos::SceneGraph* sg, Khaos::map<Khaos::String, Khaos::String>::type& meshNames );

    void _setupOriginal( Khaos::SceneGraph* sg );
    void _setupNew( Khaos::SceneGraph* sg );

    void _generalUV2( Khaos::SceneGraph* sg );
    void _bakeTest( Khaos::SceneGraph* sg );

    void _buildOther( Khaos::SceneGraph* sg );
};

