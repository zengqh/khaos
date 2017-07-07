#include "KhaosPreHeaders.h"
#include "KhaosRayQuery.h"
#include "KhaosMesh.h"
#include "KhaosSceneGraph.h"


namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    template<class RayClass>
    class _SGFindResultByRay : public SGFindBase
    {
    public:
        _SGFindResultByRay() : m_result(0), m_subIdx(0), m_faceIdx(0), m_t(Math::POS_INFINITY), m_level(0)
        {
            KHAOS_SFG_SET_RESULT( _SGFindResultByRay, NT_RENDER, _onFindRenderNode );
        }

        void setLevel( int level ) { m_level = level; }

    public:
        void _onFindRenderNode( SceneNode* node )
        {
            if ( KHAOS_OBJECT_IS(node, MeshNode) )
            {
                const RayClass* rayWorld = static_cast<const RayClass*>(m_sfo.ray);
                _findImpl( rayWorld, node );
            }
            else
            {
                khaosAssert(0);
            }
        }

    private:
        void _findImpl( const Ray* rayWorld, SceneNode* node )
        {
            Matrix4 worldToObj = node->getDerivedMatrix().inverseAffine();

            Ray rayObj(
                worldToObj.transformAffine( rayWorld->getOrigin() ),
                worldToObj.transformAffineNormal( rayWorld->getDirection() ).normalisedCopy()
                );

            _checkRay( rayObj, rayWorld, node, 0 );
        }

        void _findImpl( const LimitRay* rayWorld, SceneNode* node )
        {
            Matrix4 worldToObj = node->getDerivedMatrix().inverseAffine();

            LimitRay rayObj(
                worldToObj.transformAffine( rayWorld->getOrigin() ),
                worldToObj.transformAffine( rayWorld->getEndPoint() )
                );

            float t_limit = rayWorld->getMaxLen();
           _checkRay( rayObj, rayWorld, node, &t_limit );
        }

        void _checkRay( const RayClass& ray, const RayClass* rayWorld, SceneNode* node, float* t_limit )
        {
            MeshNode* meshNode = static_cast<MeshNode*>(node);
            Mesh*     mesh = meshNode->getMesh();

            int   subIdx  = 0;
            int   faceIdx = 0;
            float t       = 0;

            Vector3 gravity;

            bool ok;

            if ( m_level == 0 )
            {
                ok = mesh->intersectBound( ray, &t );
            }
            else if ( m_level == 1 )
            {
                ok = mesh->intersectBoundMore( ray, &subIdx, &t );
            }
            else /*if ( m_level == 2 )*/
            {
                khaosAssert( m_level == 2 );
                ok = mesh->intersectDetail( ray, &subIdx, &faceIdx, &t, &gravity );
            }

            if ( ok )
            {
                // 把object空间t转换到world
                Vector3 pointObj = ray.getPoint(t);
                Vector3 pointWorld = node->getDerivedMatrix().transformAffine( pointObj );

                t = rayWorld->getOrigin().distance( pointWorld );

                if ( t_limit )
                {
                    if ( t > *t_limit )
                        t = *t_limit;
                }

                if ( t < m_t )
                {
                    m_result  = node;
                    m_subIdx  = subIdx;
                    m_faceIdx = faceIdx;
                    m_gravity = gravity;
                    m_t       = t;
                }
            }
        }

    public:
        SceneNode* m_result;
        int        m_subIdx;
        int        m_faceIdx;
        float      m_t;
        Vector3    m_gravity;
        int        m_level;
    };

    //////////////////////////////////////////////////////////////////////////
    template<class RayClass>
    SceneNode* _rayIntersectSGBound( SceneGraph* sg, const RayClass& ray, float* t )
    {
        _SGFindResultByRay<RayClass> finder;
        finder.setLevel(0);
        finder.setFinder( const_cast<RayClass*>(&ray) );
        finder.find( sg );

        if ( !finder.m_result )
            return 0;

        if ( t ) *t = finder.m_t;

        return finder.m_result;
    }

    SceneNode* rayIntersectSGBound( SceneGraph* sg, const Ray& ray, float* t )
    {
        return _rayIntersectSGBound<Ray>( sg, ray, t );
    }

    SceneNode* rayIntersectSGBound( SceneGraph* sg, const LimitRay& ray, float* t )
    {
        return _rayIntersectSGBound<LimitRay>( sg, ray, t );
    }

    template<class RayClass>
    SceneNode* _rayIntersectSGBoundMore( SceneGraph* sg, const RayClass& ray, int* subIdx, float* t )
    {
        _SGFindResultByRay<RayClass> finder;
        finder.setLevel(1);
        finder.setFinder( const_cast<RayClass*>(&ray) );
        finder.find( sg );

        if ( !finder.m_result )
            return 0;

        if ( subIdx )  *subIdx  = finder.m_subIdx;
        if ( t )       *t       = finder.m_t;

        return finder.m_result;
    }

    SceneNode* rayIntersectSGBoundMore( SceneGraph* sg, const Ray& ray, int* subIdx, float* t )
    {
        return _rayIntersectSGBoundMore<Ray>( sg, ray, subIdx, t );
    }

    SceneNode* rayIntersectSGBoundMore( SceneGraph* sg, const LimitRay& ray, int* subIdx, float* t )
    {
        return _rayIntersectSGBoundMore<LimitRay>( sg, ray, subIdx, t );
    }

    template<class RayClass>
    SceneNode* _rayIntersectSGDetail( SceneGraph* sg, const RayClass& ray,
        int* subIdx, int* faceIdx, float* t, Vector3* gravity )
    {
        _SGFindResultByRay<RayClass> finder;
        finder.setLevel(2);
        finder.setFinder( const_cast<RayClass*>(&ray) );
        finder.find( sg );
        
        if ( !finder.m_result )
            return 0;

        if ( subIdx )  *subIdx  = finder.m_subIdx;
        if ( faceIdx ) *faceIdx = finder.m_faceIdx;
        if ( t )       *t       = finder.m_t;
        if ( gravity ) *gravity = finder.m_gravity;

        return finder.m_result;
    }

    SceneNode* rayIntersectSGDetail( SceneGraph* sg, const Ray& ray,
        int* subIdx, int* faceIdx, float* t, Vector3* gravity )
    {
        return _rayIntersectSGDetail<Ray>( sg, ray, subIdx, faceIdx, t, gravity );
    }

    SceneNode* rayIntersectSGDetail( SceneGraph* sg, const LimitRay& ray,
        int* subIdx, int* faceIdx, float* t, Vector3* gravity )
    {
        return _rayIntersectSGDetail<LimitRay>( sg, ray, subIdx, faceIdx, t, gravity );
    }
}

