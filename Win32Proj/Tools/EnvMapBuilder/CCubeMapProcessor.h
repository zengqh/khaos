//--------------------------------------------------------------------------------------
//CCubeMapProcessor
// Class for filtering and processing cubemaps
//
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef CCUBEMAPPROCESSOR_H
#define CCUBEMAPPROCESSOR_H

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>

#include "Types.h"
#include "VectorMacros.h"
#include "CBBoxInt32.h"
#include "CImageSurface.h"

//has routines for saving .rgbe files
#define CG_RGBE_SUPPORT


#ifdef CG_HDR_FILE_SUPPORT
#include "HDRWrite.h"
#endif //CG_HDR_FILE_SUPPORT


#ifndef WCHAR 
#define WCHAR wchar_t
#endif //WCHAR 


//used to index cube faces
#define CP_FACE_X_POS 0
#define CP_FACE_X_NEG 1
#define CP_FACE_Y_POS 2
#define CP_FACE_Y_NEG 3
#define CP_FACE_Z_POS 4
#define CP_FACE_Z_NEG 5


//used to index image edges
// NOTE.. the actual number corresponding to the edge is important
//  do not change these, or the code will break
//
// CP_EDGE_LEFT   is u = 0
// CP_EDGE_RIGHT  is u = width-1
// CP_EDGE_TOP    is v = 0
// CP_EDGE_BOTTOM is v = height-1
#define CP_EDGE_LEFT   0
#define CP_EDGE_RIGHT  1
#define CP_EDGE_TOP    2
#define CP_EDGE_BOTTOM 3

//corners of CUBE map (P or N specifys if it corresponds to the 
//  positive or negative direction each of X, Y, and Z
#define CP_CORNER_NNN  0
#define CP_CORNER_NNP  1
#define CP_CORNER_NPN  2
#define CP_CORNER_NPP  3
#define CP_CORNER_PNN  4
#define CP_CORNER_PNP  5
#define CP_CORNER_PPN  6
#define CP_CORNER_PPP  7

//data types processed by cube map processor
// note that UNORM data types use the full range 
// of the unsigned integer to represent the range [0, 1] inclusive
// the float16 datatype is stored as D3Ds S10E5 representation
#define CP_VAL_UNORM8      0
#define CP_VAL_UNORM8_BGRA 1
#define CP_VAL_UNORM16    10
#define CP_VAL_FLOAT16    20 
#define CP_VAL_FLOAT32    30


//return codes for thread execution
// warning STILL_ACTIVE maps to 259, so the number 259 is reserved in this case
// and should only be used for STILL_ACTIVE
#define CP_THREAD_COMPLETED       0
#define CP_THREAD_TERMINATED     15
#define CP_THREAD_STILL_ACTIVE   STILL_ACTIVE

#define CP_MAX_PROGRESS_STRING 4096

// Type of data used internally by cube map processor
//  just in case for any reason more preecision is needed, 
//  this type can be changed down the road
#define CP_ITYPE float32

// Filter type 
#define CP_FILTER_TYPE_DISC             0
#define CP_FILTER_TYPE_CONE             1
#define CP_FILTER_TYPE_COSINE           2
#define CP_FILTER_TYPE_ANGULAR_GAUSSIAN 3


// Edge fixup type (how to perform smoothing near edge region)
#define CP_FIXUP_NONE            0
#define CP_FIXUP_PULL_LINEAR     1
#define CP_FIXUP_PULL_HERMITE    2
#define CP_FIXUP_AVERAGE_LINEAR  3
#define CP_FIXUP_AVERAGE_HERMITE 4
#define CP_FIXUP_BENT			 5
#define CP_FIXUP_WARP			 6
#define CP_FIXUP_STRETCH		 7

// Max potential cubemap size is limited to 65k (2^16 texels) on a side
#define CP_MAX_MIPLEVELS 16


#define CP_SAFE_DELETE(p)       { if(p) { delete (p);   (p)=NULL; } }
#define CP_SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p)=NULL; } }


//information about cube maps neighboring face after traversing
// across an edge
struct CPCubeMapNeighbor
{
    uint8 m_Face;    //index of neighboring face
    uint8 m_Edge;    //edge in neighboring face that abuts this face
};


