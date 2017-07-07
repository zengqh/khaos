//--------------------------------------------------------------------------------------
//CCubeMapFilter
// Classes for filtering and processing cubemaps
//
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------
#include "CCubeMapProcessor.h"




//////////////////////////////////////////////////////////////////////////
// 全局静态数据表

//------------------------------------------------------------------------------
// D3D cube map face specification
//   mapping from 3D x,y,z cube map lookup coordinates 
//   to 2D within face u,v coordinates
//
//   --------------------> U direction 
//   |                   (within-face texture space)
//   |         _____
//   |        |     |
//   |        | +Y  |
//   |   _____|_____|_____ _____
//   |  |     |     |     |     |
//   |  | -X  | +Z  | +X  | -Z  |
//   |  |_____|_____|_____|_____|
//   |        |     |
//   |        | -Y  |
//   |        |_____|
//   |
//   v   V direction
//      (within-face texture space)
//------------------------------------------------------------------------------

//Information about neighbors and how texture coorrdinates change across faces 
//  in ORDER of left, right, top, bottom (e.g. edges corresponding to u=0, 
//  u=1, v=0, v=1 in the 2D coordinate system of the particular face.
//Note this currently assumes the D3D cube face ordering and orientation
CPCubeMapNeighbor sg_CubeNgh[6][4] =
{
    //XPOS face
    {{CP_FACE_Z_POS, CP_EDGE_RIGHT },
    {CP_FACE_Z_NEG, CP_EDGE_LEFT  },
    {CP_FACE_Y_POS, CP_EDGE_RIGHT },
    {CP_FACE_Y_NEG, CP_EDGE_RIGHT }},
    //XNEG face
    {{CP_FACE_Z_NEG, CP_EDGE_RIGHT },
    {CP_FACE_Z_POS, CP_EDGE_LEFT  },
    {CP_FACE_Y_POS, CP_EDGE_LEFT  },
    {CP_FACE_Y_NEG, CP_EDGE_LEFT  }},
    //YPOS face
    {{CP_FACE_X_NEG, CP_EDGE_TOP },
    {CP_FACE_X_POS, CP_EDGE_TOP },
    {CP_FACE_Z_NEG, CP_EDGE_TOP },
    {CP_FACE_Z_POS, CP_EDGE_TOP }},
    //YNEG face
    {{CP_FACE_X_NEG, CP_EDGE_BOTTOM},
    {CP_FACE_X_POS, CP_EDGE_BOTTOM},
    {CP_FACE_Z_POS, CP_EDGE_BOTTOM},
    {CP_FACE_Z_NEG, CP_EDGE_BOTTOM}},
    //ZPOS face
    {{CP_FACE_X_NEG, CP_EDGE_RIGHT  },
    {CP_FACE_X_POS, CP_EDGE_LEFT   },
    {CP_FACE_Y_POS, CP_EDGE_BOTTOM },
    {CP_FACE_Y_NEG, CP_EDGE_TOP    }},
    //ZNEG face
    {{CP_FACE_X_POS, CP_EDGE_RIGHT  },
    {CP_FACE_X_NEG, CP_EDGE_LEFT   },
    {CP_FACE_Y_POS, CP_EDGE_TOP    },
    {CP_FACE_Y_NEG, CP_EDGE_BOTTOM }}
};


//3x2 matrices that map cube map indexing vectors in 3d 
// (after face selection and divide through by the 
//  _ABSOLUTE VALUE_ of the max coord)
// into NVC space
//Note this currently assumes the D3D cube face ordering and orientation
#define CP_UDIR     0
#define CP_VDIR     1
#define CP_FACEAXIS 2

float32 sg_Face2DMapping[6][3][3] = {
    //XPOS face
    {{ 0,  0, -1},   //u towards negative Z
    { 0, -1,  0},   //v towards negative Y
    {1,  0,  0}},  //pos X axis  
    //XNEG face
    {{0,  0,  1},   //u towards positive Z
    {0, -1,  0},   //v towards negative Y
    {-1,  0,  0}},  //neg X axis       
    //YPOS face
    {{1, 0, 0},     //u towards positive X
    {0, 0, 1},     //v towards positive Z
    {0, 1 , 0}},   //pos Y axis  
    //YNEG face
    {{1, 0, 0},     //u towards positive X
    {0, 0 , -1},   //v towards negative Z
    {0, -1 , 0}},  //neg Y axis  
    //ZPOS face
    {{1, 0, 0},     //u towards positive X
    {0, -1, 0},    //v towards negative Y
    {0, 0,  1}},   //pos Z axis  
    //ZNEG face
    {{-1, 0, 0},    //u towards negative X
    {0, -1, 0},    //v towards negative Y
    {0, 0, -1}},   //neg Z axis  
};


//The 12 edges of the cubemap, (entries are used to index into the neighbor table)
// this table is used to average over the edges.
int32 sg_CubeEdgeList[12][2] = {
    {CP_FACE_X_POS, CP_EDGE_LEFT},
    {CP_FACE_X_POS, CP_EDGE_RIGHT},
    {CP_FACE_X_POS, CP_EDGE_TOP},
    {CP_FACE_X_POS, CP_EDGE_BOTTOM},

    {CP_FACE_X_NEG, CP_EDGE_LEFT},
    {CP_FACE_X_NEG, CP_EDGE_RIGHT},
    {CP_FACE_X_NEG, CP_EDGE_TOP},
    {CP_FACE_X_NEG, CP_EDGE_BOTTOM},

    {CP_FACE_Z_POS, CP_EDGE_TOP},
    {CP_FACE_Z_POS, CP_EDGE_BOTTOM},
    {CP_FACE_Z_NEG, CP_EDGE_TOP},
    {CP_FACE_Z_NEG, CP_EDGE_BOTTOM}
};


//Information about which of the 8 cube corners are correspond to the 
//  the 4 corners in each cube face
//  the order is upper left, upper right, lower left, lower right
int32 sg_CubeCornerList[6][4] = {
    { CP_CORNER_PPP, CP_CORNER_PPN, CP_CORNER_PNP, CP_CORNER_PNN }, // XPOS face
    { CP_CORNER_NPN, CP_CORNER_NPP, CP_CORNER_NNN, CP_CORNER_NNP }, // XNEG face
    { CP_CORNER_NPN, CP_CORNER_PPN, CP_CORNER_NPP, CP_CORNER_PPP }, // YPOS face
    { CP_CORNER_NNP, CP_CORNER_PNP, CP_CORNER_NNN, CP_CORNER_PNN }, // YNEG face
    { CP_CORNER_NPP, CP_CORNER_PPP, CP_CORNER_NNP, CP_CORNER_PNP }, // ZPOS face
    { CP_CORNER_PPN, CP_CORNER_NPN, CP_CORNER_PNN, CP_CORNER_NNN }  // ZNEG face
};




//////////////////////////////////////////////////////////////////////////
// 全局静态辅助函数

//--------------------------------------------------------------------------------------
// Error handling for cube map processor
// Pop up dialog box, and terminate application
//--------------------------------------------------------------------------------------
void CPFatalError(WCHAR *a_Msg)
{
    MessageBoxW(NULL, a_Msg, L"Error: Application Terminating", MB_OK);
    exit(EM_FATAL_ERROR);
}

//--------------------------------------------------------------------------------------
// row col to texel coord
//--------------------------------------------------------------------------------------

#if 1

void RowColToTexelCoord( int x, int y, float* u, float* v )
{
    // 现在范围一致，都是[0, size-1]
    *u = (float)x;
    *v = (float)y;
}

void TexelCoordToRowCol( float u, float v, int* x, int* y )
{
    *x = (int)u;
    *y = (int)v;
}

#else

