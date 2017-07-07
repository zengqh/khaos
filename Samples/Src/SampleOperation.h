#pragma once
#include <KhaosCameraNode.h>

class SampleOperation
{
public:
    SampleOperation() : m_camera(0), m_transK(0), m_rotK(0), m_deltaX(0), m_deltaY(0)
    {
        m_ptMouse.x = m_ptMouse.y = 0;
    }

public:
    void init( Khaos::CameraNode* camera, float transK, float rotK );
    bool update();

private:
    bool _isKeyboardDown( int key );

private:
    Khaos::CameraNode* m_camera;
    float              m_transK;
    float              m_rotK;
    POINT              m_ptMouse;
    float              m_deltaX;
    float              m_deltaY;
};

