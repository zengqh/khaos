#pragma once
#include "KhaosStdTypes.h"


namespace Khaos
{
    struct ISceneObjectListener
    {
        virtual void onObjectAABBDirty() = 0;
    };

    class SceneObject : public AllocatedObject
    {
    public:
        SceneObject() : m_listener(0) {}
        virtual ~SceneObject() {}

        void setObjectListener( ISceneObjectListener* listener )
        {
            m_listener = listener;
        }

    protected:
        void _fireAABBDirty()
        {
            if ( m_listener )
                m_listener->onObjectAABBDirty();
        }

    protected:
        ISceneObjectListener* m_listener;
    };
}