void RowColToTexelCoord( int x, int y, float* u, float* v )
{
    // row, col [0, size-1]
    // u, v     [0, size]
    
    assert( x >= 0 && y >= 0 );

    *u = x + 0.5f; // get row center
    *v = y + 0.5f; // get col center
}

void TexelCoordToRowCol( float u, float v, int* x1, int* y1, float* ax, float* ay )
{
    assert( u >= 0 && v >= 0 );

    float fx = u - 0.5f; // 转换到浮点数的行列
    float fy = v - 0.5f; // 范围变成[-0.5, size-0.5]

    int low_x = (int)::floor( fx );
    int low_y = (int)::floor( fy );

    *x1 = low_x;
    *y1 = low_y;

    *ax = fx - low_x;
    *ay = fy - low_y;
}

#endif

//--------------------------------------------------------------------------------------
// Convert cubemap face texel coordinates and face idx to 3D vector
// note the U and V coords are float coords and range from 0 to size-1
//--------------------------------------------------------------------------------------
void TexelCoordToVector( int32 a_FaceIdx, float32 a_U, float32 a_V, int32 a_Size, float32 *a_XYZ, int32 a_FixupType )
{
    float32 nvcU, nvcV;
    float32 tempVec[3];

    if (a_FixupType == CP_FIXUP_STRETCH && a_Size > 1)
    {
        // Code from Nvtt : http://code.google.com/p/nvidia-texture-tools/source/browse/trunk/src/nvtt/CubeSurface.cpp		
        // transform from [0..res - 1] to [-1 .. 1], match up edges exactly.
        nvcU = (2.0f * (float32)a_U / ((float32)a_Size - 1.0f) ) - 1.0f;
        nvcV = (2.0f * (float32)a_V / ((float32)a_Size - 1.0f) ) - 1.0f;
    }
    else
    {
        // Change from original AMD code
        // transform from [0..res - 1] to [- (1 - 1 / res) .. (1 - 1 / res)]
        // + 0.5f is for texel center addressing
        nvcU = (2.0f * ((float32)a_U + 0.5f) / (float32)a_Size ) - 1.0f;
        nvcV = (2.0f * ((float32)a_V + 0.5f) / (float32)a_Size ) - 1.0f;
    }

    if (a_FixupType == CP_FIXUP_WARP && a_Size > 1)
    {
        // Code from Nvtt : http://code.google.com/p/nvidia-texture-tools/source/browse/trunk/src/nvtt/CubeSurface.cpp
        float32 a = powf(float32(a_Size), 2.0f) / powf(float32(a_Size - 1), 3.0f);
        nvcU = a * powf(nvcU, 3) + nvcU;
        nvcV = a * powf(nvcV, 3) + nvcV;

        // Get current vector
        //generate x,y,z vector (xform 2d NVC coord to 3D vector)
        //U contribution
        VM_SCALE3(a_XYZ, sg_Face2DMapping[a_FaceIdx][CP_UDIR], nvcU);    
        //V contribution
        VM_SCALE3(tempVec, sg_Face2DMapping[a_FaceIdx][CP_VDIR], nvcV);
        VM_ADD3(a_XYZ, tempVec, a_XYZ);
        //add face axis
        VM_ADD3(a_XYZ, sg_Face2DMapping[a_FaceIdx][CP_FACEAXIS], a_XYZ);
        //normalize vector
        VM_NORM3(a_XYZ, a_XYZ);

        return;
    }

    if (a_FixupType == CP_FIXUP_BENT && a_Size > 1)
    {
        // Method following description of Physically based rendering slides from CEDEC2011 of TriAce

        // Get vector at edge
        float32 EdgeNormalU[3];
        float32 EdgeNormalV[3];
        float32 EdgeNormal[3];
        float32 EdgeNormalMinusOne[3];

        // Recover vector at edge
        //U contribution
        VM_SCALE3(EdgeNormalU, sg_Face2DMapping[a_FaceIdx][CP_UDIR], nvcU < 0.0 ? -1.0f : 1.0f);    
        //V contribution
        VM_SCALE3(EdgeNormalV, sg_Face2DMapping[a_FaceIdx][CP_VDIR], nvcV < 0.0 ? -1.0f : 1.0f);
        VM_ADD3(EdgeNormal, EdgeNormalV, EdgeNormalU);
        //add face axis
        VM_ADD3(EdgeNormal, sg_Face2DMapping[a_FaceIdx][CP_FACEAXIS], EdgeNormal);
        //normalize vector
        VM_NORM3(EdgeNormal, EdgeNormal);

        // Get vector at (edge - 1)
        float32 nvcUEdgeMinus1 = (2.0f * ((float32)(nvcU < 0.0f ? 0 : a_Size-1) + 0.5f) / (float32)a_Size ) - 1.0f;
        float32 nvcVEdgeMinus1 = (2.0f * ((float32)(nvcV < 0.0f ? 0 : a_Size-1) + 0.5f) / (float32)a_Size ) - 1.0f;

        // Recover vector at (edge - 1)
        //U contribution
        VM_SCALE3(EdgeNormalU, sg_Face2DMapping[a_FaceIdx][CP_UDIR], nvcUEdgeMinus1);    
        //V contribution
        VM_SCALE3(EdgeNormalV, sg_Face2DMapping[a_FaceIdx][CP_VDIR], nvcVEdgeMinus1);
        VM_ADD3(EdgeNormalMinusOne, EdgeNormalV, EdgeNormalU);
        //add face axis
        VM_ADD3(EdgeNormalMinusOne, sg_Face2DMapping[a_FaceIdx][CP_FACEAXIS], EdgeNormalMinusOne);
        //normalize vector
        VM_NORM3(EdgeNormalMinusOne, EdgeNormalMinusOne);

        // Get angle between the two vector (which is 50% of the two vector presented in the TriAce slide)
        float32 AngleNormalEdge = acosf(VM_DOTPROD3(EdgeNormal, EdgeNormalMinusOne));

        // Here we assume that high resolution required less offset than small resolution (TriAce based this on blur radius and custom value) 
        // Start to increase from 50% to 100% target angle from 128x128x6 to 1x1x6
        float32 NumLevel = (logf((float)min(a_Size, 128))  / logf(2)) - 1;
        AngleNormalEdge = VM_LERP_T(0.5f * AngleNormalEdge, AngleNormalEdge, 1.0f - (NumLevel/6) );

        float32 factorU = abs((2.0f * ((float32)a_U) / (float32)(a_Size - 1) ) - 1.0f);
        float32 factorV = abs((2.0f * ((float32)a_V) / (float32)(a_Size - 1) ) - 1.0f);
        AngleNormalEdge = VM_LERP_T(0.0f, AngleNormalEdge, max(factorU, factorV) );

        // Get current vector
        //generate x,y,z vector (xform 2d NVC coord to 3D vector)
        //U contribution
        VM_SCALE3(a_XYZ, sg_Face2DMapping[a_FaceIdx][CP_UDIR], nvcU);    
        //V contribution
        VM_SCALE3(tempVec, sg_Face2DMapping[a_FaceIdx][CP_VDIR], nvcV);
        VM_ADD3(a_XYZ, tempVec, a_XYZ);
        //add face axis
        VM_ADD3(a_XYZ, sg_Face2DMapping[a_FaceIdx][CP_FACEAXIS], a_XYZ);
        //normalize vector
        VM_NORM3(a_XYZ, a_XYZ);

        float32 RadiantAngle = AngleNormalEdge;
        // Get angle between face normal and current normal. Used to push the normal away from face normal.
        float32 AngleFaceVector = acosf(VM_DOTPROD3(sg_Face2DMapping[a_FaceIdx][CP_FACEAXIS], a_XYZ));

        // Push the normal away from face normal by an angle of RadiantAngle
        VM_SLERP(a_XYZ, sg_Face2DMapping[a_FaceIdx][CP_FACEAXIS], a_XYZ, 1.0f + RadiantAngle / AngleFaceVector);

        return;
    }

    //generate x,y,z vector (xform 2d NVC coord to 3D vector)
    //U contribution
    VM_SCALE3(a_XYZ, sg_Face2DMapping[a_FaceIdx][CP_UDIR], nvcU);    
    //V contribution
    VM_SCALE3(tempVec, sg_Face2DMapping[a_FaceIdx][CP_VDIR], nvcV);
    VM_ADD3(a_XYZ, tempVec, a_XYZ);
    //add face axis
    VM_ADD3(a_XYZ, sg_Face2DMapping[a_FaceIdx][CP_FACEAXIS], a_XYZ);

    //normalize vector
    VM_NORM3(a_XYZ, a_XYZ);
}

