#pragma once
#include "SampleFrame.h"

class SampleImpl : public SampleFrame
{
public:
    SampleImpl();
    virtual ~SampleImpl();

    virtual void onKeyUp( int key );

protected:
    virtual bool _onCreateScene();
    virtual bool _onUpdate();
};

