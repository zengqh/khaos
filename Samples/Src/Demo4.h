#pragma once
#include "SampleFrame.h"


class Demo4Impl : public SampleFrame
{
public:
    virtual void onKeyUp( int key );

protected:
    virtual bool _onCreateScene();

    void _setupCamera();
};