//--------------------------------------------------------------------------------------
// Convert 3D vector to cubemap face texel coordinates and face idx 
//  note the U and V coords are float coords and range from 0 to size-1
// returns face IDX and texel coords
//--------------------------------------------------------------------------------------
void VectorToTexelCoord( const float32 *a_XYZ, int32 a_Size, int32 *a_FaceIdx, float32 *a_U, float32 *a_V )
{
    float32 nvcU, nvcV;
    float32 absXYZ[3];
    float32 maxCoord;
    float32 onFaceXYZ[3];
    int32   faceIdx;

    //absolute value 3
    VM_ABS3(absXYZ, a_XYZ);

    if( (absXYZ[0] >= absXYZ[1]) && (absXYZ[0] >= absXYZ[2]) )
    {
        maxCoord = absXYZ[0];

        if(a_XYZ[0] >= 0) //face = XPOS
        {
            faceIdx = CP_FACE_X_POS;            
        }    
        else
        {
            faceIdx = CP_FACE_X_NEG;                    
        }
    }
    else if ( (absXYZ[1] >= absXYZ[0]) && (absXYZ[1] >= absXYZ[2]) )
    {
        maxCoord = absXYZ[1];

        if(a_XYZ[1] >= 0) //face = XPOS
        {
            faceIdx = CP_FACE_Y_POS;            
        }    
        else
        {
            faceIdx = CP_FACE_Y_NEG;                    
        }    
    }
    else  // if( (absXYZ[2] > absXYZ[0]) && (absXYZ[2] > absXYZ[1]) )
    {
        maxCoord = absXYZ[2];

        if(a_XYZ[2] >= 0) //face = XPOS
        {
            faceIdx = CP_FACE_Z_POS;            
        }    
        else
        {
            faceIdx = CP_FACE_Z_NEG;                    
        }    
    }

    //divide through by max coord so face vector lies on cube face
    VM_SCALE3(onFaceXYZ, a_XYZ, 1.0f/maxCoord);
    nvcU = VM_DOTPROD3(sg_Face2DMapping[ faceIdx ][CP_UDIR], onFaceXYZ );
    nvcV = VM_DOTPROD3(sg_Face2DMapping[ faceIdx ][CP_VDIR], onFaceXYZ );

    // Modify original AMD code to return value from 0 to Size - 1
    float u = floor( (a_Size - 1) * 0.5f * (nvcU + 1.0f) );
    float v = floor( (a_Size - 1) * 0.5f * (nvcV + 1.0f) );

    *a_FaceIdx = faceIdx;
    *a_U = u;
    *a_V = v;
}

//--------------------------------------------------------------------------------------
// gets texel ptr in a cube map given a direction vector, and an array of 
//  CImageSurfaces that represent the cube faces.
//   
//--------------------------------------------------------------------------------------

float32* GetCubeMapTexelPtr( CImageSurface* a_Surface, const float32* a_XYZ )
{
    int32   faceIdx;    
    float32 u, v;

    // get face idx and u, v texel coordinate in face
    VectorToTexelCoord( a_XYZ, a_Surface[0].m_Width, &faceIdx, &u, &v );

    // to row col
    int x, y;
    TexelCoordToRowCol( u, v, &x, &y );

    float* src = a_Surface[faceIdx].GetSurfaceTexelPtr(x, y);
    return src;
}

#if 0

void GetCubeMapTexelPtr_Linear( CImageSurface* a_Surface, const float32* a_XYZ, float32* a_Ret )
{
    int32   faceIdx;    
    float32 u, v;

    // get face idx and u, v texel coordinate in face
    VectorToTexelCoord( a_XYZ, a_Surface[0].m_Width, &faceIdx, &u, &v );

    // to row col
    int x, y;
    float ax, ay;
    TexelCoordToRowCol( u, v, &x, &y, &ax, &ay );

    // do linear lerp
    CImageSurface& img = a_Surface[faceIdx];

    int lowx = (x >= 0 ? x : 0);
    int lowy = (y >= 0 ? y : 0);

    ++x;
    ++y;
    
    int highx = (x < img.m_Width  ? x : (img.m_Width - 1));
    int highy = (y < img.m_Height ? y : (img.m_Height - 1));

    float* valA = img.GetSurfaceTexelPtr(lowx, lowy);
    float* valB = img.GetSurfaceTexelPtr(highx, lowy);
    float* valC = img.GetSurfaceTexelPtr(lowx, highy);
    float* valD = img.GetSurfaceTexelPtr(highx, highy);

    int channels = img.m_NumChannels;

    float valAB[32];
    VM_LERP_N( valAB, valA, valB, channels, ax ); // 先水平插值

    float valCD[32];
    VM_LERP_N( valCD, valC, valD, channels, ax ); // 先水平插值

    VM_LERP_N( a_Ret, valAB, valCD, channels, ay ); // 再垂直插值
}

void GetCubeMapTexelPtr_Nearest( CImageSurface* a_Surface, const float32* a_XYZ, float32* a_Ret )
{
    int32   faceIdx;    
    float32 u, v;

    // get face idx and u, v texel coordinate in face
    VectorToTexelCoord( a_XYZ, a_Surface[0].m_Width, &faceIdx, &u, &v );

    // to row col
    int x, y;
    float ax, ay;
    TexelCoordToRowCol( u, v, &x, &y, &ax, &ay );

    // nearest get
    if ( ax > 0.5f ) ++x;
    if ( ay > 0.5f ) ++y;

    VM_CLAMP( x, x, 0, a_Surface[0].m_Width-1 );
    VM_CLAMP( y, y, 0, a_Surface[0].m_Height-1 );

    const float* src = a_Surface[faceIdx].GetSurfaceTexelPtr(x, y);

    for ( int i = 0; i < a_Surface[faceIdx].m_NumChannels; ++i )
        a_Ret[i] = src[i];
}

#endif

