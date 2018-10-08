
#include "Core3D.hlsli"
#include "LightingUtility.hlsli"
#include "MIS.hlsli"
#ifndef MATERIALS_LINKED
    #include "Materials.hlsli"
#endif
#ifndef RASTER_CAMERA_LINKED
    #include "RasterCamera.hlsli"
#endif

// Process + cache surface data for the interaction occurring at
// [rayVec]
// Also return a store-able [PathVt] so we can account for this
// interaction during bi-directional path connection/integration
// Not a huge fan of all these side-effecty [out] variables, but
// it feels like the only single-function way to process arbitrary
// camera/light sub-paths without heavy code duplication in
// [SceneVis.hlsl]'s core marching loop
float3 ProcVert(float4 rayVec, // Ray position in [xyz], planetary distance in [w]
							   // (used for basic atmospheric shading)
                float3 figPos, // Central position for the closest figure to [rayVec]
                inout float3 rayDir,
                out float3 pathNorml,
                float adaptEps,
                uint2 figIDDFType,
                inout float3 rayOri,
                float2 prevFres, // Fresnel values for the volume traversed by [rayVec]
                out float4x4 pathMat,
                out float rayDist,
                inout uint randVal)
{
    // Shift [rayVec] outside the local figure
    rayVec.xyz += ((rayDir * -1.0f) * adaptEps);

    // Extract surface normal
    Figure nearFig = figures[figIDDFType.x];
    float3 normal = PlanetGrad(PtToPlanet(rayVec.xyz,
                                          nearFig.linTransf.w),
                               nearFig);

    // Cache the surface material at [coord]
    float4x4 mat = MatGen(rayVec.xyz,
                          rayDir,
                          normal,
                          prevFres,
                          figIDDFType.y,
                          nearFig.linTransf.xyz,
                          nearFig.linTransf.w,
                          randVal);

    // Sample the local surface
    float3x3 normSpace = NormalSpace(normal);
    float2x4 srf = MatSrf(rayDir,
                          normal,
                          mat[3].xyz,
                          mat[0],
                          mat[1].xyz,
                          float4(mat[2].z,
                                 mat[2].xy,
                                 mat[1].w),
                          randVal);
    bool refl = (mat[2].z == BXDF_ID_DIFFU);
    srf[0].xyz = mul(srf[0].xyz,
                     refl ? normSpace : ID_MATRIX);

    // Apply PDF appropriately, also apply generic bounce attenuation
    srf[1].rgb /= ZERO_PDF_REMAP(srf[1].a) / dot(normal, srf[0].xyz);

    // March through the surface (if appropriate)
    if (mat[2].z != BXDF_ID_DIFFU &&
        mat[2].z != BXDF_ID_MIRRO)
    {
        // Pass through the surface, cache the exiting position + normal
        float2x3 tr = SubsurfMarch(srf[0].xyz,
                                   rayVec.xyz + (rayDir * adaptEps * 2.0f), // Make sure transmitting rays start inside transmittant surfaces
                                   figIDDFType.x,
                                   adaptEps,
                                   randVal);

        // Sample the surface at the exit position
        float2x4 oSrf = MatSrf(srf[0].xyz,
                               tr[1],
                               mat[3].xyz,
                               mat[0],
                               mat[2].xyz,
                               float4(mat[2].z,
                                      mat[2].xy,
                                      mat[1].w),
                               randVal);
        // Update ray origin
        rayVec.xyz = tr[0] + oSrf[0].xyz * adaptEps * 2.0f;

        // Update ray direction
        rayDir = oSrf[0].xyz;

        // Update ray attenuation
        srf[1].rgb *= (oSrf[1].rgb / ZERO_PDF_REMAP(oSrf[1].a) * dot(tr[1], oSrf[0].xyz));
    }

    // Update the local normal + BXDF-ID
    pathNorml = normal;

    // Update path material
    pathMat = mat;

    // Update light ray starting position
    rayOri = rayVec.xyz;

    // Update light ray direction
    rayDir = srf[0].xyz;

    // Re-set ray distance as appropriate
    rayDist = 0.0f;

    // Return attenuation for the current intersection
    return srf[1].rgb;
}

// Light gathers for surface camera vertices
float3 LiGather(Path path,
                float4 liLinTransf, // Position/size for the given light (all lights are area lights in Athru)
                float adaptEps, // The adaptive epsilon value chosen for the current frame
                inout uint randVal) // The permutable random value used for the current shader instance
{
    // Initialise ray-marching values; all light sources are assumed stellar atm
    float3 stellarSurfPos = StellarSurfPos(liLinTransf,
                                           randVal,
                                           path.srfP.xyz);
    // Deploy an ordinary ray sampled from the star's distribution
    float3x4 occData = OccTest(float4(path.ro, true),
                               float4(normalize(stellarSurfPos - path.ro), STELLAR_FIG_ID),
                               float4(stellarSurfPos, false),
                               float2(adaptEps, path.rd.w),
                               float2(1.0f.xx),
                               randVal);
    float4 srfLi = MatSpat(float3x3(occData[0].xyz,
                                            path.norml.xyz,
                                            path.rd.xyz),
                                   path.mat[0],
                                   path.mat[1].rgb,
                                   float4(path.mat[2].z, path.mat[2].xy, path.mat[1].w),
                                   randVal);
    // Deploy a MIS ray sampled from the surface's BXDF
    float2x4 srf = MatSrf(path.rd.xyz,
                          path.norml.xyz,
                          path.mat[3].xyz,
                          path.mat[0],
                          path.mat[1].rgb,
                          float4(path.mat[2].z, path.mat[2].xy, path.mat[1].w),
                          randVal);
    bool refl = (path.mat[2].z == BXDF_ID_DIFFU);
    float3x3 normSpace = NormalSpace(path.norml.xyz);
    float3x4 misOccData = OccTest(float4(path.ro, true),
                                  float4(mul(srf[0].xyz, refl ? normSpace : ID_MATRIX), STELLAR_FIG_ID),
                                  float4(0.0f.xxx, false),
                                  float2(adaptEps, path.rd.w),
                                  path.ambFrs,
                                  randVal);
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
                       abs(dot(path.norml.xyz, occData[0].xyz)) *
                       srfLi.xyz *
                       path.attens[1] *
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
                       abs(dot(path.norml.xyz, misOccData[0].xyz)) *
                       srf[1].xyz *
                       path.attens[1] *
                       misOccData[2].yzw /
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
