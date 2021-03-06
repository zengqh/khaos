#pragma once
#include "KhaosPreHeaders.h"
#include "KhaosNameDef.h"

namespace Khaos
{
#define KHAOS_DEF_STR_NAME_(x, v) const String x(v)

    KHAOS_DEF_STR_NAME_(STRING_EMPTY, "");

    // shader uniform
    KHAOS_DEF_STR_NAME_(NAME_SURFACE_PARAMS, "surfaceInfo");
    KHAOS_DEF_STR_NAME_(NAME_SPECULARMASK_PARAMS, "maskSpecular");
    KHAOS_DEF_STR_NAME_(NAME_MTR_ALPHAREF, "mtrAlphaRef");

    //KHAOS_DEF_STR_NAME_(NAME_MTR_DIFFUSEA, "mtrDiffuseA");
    //KHAOS_DEF_STR_NAME_(NAME_MTR_SPECULAR, "mtrSpecular");
    //KHAOS_DEF_STR_NAME_(NAME_MTR_EMISSIVE, "mtrEmissive");
    
    KHAOS_DEF_STR_NAME_(NAME_MAP_BASE, "mapBase");
    KHAOS_DEF_STR_NAME_(NAME_MAP_SPECULAR, "mapSpecular");
    KHAOS_DEF_STR_NAME_(NAME_MAP_EMISSIVE, "mapEmiss");
    KHAOS_DEF_STR_NAME_(NAME_MAP_BUMP, "mapBump");
    KHAOS_DEF_STR_NAME_(NAME_MAP_OPACITY, "mapOpacity");
    KHAOS_DEF_STR_NAME_(NAME_MAP_BAKEDAO, "mapBakedAO");

    //KHAOS_DEF_STR_NAME_(NAME_MAP_DIFFUSE, "mapDiffuse");


    KHAOS_DEF_STR_NAME_(NAME_MAP_LIGHT, "mapLight");
    KHAOS_DEF_STR_NAME_(NAME_MAP_LIGHTB, "mapLightB");
    KHAOS_DEF_STR_NAME_(NAME_MAP_VOLR, "mapVolR");
    KHAOS_DEF_STR_NAME_(NAME_MAP_VOLG, "mapVolG");
    KHAOS_DEF_STR_NAME_(NAME_MAP_VOLB, "mapVolB");

    KHAOS_DEF_STR_NAME_(NAME_MAP_ENVDIFF, "mapEnvDiff");
    KHAOS_DEF_STR_NAME_(NAME_MAP_ENVSPEC, "mapEnvSpec");
    KHAOS_DEF_STR_NAME_(NAME_ENVMAPINFOA, "envMapInfoA");

    KHAOS_DEF_STR_NAME_(NAME_AMBPARAS, "ambParas");

    KHAOS_DEF_STR_NAME_(NAME_DIRLITS, "dirLits");
    KHAOS_DEF_STR_NAME_(NAME_POINTLITS, "pointLits");
    KHAOS_DEF_STR_NAME_(NAME_SPOTLITS, "spotLits");
    KHAOS_DEF_STR_NAME_(NAME_DIRLITS_COUNT, "dirLitsCount");
    KHAOS_DEF_STR_NAME_(NAME_POINTLITS_COUNT, "pointLitsCount");
    KHAOS_DEF_STR_NAME_(NAME_SPOTLITS_COUNT, "spotLitsCount");

    KHAOS_DEF_STR_NAME_(NAME_MATSHADOWPROJ, "matShadowProj");
    KHAOS_DEF_STR_NAME_(NAME_SHADOWPARAMS, "shadParams");
    KHAOS_DEF_STR_NAME_(NAME_MAPSHADOW0, "mapShadow0");
    KHAOS_DEF_STR_NAME_(NAME_MAPSHADOW1, "mapShadow1");
    KHAOS_DEF_STR_NAME_(NAME_MAPSHADOW2, "mapShadow2");
    KHAOS_DEF_STR_NAME_(NAME_MAPSHADOW3, "mapShadow3");

    KHAOS_DEF_STR_NAME_(NAME_MATWORLD, "matWorld");
    KHAOS_DEF_STR_NAME_(NAME_MATWORLDVIEW, "matWorldView");
    KHAOS_DEF_STR_NAME_(NAME_MATWVP, "matWVP");
    KHAOS_DEF_STR_NAME_(NAME_MATPROJINV, "matProjectInv");
    KHAOS_DEF_STR_NAME_(NAME_MATVIEWPROJINV, "matViewProjectInv");
    KHAOS_DEF_STR_NAME_(NAME_MATVIEWTOTEX, "matViewToTex");
    KHAOS_DEF_STR_NAME_(NAME_MATWORLDTOVOL, "matW2V");

    KHAOS_DEF_STR_NAME_(NAME_CAMERAPOSWORLD, "cameraPosWorld");
    KHAOS_DEF_STR_NAME_(NAME_CAMERAZFAR, "zFar");
    KHAOS_DEF_STR_NAME_(NAME_CAMERAZFARFOV, "zFarFov");
    KHAOS_DEF_STR_NAME_(NAME_CAMERAINFO, "camInfo");
    KHAOS_DEF_STR_NAME_(NAME_TARGETINFO, "invTargetSize");
    KHAOS_DEF_STR_NAME_(NAME_TARGETINFOEX, "targetInfo");
    KHAOS_DEF_STR_NAME_(NAME_SCREENSCALE, "screenScale");