//--------------------------------------------------------------------------------------
// Fixup cube edges
//
// average texels on cube map faces across the edges
//--------------------------------------------------------------------------------------
void FixupCubeEdges(CImageSurface *a_CubeMap, int32 a_FixupType, int32 a_FixupWidth)
{
    int32 i, j, k;
    int32 face;
    int32 edge;
    int32 neighborFace;
    int32 neighborEdge;

    int32 nChannels = a_CubeMap[0].m_NumChannels;
    int32 size = a_CubeMap[0].m_Width;

    CPCubeMapNeighbor neighborInfo;

    CP_ITYPE* edgeStartPtr;
    CP_ITYPE* neighborEdgeStartPtr;

    int32 edgeWalk;
    int32 neighborEdgeWalk;

    //pointer walk to walk one texel away from edge in perpendicular direction
    int32 edgePerpWalk;
    int32 neighborEdgePerpWalk;

    //number of texels inward towards cubeface center to apply fixup to
    int32 fixupDist;
    int32 iFixup;   

    // note that if functionality to filter across the three texels for each corner, then 
    CP_ITYPE *cornerPtr[8][3];      //indexed by corner and face idx
    CP_ITYPE *faceCornerPtrs[4];    //corner pointers for face
    int32 cornerNumPtrs[8];         //indexed by corner and face idx
    int32 iCorner;                  //corner iterator
    int32 iFace;                    //iterator for faces
    int32 corner;

    //if there is no fixup, or fixup width = 0, do nothing
    if((a_FixupType == CP_FIXUP_NONE) ||
        (a_FixupWidth == 0)
        // new fixup method
        || (a_FixupType == CP_FIXUP_BENT && a_CubeMap[0].m_Width != 1) // In case of Bent Fixup and width of 1, we take the average of the texel color.
        || (a_FixupType == CP_FIXUP_WARP && a_CubeMap[0].m_Width != 1)
        || (a_FixupType == CP_FIXUP_STRETCH && a_CubeMap[0].m_Width != 1)	  
        // new fixup method
        )
    {
        return;
    }

    //special case 1x1 cubemap, average face colors
    if( a_CubeMap[0].m_Width == 1 )
    {
        //iterate over channels
        for(k=0; k<nChannels; k++)
        {   
            CP_ITYPE accum = 0.0f;

            //iterate over faces to accumulate face colors
            for(iFace=0; iFace<6; iFace++)
            {
                accum += *(a_CubeMap[iFace].m_ImgData + k);
            }

            //compute average over 6 face colors
            accum /= 6.0f;

            //iterate over faces to distribute face colors
            for(iFace=0; iFace<6; iFace++)
            {
                *(a_CubeMap[iFace].m_ImgData + k) = accum;
            }
        }

        return;
    }


    //iterate over corners
    for(iCorner = 0; iCorner < 8; iCorner++ )
    {
        cornerNumPtrs[iCorner] = 0;
    }

    //iterate over faces to collect list of corner texel pointers
    for(iFace=0; iFace<6; iFace++ )
    {
        //the 4 corner pointers for this face
        faceCornerPtrs[0] = a_CubeMap[iFace].m_ImgData;
        faceCornerPtrs[1] = a_CubeMap[iFace].m_ImgData + ( (size - 1) * nChannels );
        faceCornerPtrs[2] = a_CubeMap[iFace].m_ImgData + ( (size) * (size - 1) * nChannels );
        faceCornerPtrs[3] = a_CubeMap[iFace].m_ImgData + ( (((size) * (size - 1)) + (size - 1)) * nChannels );

        //iterate over face corners to collect cube corner pointers
        for(i=0; i<4; i++ )
        {
            corner = sg_CubeCornerList[iFace][i];   
            cornerPtr[corner][ cornerNumPtrs[corner] ] = faceCornerPtrs[i];
            cornerNumPtrs[corner]++;
        }
    }


    //iterate over corners to average across corner tap values
    for(iCorner = 0; iCorner < 8; iCorner++ )
    {
        for(k=0; k<nChannels; k++)
        {             
            CP_ITYPE cornerTapAccum;

            cornerTapAccum = 0.0f;

            //iterate over corner texels and average results
            for(i=0; i<3; i++ )
            {
                cornerTapAccum += *(cornerPtr[iCorner][i] + k);
            }

            //divide by 3 to compute average of corner tap values
            cornerTapAccum *= (1.0f / 3.0f);

            //iterate over corner texels and average results
            for(i=0; i<3; i++ )
            {
                *(cornerPtr[iCorner][i] + k) = cornerTapAccum;
            }
        }
    }   


    //maximum width of fixup region is one half of the cube face size
    fixupDist = VM_MIN( a_FixupWidth, size / 2);

    //iterate over the twelve edges of the cube to average across edges
    for(i=0; i<12; i++)
    {
        face = sg_CubeEdgeList[i][0];
        edge = sg_CubeEdgeList[i][1];

        neighborInfo = sg_CubeNgh[face][edge];
        neighborFace = neighborInfo.m_Face;
        neighborEdge = neighborInfo.m_Edge;

        edgeStartPtr = a_CubeMap[face].m_ImgData;
        neighborEdgeStartPtr = a_CubeMap[neighborFace].m_ImgData;
        edgeWalk = 0;
        neighborEdgeWalk = 0;

        //amount to pointer to sample taps away from cube face
        edgePerpWalk = 0;
        neighborEdgePerpWalk = 0;

        //Determine walking pointers based on edge type
        // e.g. CP_EDGE_LEFT, CP_EDGE_RIGHT, CP_EDGE_TOP, CP_EDGE_BOTTOM
        switch(edge)
        {
        case CP_EDGE_LEFT:
            // no change to faceEdgeStartPtr  
            edgeWalk = nChannels * size;
            edgePerpWalk = nChannels;
            break;
        case CP_EDGE_RIGHT:
            edgeStartPtr += (size - 1) * nChannels;
            edgeWalk = nChannels * size;
            edgePerpWalk = -nChannels;
            break;
        case CP_EDGE_TOP:
            // no change to faceEdgeStartPtr  
            edgeWalk = nChannels;
            edgePerpWalk = nChannels * size;
            break;
        case CP_EDGE_BOTTOM:
            edgeStartPtr += (size) * (size - 1) * nChannels;
            edgeWalk = nChannels;
            edgePerpWalk = -(nChannels * size);
            break;
        }

        //For certain types of edge abutments, the neighbor edge walk needs to 
        //  be flipped: the cases are 
        // if a left   edge mates with a left or bottom  edge on the neighbor
        // if a top    edge mates with a top or right edge on the neighbor
        // if a right  edge mates with a right or top edge on the neighbor
        // if a bottom edge mates with a bottom or left  edge on the neighbor
        //Seeing as the edges are enumerated as follows 
        // left   =0 
        // right  =1 
        // top    =2 
        // bottom =3            
        // 
        //If the edge enums are the same, or the sum of the enums == 3, 
        //  the neighbor edge walk needs to be flipped
        if( (edge == neighborEdge) || ((edge + neighborEdge) == 3) )
        {   //swapped direction neighbor edge walk
            switch(neighborEdge)
            {
            case CP_EDGE_LEFT:  //start at lower left and walk up
                neighborEdgeStartPtr += (size - 1) * (size) *  nChannels;
                neighborEdgeWalk = -(nChannels * size);
                neighborEdgePerpWalk = nChannels;
                break;
            case CP_EDGE_RIGHT: //start at lower right and walk up
                neighborEdgeStartPtr += ((size - 1)*(size) + (size - 1)) * nChannels;
                neighborEdgeWalk = -(nChannels * size);
                neighborEdgePerpWalk = -nChannels;
                break;
            case CP_EDGE_TOP:   //start at upper right and walk left
                neighborEdgeStartPtr += (size - 1) * nChannels;
                neighborEdgeWalk = -nChannels;
                neighborEdgePerpWalk = (nChannels * size);
                break;
            case CP_EDGE_BOTTOM: //start at lower right and walk left
                neighborEdgeStartPtr += ((size - 1)*(size) + (size - 1)) * nChannels;
                neighborEdgeWalk = -nChannels;
                neighborEdgePerpWalk = -(nChannels * size);
                break;
            }            
        }
        else
        { //swapped direction neighbor edge walk
            switch(neighborEdge)
            {
            case CP_EDGE_LEFT: //start at upper left and walk down
                //no change to neighborEdgeStartPtr for this case since it points 
                // to the upper left corner already
                neighborEdgeWalk = nChannels * size;
                neighborEdgePerpWalk = nChannels;
                break;
            case CP_EDGE_RIGHT: //start at upper right and walk down
                neighborEdgeStartPtr += (size - 1) * nChannels;
                neighborEdgeWalk = nChannels * size;
                neighborEdgePerpWalk = -nChannels;
                break;
            case CP_EDGE_TOP:   //start at upper left and walk left
                //no change to neighborEdgeStartPtr for this case since it points 
                // to the upper left corner already
                neighborEdgeWalk = nChannels;
                neighborEdgePerpWalk = (nChannels * size);
                break;
            case CP_EDGE_BOTTOM: //start at lower left and walk left
                neighborEdgeStartPtr += (size) * (size - 1) * nChannels;
                neighborEdgeWalk = nChannels;
                neighborEdgePerpWalk = -(nChannels * size);
                break;
            }
        }


        //Perform edge walk, to average across the 12 edges and smoothly propagate change to 
        //nearby neighborhood

        //step ahead one texel on edge
        edgeStartPtr += edgeWalk;
        neighborEdgeStartPtr += neighborEdgeWalk;

        // note that this loop does not process the corner texels, since they have already been
        //  averaged across faces across earlier
        for(j=1; j<(size - 1); j++)       
        {             
            //for each set of taps along edge, average them
            // and rewrite the results into the edges
            for(k = 0; k<nChannels; k++)
            {             
                CP_ITYPE edgeTap, neighborEdgeTap, avgTap;  //edge tap, neighborEdgeTap and the average of the two
                CP_ITYPE edgeTapDev, neighborEdgeTapDev;

                edgeTap = *(edgeStartPtr + k);
                neighborEdgeTap = *(neighborEdgeStartPtr + k);

                //compute average of tap intensity values
                avgTap = 0.5f * (edgeTap + neighborEdgeTap);

                //propagate average of taps to edge taps
                (*(edgeStartPtr + k)) = avgTap;
                (*(neighborEdgeStartPtr + k)) = avgTap;

                edgeTapDev = edgeTap - avgTap;
                neighborEdgeTapDev = neighborEdgeTap - avgTap;

                //iterate over taps in direction perpendicular to edge, and 
                //  adjust intensity values gradualy to obscure change in intensity values of 
                //  edge averaging.
                for(iFixup = 1; iFixup < fixupDist; iFixup++)
                {
                    //fractional amount to apply change in tap intensity along edge to taps 
                    //  in a perpendicular direction to edge 
                    CP_ITYPE fixupFrac = (CP_ITYPE)(fixupDist - iFixup) / (CP_ITYPE)(fixupDist); 
                    CP_ITYPE fixupWeight;

                    switch(a_FixupType )
                    {
                    case CP_FIXUP_PULL_LINEAR:
                        {
                            fixupWeight = fixupFrac;
                        }
                        break;
                    case CP_FIXUP_PULL_HERMITE:
                        {
                            //hermite spline interpolation between 1 and 0 with both pts derivatives = 0 
                            // e.g. smooth step
                            // the full formula for hermite interpolation is:
                            //              
                            //                  [  2  -2   1   1 ][ p0 ] 
                            // [t^3  t^2  t  1 ][ -3   3  -2  -1 ][ p1 ]
                            //                  [  0   0   1   0 ][ d0 ]
                            //                  [  1   0   0   0 ][ d1 ]
                            // 
                            // Where p0 and p1 are the point locations and d0, and d1 are their respective derivatives
                            // t is the parameteric coordinate used to specify an interpoltion point on the spline
                            // and ranges from 0 to 1.
                            //  if p0 = 0 and p1 = 1, and d0 and d1 = 0, the interpolation reduces to
                            //
                            //  p(t) =  - 2t^3 + 3t^2
                            fixupWeight = ((-2.0f * fixupFrac + 3.0f) * fixupFrac * fixupFrac);
                        }
                        break;
                    case CP_FIXUP_AVERAGE_LINEAR:
                        {
                            fixupWeight = fixupFrac;

                            //perform weighted average of edge tap value and current tap
                            // fade off weight linearly as a function of distance from edge
                            edgeTapDev = 
                                (*(edgeStartPtr + (iFixup * edgePerpWalk) + k)) - avgTap;
                            neighborEdgeTapDev = 
                                (*(neighborEdgeStartPtr + (iFixup * neighborEdgePerpWalk) + k)) - avgTap;
                        }
                        break;
                    case CP_FIXUP_AVERAGE_HERMITE:
                        {
                            fixupWeight = ((-2.0f * fixupFrac + 3.0f) * fixupFrac * fixupFrac);

                            //perform weighted average of edge tap value and current tap
                            // fade off weight using hermite spline with distance from edge
                            //  as parametric coordinate
                            edgeTapDev = 
                                (*(edgeStartPtr + (iFixup * edgePerpWalk) + k)) - avgTap;
                            neighborEdgeTapDev = 
                                (*(neighborEdgeStartPtr + (iFixup * neighborEdgePerpWalk) + k)) - avgTap;
                        }
                        break;
                    }

                    // vary intensity of taps within fixup region toward edge values to hide changes made to edge taps
                    *(edgeStartPtr + (iFixup * edgePerpWalk) + k) -= (fixupWeight * edgeTapDev);
                    *(neighborEdgeStartPtr + (iFixup * neighborEdgePerpWalk) + k) -= (fixupWeight * neighborEdgeTapDev);
                }

            }

            edgeStartPtr += edgeWalk;
            neighborEdgeStartPtr += neighborEdgeWalk;
        }        
    }
}


