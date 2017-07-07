#include "SampleOperation.h"
#include <KhaosTimer.h>


bool SampleOperation::_isKeyboardDown( int key )
{
    return (GetAsyncKeyState( key ) & 0x8000) != 0;
}

void SampleOperation::init( Khaos::CameraNode* camera, float transK, float rotK )
{
    m_camera   = camera;
    m_transK   = transK;
    m_rotK     = rotK;

    GetCursorPos( &m_ptMouse );
}

bool SampleOperation::update()
{
    if ( _isKeyboardDown( VK_ESCAPE ) )
        return false;

    float elapsedTime = Khaos::g_timerSystem.getElapsedTime();

    // ¼üÅÌ²Ù×÷
    {
        float units = elapsedTime * m_transK;

        // Ç°ºó
        if ( _isKeyboardDown( 'W' ) )
            m_camera->translate( Khaos::Vector3::NEGATIVE_UNIT_Z * units, Khaos::Node::TS_LOCAL );
        else if ( _isKeyboardDown( 'S' ) )
            m_camera->translate( Khaos::Vector3::UNIT_Z * units, Khaos::Node::TS_LOCAL );

        // ×óÓÒ
        if ( _isKeyboardDown( 'D' ) )
            m_camera->translate( Khaos::Vector3::UNIT_X * units, Khaos::Node::TS_LOCAL );
        else if ( _isKeyboardDown( 'A' ) )
            m_camera->translate( Khaos::Vector3::NEGATIVE_UNIT_X * units, Khaos::Node::TS_LOCAL );

        // ÉÏÏÂ
        if ( _isKeyboardDown( 'E' ) )
            m_camera->translate( Khaos::Vector3::UNIT_Y * units, Khaos::Node::TS_LOCAL );
        else if ( _isKeyboardDown( 'Q' ) )
            m_camera->translate( Khaos::Vector3::NEGATIVE_UNIT_Y * units, Khaos::Node::TS_LOCAL );
    }

    // Êó±ê²Ù×÷
    POINT curMouse = { 0 };
    GetCursorPos( &curMouse );

    if ( _isKeyboardDown( VK_LBUTTON ) )
    {
        float units = - m_rotK;
        LONG  offx  = curMouse.x - m_ptMouse.x;
        LONG  offy  = curMouse.y - m_ptMouse.y;

        float fPercentOfNew = 1.0f / 2;
        float fPercentOfOld = 1.0f - fPercentOfNew;

        m_deltaX = m_deltaX * fPercentOfOld + offx * units * fPercentOfNew;
        m_deltaY = m_deltaY * fPercentOfOld + offy * units * fPercentOfNew;

        // Æ«º½
        m_camera->yaw( m_deltaX, Khaos::Node::TS_PARENT );

        // ¸©Ñö
        m_camera->pitch( m_deltaY, Khaos::Node::TS_LOCAL );
    }

    m_ptMouse = curMouse;
    return true;
}

