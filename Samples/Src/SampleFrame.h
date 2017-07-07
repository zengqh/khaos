#pragma once
#include "SampleOperation.h"
#include <khaosStdTypes.h>

struct SampleCreateContext
{
    void* handleWindow;
    int   windowWidth;
    int   windowHeight;

    const char* exePath;
};

class SampleFrame : public Khaos::AllocatedObject
{
public:
    SampleFrame();
    virtual ~SampleFrame();

public:
    virtual void release() { KHAOS_DELETE this; }

    virtual bool create( const SampleCreateContext& scc );
    virtual void destroy();
    virtual bool runIdle();
    
    virtual void onKeyUp( int key ) {}
    virtual void onMouseUp( int key, int x, int y ) {}

protected:
    virtual bool _onCreateScene() { return true; }
    virtual void _onDestroyScene() {}
    virtual bool _onUpdate();

    void _processKeyCommon( int key );

protected:
    SampleOperation m_samOp;
};

extern SampleFrame* g_sampleFrame;

SampleFrame* createSampleFrame();

//#define _BASIC_ACTIVE
//#define _DEMO1_ACTIVE
//#define _DEMO2_ACTIVE
//#define _DEMO3_ACTIVE
#define _DEMO4_ACTIVE