//////////////////////////////////////////////////////////////////////////
/** Original code from Ignacio Casta駉
* This formula is from Manne 謍rstr鰉's thesis.
* Take two coordiantes in the range [-1, 1] that define a portion of a
* cube face and return the area of the projection of that portion on the
* surface of the sphere.
**/

inline float32 _AreaElement( float32 x, float32 y )
{
    return atan2(x * y, sqrt(x * x + y * y + 1));
}

float32 _TexelCoordSolidAngle(int32 a_FaceIdx, float32 a_U, float32 a_V, int32 a_Size)
{
    // transform from [0..res - 1] to [- (1 - 1 / res) .. (1 - 1 / res)]
    // (+ 0.5f is for texel center addressing)
    float32 U = (2.0f * ((float32)a_U + 0.5f) / (float32)a_Size ) - 1.0f;
    float32 V = (2.0f * ((float32)a_V + 0.5f) / (float32)a_Size ) - 1.0f;

    // Shift from a demi texel, mean 1.0f / a_Size with U and V in [-1..1]
    float32 InvResolution = 1.0f / a_Size;

    // U and V are the -1..1 texture coordinate on the current face.
    // Get projected area for this texel
    float32 x0 = U - InvResolution;
    float32 y0 = V - InvResolution;
    float32 x1 = U + InvResolution;
    float32 y1 = V + InvResolution;
    float32 SolidAngle = _AreaElement(x0, y0) - _AreaElement(x0, y1) - _AreaElement(x1, y0) + _AreaElement(x1, y1);

    return SolidAngle;
}

int32 _CalcHemiFilterSize( int32 a_CubeSize )
{
    // min angle a src texel can cover
    // 1 / cube size = 0.5 / half cube size
    const double minSrcTexelAngle = atan2(1.0, (double)a_CubeSize);  

    // we use 180 cone angle
    // filter angle is 1/2 the cone angle
    const double filterAngle = (CP_PI * 0.5);

    // the maximum number of texels in 1D the filter cone angle will cover
    // used to determine bounding box size for filter extents
    int32 filterSize = (int32)ceil(filterAngle / minSrcTexelAngle);
    return filterSize;
}

