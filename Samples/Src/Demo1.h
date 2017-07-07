#pragma once
#include "SampleFrame.h"


class Demo1Impl : public SampleFrame
{
public:
    Demo1Impl();
    virtual ~Demo1Impl();

    virtual void onKeyUp( int key );

protected:
    virtual bool _onCreateScene();
    virtual bool _onUpdate();

    void _setupCamera();
};

