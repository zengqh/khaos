//=============================================================================
//CBBoxInt32
// 3D bounding box with int32 coordinates
//
//=============================================================================
// (C) 2005 ATI Research, Inc., All rights reserved.
//=============================================================================
#ifndef CBBOXINT32_H
#define CBBOXINT32_H

#include <math.h>
#include <stdio.h>

#include "Types.h"
#include "VectorMacros.h"


//bounding box class with coords specified as int32
class CBBoxInt32
{
public:
    int32 m_minCoord[3];  //upper left back corner
    int32 m_maxCoord[3];  //lower right front corner
    
    CBBoxInt32();
    
    bool8 Empty(void);
    void  Clear(void);

    void  Augment(int32 a_X, int32 a_Y, int32 a_Z); // 通过不断加入顶点来统计aabb

    void  AugmentX(int32 a_X);
    void  AugmentY(int32 a_Y);
    void  AugmentZ(int32 a_Z);
    
    void  ClampMin(int32 a_X, int32 a_Y, int32 a_Z); // aabb的min裁剪至少的位置
    void  ClampMax(int32 a_X, int32 a_Y, int32 a_Z); // aabb的max裁剪最多的位置
};

#endif //CBBOXINT32_H