void _DetermineFilterExtents( const float32 *a_CenterTapDir, int32 a_SrcSize, int32 a_BBoxSize, 
                             CBBoxInt32 *a_FilterExtents )
{
    int32 u, v;
    int32 faceIdx;
    int32 minU, minV, maxU, maxV;
    int32 i;

    //neighboring face and bleed over amount, and width of BBOX for
    // left, right, top, and bottom edges of this face
    int32 bleedOverAmount[4];
    int32 bleedOverBBoxMin[4];
    int32 bleedOverBBoxMax[4];

    int32 neighborFace;
    int32 neighborEdge;

    // clear first
    //for( int iCubeFaces=0; iCubeFaces<6; iCubeFaces++)
    //{
    //    a_FilterExtents[iCubeFaces].Clear();    
    //}

    //get face idx, and u, v info from center tap dir
    float32 u_f, v_f;
    VectorToTexelCoord( a_CenterTapDir, a_SrcSize, &faceIdx, &u_f, &v_f );
    u = (int32)u_f;
    v = (int32)v_f;

    //define bbox size within face
    a_FilterExtents[faceIdx].Augment(u - a_BBoxSize, v - a_BBoxSize, 0);
    a_FilterExtents[faceIdx].Augment(u + a_BBoxSize, v + a_BBoxSize, 0);

    a_FilterExtents[faceIdx].ClampMin(0, 0, 0);
    a_FilterExtents[faceIdx].ClampMax(a_SrcSize-1, a_SrcSize-1, 0);

    //u and v extent in face corresponding to center tap
    minU = a_FilterExtents[faceIdx].m_minCoord[0];
    minV = a_FilterExtents[faceIdx].m_minCoord[1];
    maxU = a_FilterExtents[faceIdx].m_maxCoord[0];
    maxV = a_FilterExtents[faceIdx].m_maxCoord[1];

    //bleed over amounts for face across u=0 edge (left)    
    bleedOverAmount[0] = (a_BBoxSize - u);
    bleedOverBBoxMin[0] = minV;
    bleedOverBBoxMax[0] = maxV;

    //bleed over amounts for face across u=1 edge (right)    
    bleedOverAmount[1] = (u + a_BBoxSize) - (a_SrcSize-1);
    bleedOverBBoxMin[1] = minV;
    bleedOverBBoxMax[1] = maxV;

    //bleed over to face across v=0 edge (up)
    bleedOverAmount[2] = (a_BBoxSize - v);
    bleedOverBBoxMin[2] = minU;
    bleedOverBBoxMax[2] = maxU;

    //bleed over to face across v=1 edge (down)
    bleedOverAmount[3] = (v + a_BBoxSize) - (a_SrcSize-1);
    bleedOverBBoxMin[3] = minU;
    bleedOverBBoxMax[3] = maxU;

    //compute bleed over regions in neighboring faces
    for(i=0; i<4; i++)
    {
        if(bleedOverAmount[i] > 0)
        {
            neighborFace = sg_CubeNgh[faceIdx][i].m_Face;
            neighborEdge = sg_CubeNgh[faceIdx][i].m_Edge;

            //For certain types of edge abutments, the bleedOverBBoxMin, and bleedOverBBoxMax need to 
            //  be flipped: the cases are 
            // if a left   edge mates with a left or bottom  edge on the neighbor
            // if a top    edge mates with a top or right edge on the neighbor
            // if a right  edge mates with a right or top edge on the neighbor
            // if a bottom edge mates with a bottom or left  edge on the neighbor
            //Seeing as the edges are enumerated as follows 
            // left   =0 
            // right  =1 
            // top    =2 
            // bottom =3            
            // 
            // so if the edge enums are the same, or the sum of the enums == 3, 
            //  the bbox needs to be flipped
            if( (i == neighborEdge) || ((i+neighborEdge) == 3) )
            {
                bleedOverBBoxMin[i] = (a_SrcSize-1) - bleedOverBBoxMin[i];
                bleedOverBBoxMax[i] = (a_SrcSize-1) - bleedOverBBoxMax[i];
            }


            //The way the bounding box is extended onto the neighboring face
            // depends on which edge of neighboring face abuts with this one
            switch(sg_CubeNgh[faceIdx][i].m_Edge)
            {
            case CP_EDGE_LEFT:
                a_FilterExtents[neighborFace].Augment(0, bleedOverBBoxMin[i], 0);
                a_FilterExtents[neighborFace].Augment(bleedOverAmount[i], bleedOverBBoxMax[i], 0);
                break;
            case CP_EDGE_RIGHT:                
                a_FilterExtents[neighborFace].Augment( (a_SrcSize-1), bleedOverBBoxMin[i], 0);
                a_FilterExtents[neighborFace].Augment( (a_SrcSize-1) - bleedOverAmount[i], bleedOverBBoxMax[i], 0);
                break;
            case CP_EDGE_TOP:   
                a_FilterExtents[neighborFace].Augment(bleedOverBBoxMin[i], 0, 0);
                a_FilterExtents[neighborFace].Augment(bleedOverBBoxMax[i], bleedOverAmount[i], 0);
                break;
            case CP_EDGE_BOTTOM:   
                a_FilterExtents[neighborFace].Augment(bleedOverBBoxMin[i], (a_SrcSize-1), 0);
                a_FilterExtents[neighborFace].Augment(bleedOverBBoxMax[i], (a_SrcSize-1) - bleedOverAmount[i], 0);            
                break;
            }

            //clamp filter extents in non-center tap faces to remain within surface
            a_FilterExtents[neighborFace].ClampMin(0, 0, 0);
            a_FilterExtents[neighborFace].ClampMax(a_SrcSize-1, a_SrcSize-1, 0);
        }

        //If the bleed over amount bleeds past the adjacent face onto the opposite face 
        // from the center tap face, then process the opposite face entirely for now. 
        //Note that the cases in which this happens, what usually happens is that 
        // more than one edge bleeds onto the opposite face, and the bounding box 
        // encompasses the entire cube map face.
        if(bleedOverAmount[i] > a_SrcSize)
        {
            uint32 oppositeFaceIdx; 

            //determine opposite face 
            switch(faceIdx)
            {
            case CP_FACE_X_POS:
                oppositeFaceIdx = CP_FACE_X_NEG;
                break;
            case CP_FACE_X_NEG:
                oppositeFaceIdx = CP_FACE_X_POS;
                break;
            case CP_FACE_Y_POS:
                oppositeFaceIdx = CP_FACE_Y_NEG;
                break;
            case CP_FACE_Y_NEG:
                oppositeFaceIdx = CP_FACE_Y_POS;
                break;
            case CP_FACE_Z_POS:
                oppositeFaceIdx = CP_FACE_Z_NEG;
                break;
            case CP_FACE_Z_NEG:
                oppositeFaceIdx = CP_FACE_Z_POS;
                break;
            default:
                break;
            }

            //just encompass entire face for now
            a_FilterExtents[oppositeFaceIdx].Augment(0, 0, 0);
            a_FilterExtents[oppositeFaceIdx].Augment((a_SrcSize-1), (a_SrcSize-1), 0);            
        }
    }

    minV=minV;
}

//////////////////////////////////////////////////////////////////////////
// CCubeMapProcessor实现方法
CCubeMapProcessor::CCubeMapProcessor(void)
{
    m_InputSize = 0;
    m_OutputSize = 0;
    m_NumInputMipLevels = 0;
    m_NumMipLevels = 0;
    m_NumChannels = 0;

    m_FilterSize = 0;

    m_InputSurfaceMips[0] = m_InputSurface;
    for ( int i = 1; i < CP_MAX_MIPLEVELS; ++i )
        m_InputSurfaceMips[i] = m_InputSurfaceMipsBuffer[i-1];
}

