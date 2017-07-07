#pragma once
#include "KhaosVector3.h"
#include "KhaosVector2.h"

namespace Khaos 
{
	class ObjectAlignedBox
	{
    public:
        static void getCorners( const Vector3& center, // 中心位置
            const Vector3& xdir, const Vector3& ydir, const Vector3& zdir, // 坐标系
            const Vector3& halfSize, Vector3* pts ) // 半大小
        {
            Vector3 px = xdir * halfSize.x;
            Vector3 py = ydir * halfSize.y;
            Vector3 pz = zdir * halfSize.z;

            pts[0] = center - px - py - pz;
            pts[1] = center - px - py + pz;
            pts[2] = center - px + py - pz;
            pts[3] = center - px + py + pz;
            pts[4] = center + px - py - pz;
            pts[5] = center + px - py + pz;
            pts[6] = center + px + py - pz;
            pts[7] = center + px + py + pz;
        }

        static void getRange( const Vector3& xdir, const Vector3& ydir, const Vector3& zdir, // 坐标系
            const Vector3* pts, int ptsCnt, // 被投影的点
            Vector2& rangeX, Vector2& rangeY, Vector2& rangeZ ) // 范围
        {
            // M(obb->world) = |xdir, ydir, zdir|
            // M(world->obb) = inverse(|xdir, ydir, zdir|) = transpose(|xdir, ydir, zdir|)
            // V(obb) = { xdir * V(world), ydir * V(world), zdir * V(world) }

            khaosAssert( ptsCnt >= 1 );

            const Vector3& pt = pts[0];
            float x = xdir.dotProduct( pt );
            float y = ydir.dotProduct( pt );
            float z = zdir.dotProduct( pt );

            rangeX.setValue( x );
            rangeY.setValue( y );
            rangeZ.setValue( z );
            
            for ( int i =1; i < ptsCnt; ++i )
            {
                const Vector3& pt = pts[i];
                float x = xdir.dotProduct( pt );
                float y = ydir.dotProduct( pt );
                float z = zdir.dotProduct( pt );

                rangeX.makeRange( x );
                rangeY.makeRange( y );
                rangeZ.makeRange( z );
            }
	    }

        static void convertRange( const Vector3& xdir, const Vector3& ydir, const Vector3& zdir, // 坐标系
            const Vector2& rangeX, const Vector2& rangeY, const Vector2& rangeZ, // 范围
            Vector3& center, Vector3& halfSize ) // obb
        {
            halfSize.x = rangeX.getHalfRange();
            halfSize.y = rangeY.getHalfRange();
            halfSize.z = rangeZ.getHalfRange();

            center = rangeX.getMidValue() * xdir + 
                     rangeY.getMidValue() * ydir + 
                     rangeZ.getMidValue() * zdir;
        }
    };
} 