//////////////////////////////////////////////////////////////////////////
// useful helper functions
void RowColToTexelCoord( int x, int y, float* u, float* v );
//void TexelCoordToRowCol( float u, float v, int* x1, int* y1, float* ax, float* ay );
void TexelCoordToRowCol( float u, float v, int* x, int* y );

void TexelCoordToVector( int32 a_FaceIdx, float32 a_U, float32 a_V, int32 a_Size, float32 *a_XYZ, int32 a_FixupType );
void VectorToTexelCoord( const float32 *a_XYZ, int32 a_Size, int32 *a_FaceIdx, float32 *a_U, float32 *a_V );

//void GetCubeMapTexelPtr_Linear( CImageSurface* a_Surface, const float32* a_XYZ, float32* a_Ret );
//void GetCubeMapTexelPtr_Nearest( CImageSurface* a_Surface, const float32* a_XYZ, float32* a_Ret );
float32* GetCubeMapTexelPtr( CImageSurface* a_Surface, const float32* a_XYZ );

void FixupCubeEdges( CImageSurface *a_CubeMap, int32 a_FixupType, int32 a_FixupWidth );

//--------------------------------------------------------------------------------------------------
//Class to filter, perform edge fixup, and build a mip chain for a cubemap
//--------------------------------------------------------------------------------------------------
class CCubeMapProcessor
{
public:
    int32 m_InputSize;                     //input cubemap size  (e.g. face width and height of topmost mip level)
    int32 m_OutputSize;                    //output cubemap size (e.g. face width and height of topmost mip level)
    int32 m_NumInputMipLevels;             //number of input mip levels
    int32 m_NumMipLevels;                  //number of output mip levels
    int32 m_NumChannels;                   //number of channels in cube map processor

    CImageSurface m_InputSurface[6];       //input faces for topmost mip level
    CImageSurface m_NormCubeMap[6];        //normalizer cube map and solid angle lookup table

    CImageSurface  m_InputSurfaceMipsBuffer[CP_MAX_MIPLEVELS-1][6]; // input face mipmap
    CImageSurface* m_InputSurfaceMips[CP_MAX_MIPLEVELS];

    CImageSurface m_OutputSurface[CP_MAX_MIPLEVELS][6];   //output faces for all mip levels
    
private:
    int32 m_FilterSize;

public:
    CCubeMapProcessor();
    ~CCubeMapProcessor();

    void Init(int32 a_InputSize, int32 a_OutputSize, int32 a_MaxNumMipLevels, int32 a_NumChannels);
    void Clear();

    void BuildNormalizerSolidAngleCubemap( int32 a_Size, int32 a_FixupType );

    void SetInputFaceData(int32 a_FaceIdx, int32 a_SrcType, int32 a_SrcNumChannels, 
        int32 a_SrcPitch, void *a_SrcDataPtr, float32 a_MaxClamp, float32 a_Degamma, float32 a_Scale);
    void GetInputFaceData(int32 a_FaceIdx, int32 a_DstType, int32 a_DstNumChannels, 
        int32 a_DstPitch, void *a_DstDataPtr, float32 a_Scale, float32 a_Gamma);
    void GetOutputFaceData(int32 a_FaceIdx, int32 a_Level, int32 a_DstType, 
        int32 a_DstNumChannels, int32 a_DstPitch, void *a_DstDataPtr, float32 a_Scale, float32 a_Gamma );

    void ChannelSwapInputFaceData(int32 a_Channel0Src, int32 a_Channel1Src, 
        int32 a_Channel2Src, int32 a_Channel3Src );
    void ChannelSwapOutputFaceData(int32 a_Channel0Src, int32 a_Channel1Src, 
        int32 a_Channel2Src, int32 a_Channel3Src );

    void ResizeInput( int32 a_NewInputSize );

    void GeneralInputMips();

    void GetMipmapTexelVal( const float32* a_XYZ, float level, float* val );

    typedef bool FnGetSampleValType( CCubeMapProcessor* proc, void* context, int face, int u, int v, 
        const float* vDir, float* tmpSampleVal, double& deltaArea );

    void BeginIntegrate();

    void Integrate( FnGetSampleValType fnGetSampleVal, void* context,
        const float* centerTapDir, float* retVals, int sampleChannels );
};


#endif //CCUBEMAPFILTER_H