CCubeMapProcessor::~CCubeMapProcessor()
{
    Clear();
}


//--------------------------------------------------------------------------------------
//Init cube map processor
//
// note that if a_MaxNumMipLevels is set to 0, the entire mip chain will be created
//  similar to the way the D3D tools allow you to specify 0 to denote an entire mip chain
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::Init(int32 a_InputSize, int32 a_OutputSize, int32 a_MaxNumMipLevels, int32 a_NumChannels)
{
    int32 i, j;
    int32 mipLevelSize;
    int32 maxNumMipLevels;

    m_InputSize = a_InputSize;
    m_OutputSize = a_OutputSize;

    m_NumChannels = a_NumChannels;

    maxNumMipLevels = a_MaxNumMipLevels;

    //if nax num mip levels is set to 0, set it to generate the entire mip chain
    if(maxNumMipLevels == 0 )
    {
        maxNumMipLevels = CP_MAX_MIPLEVELS;
    }

    //Iterate over faces for input images
    for(i=0; i<6; i++)
    {
        m_InputSurface[i].Init(m_InputSize, m_InputSize, m_NumChannels );
    }

    //zero mip levels constructed so far
    m_NumMipLevels = 0;

    //first miplevel size 
    mipLevelSize = m_OutputSize;

    //Iterate over mip chain, and init CImageSurfaces for mip-chain
    for(j=0; j<maxNumMipLevels; j++)
    {
        //Iterate over faces for output images
        for(i=0; i<6; i++)
        {
            m_OutputSurface[j][i].Init(mipLevelSize, mipLevelSize, a_NumChannels );            
        }

        //next mip level is half size
        mipLevelSize >>= 1;

        m_NumMipLevels++;

        //terminate if mip chain becomes too small
        if(mipLevelSize == 0)
        {            
            return;
        }
    }
}


//--------------------------------------------------------------------------------------
// Builds a normalizer|solid angle cubemap of size a_Size. This routine deallocates the CImageSurfaces 
// passed into the the function and reallocates them with the correct size and 4 channels to store the 
// normalized vector, and solid angle for each texel.
//
// a_Size         [in]
// a_Surface      [out]  Pointer to array of 6 CImageSurfaces where normalizer cube faces will be stored
//
void CCubeMapProcessor::BuildNormalizerSolidAngleCubemap( int32 a_Size, int32 a_FixupType )
{
    CImageSurface *a_Surface = m_NormCubeMap; // output to norm cube map

    int32 iCubeFace, u, v;

    //iterate over cube faces
    for(iCubeFace=0; iCubeFace<6; iCubeFace++)
    {
        a_Surface[iCubeFace].Clear();
        a_Surface[iCubeFace].Init(a_Size, a_Size, 4);  //First three channels for norm cube, and last channel for solid angle

        //fast texture walk, build normalizer cube map
        CP_ITYPE *texelPtr = a_Surface[iCubeFace].m_ImgData;

        for(v=0; v<a_Surface[iCubeFace].m_Height; v++)
        {
            for(u=0; u<a_Surface[iCubeFace].m_Width; u++)
            {
                TexelCoordToVector(iCubeFace, (float32)u, (float32)v, a_Size, texelPtr, a_FixupType);

                *(texelPtr + 3) = _TexelCoordSolidAngle(iCubeFace, (float32)u, (float32)v, a_Size);

                texelPtr += a_Surface[iCubeFace].m_NumChannels;
            }         
        }
    }
}


//--------------------------------------------------------------------------------------
// Stop any currently running threads, and clear all allocated data from cube map 
//   processor.
//
// To use the cube map processor after calling Clear(....), you need to call Init(....) 
//   again
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::Clear(void)
{
    int32 i, j;

    m_InputSize = 0;             
    m_OutputSize = 0;             
    m_NumMipLevels = 0;     
    m_NumChannels = 0; 

    //Iterate over faces for input images
    for(i=0; i<6; i++)
    {
        m_InputSurface[i].Clear();
    }

    for(j=0; j<CP_MAX_MIPLEVELS-1; j++)
    {
        for(i=0; i<6; i++)
            m_InputSurfaceMipsBuffer[j][i].Clear();
    }

    //Iterate over mip chain, and allocate memory for mip-chain
    for(j=0; j<CP_MAX_MIPLEVELS; j++)
    {
        //Iterate over faces for output images
        for(i=0; i<6; i++)
        {
            m_OutputSurface[j][i].Clear();            
        }
    }
}

//--------------------------------------------------------------------------------------
//Copy and convert cube map face data from an external image/surface into this object
//
// a_FaceIdx        = a value 0 to 5 speciying which face to copy into (one of the CP_FACE_? )
// a_Level          = mip level to copy into
// a_SrcType        = data type of image being copyed from (one of the CP_TYPE_? types)
// a_SrcNumChannels = number of channels of the image being copied from (usually 1 to 4)
// a_SrcPitch       = number of bytes per row of the source image being copied from
// a_SrcDataPtr     = pointer to the image data to copy from
// a_Degamma        = original gamma level of input image to undo by degamma
// a_Scale          = scale to apply to pixel values after degamma (in linear space)
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::SetInputFaceData(int32 a_FaceIdx, int32 a_SrcType, int32 a_SrcNumChannels, 
                                         int32 a_SrcPitch, void *a_SrcDataPtr, float32 a_MaxClamp, float32 a_Degamma, float32 a_Scale)
{
    //since input is being modified, terminate any active filtering threads
    m_InputSurface[a_FaceIdx].SetImageDataClampDegammaScale( a_SrcType, a_SrcNumChannels, a_SrcPitch, 
        a_SrcDataPtr, a_MaxClamp, a_Degamma, a_Scale );
}

//--------------------------------------------------------------------------------------
//Copy and convert cube map face data from this object into an external image/surface
//
// a_FaceIdx        = a value 0 to 5 speciying which face to copy into (one of the CP_FACE_? )
// a_Level          = mip level to copy into
// a_DstType        = data type of image to copy to (one of the CP_TYPE_? types)
// a_DstNumChannels = number of channels of the image to copy to (usually 1 to 4)
// a_DstPitch       = number of bytes per row of the dest image to copy to
// a_DstDataPtr     = pointer to the image data to copy to
// a_Scale          = scale to apply to pixel values (in linear space) before gamma for output
// a_Gamma          = gamma level to apply to pixels after scaling
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::GetInputFaceData(int32 a_FaceIdx, int32 a_DstType, int32 a_DstNumChannels, 
                                         int32 a_DstPitch, void *a_DstDataPtr, float32 a_Scale, float32 a_Gamma)
{
    m_InputSurface[a_FaceIdx].GetImageDataScaleGamma( a_DstType, a_DstNumChannels, a_DstPitch, 
        a_DstDataPtr, a_Scale, a_Gamma );
}

//--------------------------------------------------------------------------------------
//Copy and convert cube map face data out of this class into an external image/surface
//
// a_FaceIdx        = a value 0 to 5 specifying which face to copy from (one of the CP_FACE_? )
// a_Level          = mip level to copy from
// a_DstType        = data type of image to copyed into (one of the CP_TYPE_? types)
// a_DstNumChannels = number of channels of the image to copyed into  (usually 1 to 4)
// a_DstPitch       = number of bytes per row of the source image to copyed into 
// a_DstDataPtr     = pointer to the image data to copyed into 
// a_Scale          = scale to apply to pixel values (in linear space) before gamma for output
// a_Gamma          = gamma level to apply to pixels after scaling
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::GetOutputFaceData(int32 a_FaceIdx, int32 a_Level, int32 a_DstType, 
    int32 a_DstNumChannels, int32 a_DstPitch, void *a_DstDataPtr, float32 a_Scale, float32 a_Gamma )   
{
    switch(a_DstType)
    {
    case CP_VAL_UNORM8:
    case CP_VAL_UNORM8_BGRA:
    case CP_VAL_UNORM16:
    case CP_VAL_FLOAT16:
    case CP_VAL_FLOAT32:
        {
            m_OutputSurface[a_Level][a_FaceIdx].GetImageDataScaleGamma( a_DstType, a_DstNumChannels, 
                a_DstPitch, a_DstDataPtr, a_Scale, a_Gamma ); 
        }
        break;
    default:
        break;
    }
}