    KHAOS_DEF_STR_NAME_(NAME_POISSON_DISK, "poissonDisk");
    KHAOS_DEF_STR_NAME_(NAME_RANDOM_REPEAT, "randomRepeat");
    KHAOS_DEF_STR_NAME_(NAME_MAP_RANDOM, "mapRandom");
    KHAOS_DEF_STR_NAME_(NAME_MAP_NORMALFITTING, "mapNormalFitting");
    KHAOS_DEF_STR_NAME_(NAME_MAP_ENVLUT, "mapEnvLUT");

    KHAOS_DEF_STR_NAME_(NAME_GBUF_DIFFUSE, "gbufDiffuse");
    KHAOS_DEF_STR_NAME_(NAME_GBUF_SPECULAR, "gbufSpecular");
    KHAOS_DEF_STR_NAME_(NAME_MAP_DEPTH, "mapDepth");
    KHAOS_DEF_STR_NAME_(NAME_MAP_DEPTHHALF, "mapDepthHalf");
    KHAOS_DEF_STR_NAME_(NAME_MAP_DEPTHQUARTER, "mapDepthQuarter");
    KHAOS_DEF_STR_NAME_(NAME_MAP_NORMAL, "mapNormal");
    KHAOS_DEF_STR_NAME_(NAME_MAP_AO, "mapAO");
    KHAOS_DEF_STR_NAME_(NAME_MAP_INPUT, "mapInput");

    KHAOS_DEF_STR_NAME_(NAME_SSAO_PARAMS, "ssaoParams");
    KHAOS_DEF_STR_NAME_(NAME_SSAO_SAMPLE_KERNELS, "arrKernel");
    KHAOS_DEF_STR_NAME_(NAME_FILTER_SAMPLE_OFFSETS, "filterSampleOffsets");
    KHAOS_DEF_STR_NAME_(NAME_FILTER_INFO, "photometricExponent");

    KHAOS_DEF_STR_NAME_(NAME_GAMMA_VALUE, "gammaValue");

    KHAOS_DEF_STR_NAME_(NAME_CALCLUM_PARAMS, "offsetParams");

    KHAOS_DEF_STR_NAME_(NAME_DOWNSCALEZ_PARAMA, "texToTexParams0");
    KHAOS_DEF_STR_NAME_(NAME_DOWNSCALEZ_PARAMB, "texToTexParams1");

    KHAOS_DEF_STR_NAME_(NAME_COMM_PARAMA, "cParams0");
    KHAOS_DEF_STR_NAME_(NAME_COMM_PARAMB, "cParams1");
    KHAOS_DEF_STR_NAME_(NAME_COMM_PARAMC, "cParams2");

    KHAOS_DEF_STR_NAME_(NAME_MAP_LUM0, "mapLum0");
    KHAOS_DEF_STR_NAME_(NAME_MAP_LUM1, "mapLum1");
    KHAOS_DEF_STR_NAME_(NAME_MAP_TARGETFULL, "mapTargetFull");
    KHAOS_DEF_STR_NAME_(NAME_MAP_TARGETQUARTER, "mapTargetQuarter");
    KHAOS_DEF_STR_NAME_(NAME_MAP_TARGETEIGHTH, "mapTargetEighth");
    KHAOS_DEF_STR_NAME_(NAME_MAP_TARGETSIXTEENTH, "mapTargetSixteenth");

    KHAOS_DEF_STR_NAME_(NAME_HDRParams0, "HDRParams0");
    KHAOS_DEF_STR_NAME_(NAME_HDRParams1, "HDRParams1");
    KHAOS_DEF_STR_NAME_(NAME_HDRParams5, "HDRParams5");
    KHAOS_DEF_STR_NAME_(NAME_HDRParams8, "HDRParams8");

    KHAOS_DEF_STR_NAME_(NAME_MAP_AREA, "mapArea");
    KHAOS_DEF_STR_NAME_(NAME_MAP_SEARCH, "mapSearch");
    KHAOS_DEF_STR_NAME_(NAME_MAP_COLOR, "mapColor");
    KHAOS_DEF_STR_NAME_(NAME_MAP_BLEND, "mapBlend");

    KHAOS_DEF_STR_NAME_(NAME_MAP_FRAMECURR, "mapFrameCurr");
    KHAOS_DEF_STR_NAME_(NAME_MAP_FRAMEPRE, "mapFramePre");
    KHAOS_DEF_STR_NAME_(NAME_MATVIEWPROJPRE, "matViewProjPrev");

    KHAOS_DEF_STR_NAME_(NAME_JITTER_WORLD, "jitterWorld");
    KHAOS_DEF_STR_NAME_(NAME_JITTER_UV, "jitterUV");

    KHAOS_DEF_STR_NAME_(NAME_CAMVOLBIAS, "camVolBias");

    KHAOS_DEF_STR_NAME_(NAME_COMMSCALE_PARAMS, "commScaleParams");
    KHAOS_DEF_STR_NAME_(NAME_COMMBLUR_TEXOFFSETS, "texOffsets");
    KHAOS_DEF_STR_NAME_(NAME_COMMBLUR_PSWEIGHTS, "psWeights");
}

