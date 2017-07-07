#pragma once
#include "KhaosStdTypes.h"
#include "KhaosRTTI.h"

namespace Khaos
{
    class SceneNode;

    //////////////////////////////////////////////////////////////////////////
    class NodeFactory : public AllocatedObject
    {
    public:
        typedef SceneNode* (*CreatorType)();
        typedef unordered_map<ClassType, CreatorType>::type CreatorMap;

    public:
        NodeFactory();
        ~NodeFactory();

    public:
        SceneNode* createSceneNode( ClassType type );
        void       destroySceneNode( SceneNode* node );

    private:
        template<class T>
        static SceneNode* _createSceneNode();

        CreatorType _findCreator( ClassType type ) const;

        template<class T>
        void _registerCreator();

        void _init();

    private:
        CreatorMap m_creatorMap;
    };

    //////////////////////////////////////////////////////////////////////////
    extern NodeFactory* g_nodeFactory;
}

