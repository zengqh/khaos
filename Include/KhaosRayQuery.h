#pragma once
#include "KhaosRay.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class SceneGraph;
    class SceneNode;

    SceneNode* rayIntersectSGBound( SceneGraph* sg, const Ray& ray, float* t = 0 );
    SceneNode* rayIntersectSGBound( SceneGraph* sg, const LimitRay& ray, float* t = 0 );

    SceneNode* rayIntersectSGBoundMore( SceneGraph* sg, const Ray& ray, int* subIdx = 0, float* t = 0 );
    SceneNode* rayIntersectSGBoundMore( SceneGraph* sg, const LimitRay& ray, int* subIdx = 0, float* t = 0 );

    SceneNode* rayIntersectSGDetail( SceneGraph* sg, const Ray& ray,
        int* subIdx = 0, int* faceIdx = 0, float* t = 0, Vector3* gravity = 0 );
    SceneNode* rayIntersectSGDetail( SceneGraph* sg, const LimitRay& ray,
        int* subIdx = 0, int* faceIdx = 0, float* t = 0, Vector3* gravity = 0 );
}