//--------------------------------------------------------------------------------------
//ChannelSwapInputFaceData
//  swizzle data in first 4 channels for input faces
//
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::ChannelSwapInputFaceData(int32 a_Channel0Src, int32 a_Channel1Src, 
                                                 int32 a_Channel2Src, int32 a_Channel3Src )
{
    int32 iFace, u, v, k;
    int32 size;
    CP_ITYPE texelData[4];
    int32 channelSrcArray[4];

    //since input is being modified, terminate any active filtering threads
    size = m_InputSize;

    channelSrcArray[0] = a_Channel0Src;
    channelSrcArray[1] = a_Channel1Src;
    channelSrcArray[2] = a_Channel2Src;
    channelSrcArray[3] = a_Channel3Src;

    //Iterate over faces for input images
    for(iFace=0; iFace<6; iFace++)
    {
        for(v=0; v<m_InputSize; v++ )
        {
            for(u=0; u<m_InputSize; u++ )
            {
                //get channel data
                for(k=0; k<m_NumChannels; k++)
                {
                    texelData[k] = *(m_InputSurface[iFace].GetSurfaceTexelPtr(u, v) + k);
                }

                //repack channel data accoring to swizzle information
                for(k=0; k<m_NumChannels; k++)
                {
                    *( m_InputSurface[iFace].GetSurfaceTexelPtr(u, v) + k) = 
                        texelData[ channelSrcArray[k] ];
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------
//ChannelSwapOutputFaceData
//  swizzle data in first 4 channels for input faces
//
//--------------------------------------------------------------------------------------
void CCubeMapProcessor::ChannelSwapOutputFaceData(int32 a_Channel0Src, int32 a_Channel1Src, 
                                                  int32 a_Channel2Src, int32 a_Channel3Src )
{
    int32 iFace, iMipLevel, u, v, k;
    int32 size;
    CP_ITYPE texelData[4];
    int32 channelSrcArray[4];

    //since output is being modified, terminate any active filtering threads

    size = m_OutputSize;

    channelSrcArray[0] = a_Channel0Src;
    channelSrcArray[1] = a_Channel1Src;
    channelSrcArray[2] = a_Channel2Src;
    channelSrcArray[3] = a_Channel3Src;

    //Iterate over faces for input images
    for(iMipLevel=0; iMipLevel<m_NumMipLevels; iMipLevel++ )
    {
        for(iFace=0; iFace<6; iFace++)
        {
            for(v=0; v<m_OutputSurface[iMipLevel][iFace].m_Height; v++ )
            {
                for(u=0; u<m_OutputSurface[iMipLevel][iFace].m_Width; u++ )
                {
                    //get channel data
                    for(k=0; k<m_NumChannels; k++)
                    {
                        texelData[k] = *(m_OutputSurface[iMipLevel][iFace].GetSurfaceTexelPtr(u, v) + k);
                    }

                    //repack channel data accoring to swizzle information
                    for(k=0; k<m_NumChannels; k++)
                    {
                        *(m_OutputSurface[iMipLevel][iFace].GetSurfaceTexelPtr(u, v) + k) = texelData[ channelSrcArray[k] ];
                    }
                }
            }
        }
    }
}

void CCubeMapProcessor::ResizeInput( int32 a_NewInputSize )
{
    if ( m_InputSize == a_NewInputSize )
        return;

    for ( int i = 0; i < 6; ++i )
        m_InputSurface[i].Resize( a_NewInputSize, a_NewInputSize );

    m_InputSize = a_NewInputSize;
}

void CCubeMapProcessor::GeneralInputMips()
{
    int currSize = m_InputSize;
    int currLevel = 0;

    while ( currSize > 1 )
    {
        int destSize = currSize / 2;

        CImageSurface* srcCube  = m_InputSurfaceMips[currLevel];
        CImageSurface* destCube = m_InputSurfaceMips[currLevel + 1];

        for ( int face = 0; face < 6; ++face )
        {   
            destCube[face].Init( destSize, destSize, m_NumChannels );
            srcCube[face].CopyTo( &destCube[face] );
        }

        currSize /= 2;
        ++currLevel;
    }

    m_NumInputMipLevels = currLevel + 1;
}

void CCubeMapProcessor::GetMipmapTexelVal( const float32* a_XYZ, float level, float* val )
{
    if ( m_NumInputMipLevels <= 1 )
    {
        const float* ptr = GetCubeMapTexelPtr( m_InputSurface, a_XYZ );
        VM_COPY4(val, ptr);
        return;
    }

    int lastLevel = m_NumInputMipLevels-1;

    if ( level < 0 )
        level = 0;
    else if ( level > lastLevel )
        level = (float)lastLevel;

    int   levelLow  = (int)level;
    int   levelHigh = levelLow < lastLevel ? (levelLow + 1) : levelLow;
    float levelLerp = level - levelLow;

    const float* valLow  = GetCubeMapTexelPtr( m_InputSurfaceMips[levelLow],  a_XYZ );
    const float* valHigh = GetCubeMapTexelPtr( m_InputSurfaceMips[levelHigh], a_XYZ );

    VM_LERP_N( val, valLow, valHigh, m_NumChannels, levelLerp );
}

void CCubeMapProcessor::BeginIntegrate()
{
    m_FilterSize = _CalcHemiFilterSize( m_NormCubeMap[0].m_Width );
}

void CCubeMapProcessor::Integrate( FnGetSampleValType fnGetSampleVal, void* context, 
                                  const float* centerTapDir, float* retVals, int sampleChannels )
{
    const int MAX_SAMPLE_CHANNELS = 16;
    float tmpSampleVal[MAX_SAMPLE_CHANNELS] = {0};
    double totalSampleVal[MAX_SAMPLE_CHANNELS] = {0};

    CBBoxInt32 filterExtents[6]; // already clear now

    int srcSize = m_NormCubeMap[0].m_Width;
    _DetermineFilterExtents( centerTapDir, srcSize, m_FilterSize, filterExtents );

    for ( int face = 0; face < 6; ++face )
    {
        CBBoxInt32& bound = filterExtents[face];
        if ( bound.Empty() ) // no texels calc in this face
            continue;

        int uStart = bound.m_minCoord[0];
        int uEnd = bound.m_maxCoord[0];

        int vStart = bound.m_minCoord[1];
        int vEnd = bound.m_maxCoord[1];

        CImageSurface& normTable = m_NormCubeMap[face];

        for ( int v = vStart; v <= vEnd; ++v )
        {
            for ( int u = uStart; u <= uEnd; ++u )
            {
                // get one sample
                const float32* vDir = normTable.GetSurfaceTexelPtr(u, v);
                double deltaArea = vDir[3]; // .w channel store per solid angle area

                if ( fnGetSampleVal( this, context, face, u, v, vDir, tmpSampleVal, deltaArea ) )
                {
                    for ( int i = 0; i < sampleChannels; ++i )
                        totalSampleVal[i] += tmpSampleVal[i] * deltaArea;
                }
            }
        }
    }

    for ( int i = 0; i < sampleChannels; ++i )
        retVals[i] = (float)totalSampleVal[i];
}