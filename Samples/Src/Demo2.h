#pragma once
#include "SampleFrame.h"

class Demo2Impl : public SampleFrame
{
public:
    Demo2Impl();
    virtual ~Demo2Impl();

protected:
    virtual bool _onCreateScene();
    virtual void _onDestroyScene();
    virtual bool _onUpdate();

    void _testSH();
};

