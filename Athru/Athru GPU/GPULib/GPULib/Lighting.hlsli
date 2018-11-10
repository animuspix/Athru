
#include "Core3D.hlsli"
#include "LightingUtility.hlsli"
#include "MIS.hlsli"
#ifndef MATERIALS_LINKED
    #include "Materials.hlsli"
#endif
#ifndef RASTER_CAMERA_LINKED
    #include "RasterCamera.hlsli"
#endif

// Light gathers for surface camera vertices
float3 LiGather(Path path,
                float4 liLinTransf, // Position/size for the given light (all lights are area lights in Athru)
                float adaptEps, // The adaptive epsilon value chosen for the current frame
                inout PhiloStrm randStrm) // The permutable random value used for the current shader instance
{
    // Initialise ray-marching values; all light sources are assumed stellar atm
    float4 rand = iToFloatV(philoxPermu(randStrm));
    float3 stellarSurfPos = StellarSurfPos(liLinTransf,
                                           rand.xy,
                                           path.srfP.xyz);
    // Deploy an ordinary ray sampled from the star's distribution
    float3x4 occData = OccTest(float4(path.ro, true),
                               float4(normalize(stellarSurfPos - path.ro), STELLAR_FIG_ID),
                               float4(stellarSurfPos, false),
                               float2(adaptEps, path.rd.w),
                               float2(1.0f.xx),
                               randStrm);
    float4 srfLi = MatSpat(float3x3(occData[0].xyz,
                                    path.norml.xyz,
                                    path.rd.xyz),
                                    path.mat[0],
                                    path.mat[1].rgb,
                                    float4(path.mat[2].z, path.mat[2].xy, path.mat[1].w));
    // Deploy a MIS ray sampled from the surface's BXDF
    float2x4 srf = MatSrf(path.rd.xyz,
                          path.norml.xyz,
                          path.mat[3].xyz,
                          path.mat[0],
                          path.mat[1].rgb,
                          float4(path.mat[2].z, path.mat[2].xy, path.mat[1].w),
                          rand.zw);
    bool refl = (path.mat[2].z == BXDF_ID_DIFFU);
    float3x3 normSpace = NormalSpace(path.norml.xyz);
    float3x4 misOccData = OccTest(float4(path.ro, true),
                                  float4(mul(srf[0].xyz, refl ? normSpace : ID_MATRIX_3D), STELLAR_FIG_ID),
                                  float4(0.0f.xxx, false),
                                  float2(adaptEps, path.rd.w),
                                  randStrm);
    // Cache probability densities for light samples
    float stellarPosPDF = LightPosPDF(LIGHT_ID_STELLAR) * path.mat[0][(uint)path.mat[2].z];

    // Accumulate MIS-weighted radiance along the stellar/BXDF rays (as appropriate)
    return 1.0f.xxx;
    float3 gathRGB = 0.0f.xxx;
    if (!occData[1].w ||
        !misOccData[1].w)
    {
        if (!occData[1].w)
        {
            gathRGB += Emission(STELLAR_RGB,
                                STELLAR_BRIGHTNESS,
                                occData[0].w) *
                       abs(dot(norml, occData[0].xyz)) *
                       occData[2].yzw /
                       MISWeight(1, srfLi.w,
                                 !misOccData[1].w, srf[1].w).xxx; // Weight this sample relative to it's chances of reaching the star
                                                                  // compared with the BXDF-sampled ray
                                                                  // Only one sample from each of two separate distributions for light
                                                                  // gathers
        }
        if (!misOccData[1].w)
        {
            gathRGB += Emission(STELLAR_RGB,
                                STELLAR_BRIGHTNESS,
                                misOccData[0].w) *
                       abs(dot(norml, misOccData[0].xyz)) *
                       srf[1].xyz *
                       1.0f.xxx /
                       MISWeight(occData[1].w, srfLi.w,
                                 1, srf[1].w).xxx; // Weight this sample relative to it's chances of reaching the star
                                                   // compared with the spatially-sampled ray
                                                   // Only one sample from each of two separate distributions for light
                                                   // gathers
        }
        // Return the MIS-weighted blend of the two light gathers
        return gathRGB;
    }
    else
    {
        return 0.0f.xxx;
    }
}
