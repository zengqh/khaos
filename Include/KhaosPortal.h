#pragma once
#include "KhaosStdTypes.h"
#include "KhaosVector3.h"
#include "KhaosResource.h"

#if 0
namespace Khaos
{
    class SceneNode;

    class Portal : public Resource
    {
        KHAOS_DECLARE_RTTI(Portal)

    public:
        typedef vector<Vector3>::type Points;

    public:
        Portal() : m_home(0), m_target(0), m_twoWay(true) {}
        virtual ~Portal();

    public:
        void setHome( SceneNode* node ) { m_home = node; }
        void setTarget( SceneNode* node ) { m_target = node; }
        
        SceneNode* getHome() const { return m_home; }
        SceneNode* getTarget() const { return m_target; }

        void setData( const Points& data ) { m_pts = data; }
        void clearData() { m_pts.clear(); }
        const Points& getData() const { return m_pts; }

        void setTwoWay( bool twoway ) { m_twoWay = twoway; }
        bool isTwoWay() const { return m_twoWay; }

    protected:
        SceneNode*  m_home;
        SceneNode*  m_target;
        Points      m_pts;
        bool        m_twoWay;
    };

    typedef SharedPtr<Portal> PortalPtr;
}

#endif

