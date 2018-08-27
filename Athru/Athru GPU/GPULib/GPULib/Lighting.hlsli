
#include "Core3D.hlsli"
#include "LightingUtility.hlsli"
#include "MIS.hlsli"
#ifndef MATERIALS_LINKED
    #include "Materials.hlsli"
#endif
#ifndef RASTER_CAMERA_LINKED
    #include "RasterCamera.hlsli"
#endif

// Per-vertex material synthesizer
// Light materials are implict in [DirIllumRadiance]/[Emission]
// so there's no reason to generate them here
float2x4 MatGen(float3 coord,
                float3 norml,
                uint dfType,
                inout uint randVal)
{
    // Just a basic generalized planetary/stellar function now, will create
    // plant/critter variants when neccessary
    switch (dfType)
    {
        case DF_TYPE_STAR:
            return float2x4(float4(1.0f, 0.0f.xxx), // Stars are smooth bright-white diffuse surfaces
                            STELLAR_BRIGHTNESS.xxx, 0.0f);
        default:
            return float2x4(float4(1.0f, 0.0f.xxx), // Only diffuse materials supported atm, will work on others
                                                    // when possible
                            abs(sin(norml)), iToFloat(randVal) * TWO_PI);
    }
}

// Process + cache surface data for the interaction occurring at
// [rayVec]
// Also return a store-able [PathVt] so we can account for this
// interaction during bi-directional path connection/integration
// Not a huge fan of all these side-effecty [out] variables, but
// it feels like the only single-function way to process arbitrary
// camera/light sub-paths without heavy code duplication in
// [SceneVis.hlsl]'s core marching loop
PathVt ProcVert(float4 rayVec, // Ray position in [xyz], planetary distance in [w]
							  // (used for basic atmospheric shading)
                float3 figPos, // Central position for the closest figure to [rayVec]
                inout float3 rayDir,
                float adaptEps,
                uint2 figIDDFType,
                bool lightPath,
                float3 lastVtPos,
                inout float3 atten,
                inout float3 rayOri,
                float2x3 prevFres, // Fresnel value for the volume traversed by [rayVec]
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
    float2x4 mat = MatGen(rayVec.xyz,
                          normal,
                          figIDDFType.y,
                          randVal);

    // Extract a shading BXDF for illumination at the current vertex
    uint bxdfID = MatBXDFID(mat[0],
                            randVal);

    // Generate an importance-sampled bounce direction
    float3x3 normSpace = NormalSpace(normal);
    float3 bounceDir = mul(MatDir(rayDir,
                                  rayVec.xyz,
                                  randVal,
                                  float4(bxdfID,
                                         figIDDFType.yx,
                                         mat[1].w)),
                           normSpace);

    // Evaluate the current vertex's outgoing directions + PDF
    // (both needed for attenuation atm + path integration
    // later)
    float3x3 surfDirs = float3x3(bounceDir,
                                 rayDir,
                                 normal);
    float2 pdfIO = float2(MatPDF(surfDirs,
                                 float4(rayVec.xyz,
                                        lightPath),
                                 float4(bxdfID,
                                        figIDDFType.yx,
                                        true),
                                 mat[0],
                                 mat[1].w), // Probability of exiting along [bounceDir] after entering along [rayDir]
                          MatPDF(surfDirs,
                                 float4(rayVec.xyz,
                                        lightPath),
                                 float4(bxdfID,
                                        figIDDFType.yx,
                                        true),
                                 mat[0],
                                 mat[1].w)); // Probability of exiting along [rayDir] after entering along [bounceDir]
    // Update path attenuation
    atten *= MatBXDF(rayVec.xyz,
                     prevFres,
                     surfDirs,
                     float4(bxdfID,
                            figIDDFType.yx,
                            mat[1].w),
                     mat[1].rgb) * abs(dot(normal, bounceDir)) * pdfIO.x;

    // Cache the bi-directional vertex representing the current interaction
    PathVt bdVt = BuildBidirVt(float4(rayVec.xyz, figIDDFType.x),
                               figPos,
                               float4(atten, bxdfID),
                               float4(normal, figIDDFType.y),
                               float2x3(bounceDir, rayDir),
                               mat,
                               float3(pdfIO, rayVec.w));

    // Update remaining [out] parameters

    // Update light ray starting position
    // Shift the starting position just outside the surface so that rays avoid
    // immediately re-intersecting with the figure
    rayOri = rayVec.xyz;

    // Update light ray direction (if appropriate)
    rayDir = bounceDir;

    // Re-set ray distance as appropriate
    rayDist = 0.0f;

    // Return the bi-directional vertex we cached before
    return bdVt;
}

// Camera gathers for surface light vertices
// Not MIS-weighted just yet, need to cleanly integrate the lens DF into the scene field before I
// look at that...
float4 CamGather(PathVt liVt, // A surface light vertex
                 float4 camInfo, // Camera information needed for lens interactions; contains the lens normal in [xyz] and
                                 // the camera z-scale (projective depth used for perspective projection before normalization)
                                 // in [w]
                 float adaptEps, // The adaptive epsilon value chosen for the current frame
                 inout uint randVal) // Not used here, but needed as an argument for [OccTest(...)]
{
    // Only assign color to [connStratRGB] if the camera-gather is unoccluded; assume the camera's
    // importance is "shadowed" and set [connStratRGB] to [0.0f.xxx] otherwise
    // No default lens intersections rn so MIS-weighted lens gathers are impossible; tempting to
    // change that later to simplify path integration
    float3x4 occData = OccTest(float4(liVt.pos.xyz, true),
                               float4(normalize(cameraPos.xyz - liVt.pos.xyz), LENS_FIG_ID), // Placeholder figure-ID here (no default lens intersections rn)
                               float4(cameraPos.xyz, true),
                               float2(adaptEps, liVt.pos.w),
                               randVal);

    // Generate a pixel ID for the sampling ray direction, cache alongside default sample importance
    float4 gathInfo = float4(0.0f.xxx, PRayPix(occData[0].xyz,
                                               camInfo.w).z);

    // Return appropriate importance (0.0f.xxx for occluded rays, a surface/atmosphere/lens interaction
    // for clear directions)
    if (!occData[1].w)
    {
        // Attenuate the importance at [lightVt] appropriately, then store the result
        // Assume unit attenuation at the camera and 100% transmittance
        //float4 thetaPhiIO = float4(VecToAngles(occData[0].xyz),
        //                                       lightVt.ioSrs.xy);
        gathInfo.rgb = liVt.atten.rgb * // Emitted color from the source vertex on the light subpath
                       MatBXDF(liVt.pos.xyz,
                               AmbFres(liVt.pdfIO.z), // Not worrying about subsurface scattering just yet...
                               float3x3(liVt.iDir.xyz,
                                        occData[0].xyz,
                                        liVt.norml.xyz),
                               float4(liVt.atten.w,
                                      liVt.norml.w,
                                      liVt.pos.w,
                                      liVt.dfOri.w),
                               float3(liVt.iDir.w,
                                      liVt.oDir.w,
                                      liVt.pdfIO.w)); // * // Attenuation by the source interaction's local BXDF
                       //RayImportance(occData[0].xyz,
                       //              camInfo,
                       //              pixWidth) / // Attenuation by the importance emitted from the camera towards [lightVt.pos.xyz]
                       //CamAreaPDFIn(lightVt.pos.xyz - camVt.pos.xyz, // Scaling to match the probability of gather rays reaching the lens/pinhole along [occData[0].xyz]
                       //             camInfo.xyz);

        // Grains of participating media are treated like perfect re-emitters, so only
        // attenuate lighting by facing ratio for solid surfaces
        if (liVt.atten.w != BXDF_ID_VOLUM)
        {
            gathInfo.rgb *= dot(occData[0].xyz,
                                liVt.norml.xyz);
        }
    }
    else
    {
        gathInfo.rgb = 0.0f.xxx;
    }
    return gathInfo;
}

// Light gathers for surface camera vertices
float3 LiGather(PathVt camVt, // A surface camera vertex
                //float3 prevCamVtAtten, // Attenuation at the camera vertex immediately before [camVt]
                float4 liLinTransf, // Position/size for the given light (all lights are area lights in Athru)
                float adaptEps, // The adaptive epsilon value chosen for the current frame
                inout uint randVal) // The permutable random value used for the current shader instance
{
    // Initialise ray-marching values; all light sources are assumed stellar atm
    float3 rayOri = camVt.pos.xyz;
    float3 stellarSurfPos = StellarSurfPos(liLinTransf,
                                           randVal,
                                           camVt.dfOri.xyz);
    // Deploy an ordinary ray sampled from the star's distribution
    float3x4 occData = OccTest(float4(rayOri, true),
                               float4(normalize(stellarSurfPos - rayOri), STELLAR_FIG_ID),
                               float4(stellarSurfPos, false),
                               float2(adaptEps, camVt.pos.w),
                               randVal);
    // Deploy a MIS ray sampled from the surface's BXDF
    float3x3 normSpace = NormalSpace(camVt.norml.xyz);
    float3 misRayDir = mul(MatDir(camVt.oDir.xyz,
                                  rayOri,
                                  randVal,
                                  float4(camVt.atten.w,
                                         camVt.norml.w,
                                         camVt.pos.w,
                                         camVt.dfOri.w)),
                           normSpace);
    float3x4 misOccData = OccTest(float4(rayOri, true),
                                  float4(misRayDir, STELLAR_FIG_ID),
                                  float4(0.0f.xxx, false),
                                  float2(adaptEps, camVt.pos.w),
                                  randVal);
    // Cache probability densities for each sample
    float stellarPosPDF = LightPosPDF(LIGHT_ID_STELLAR);
    float3x3 surfDirs[2] = { float3x3(occData[0].xyz,
                                      camVt.oDir.xyz,
                                      camVt.norml.xyz),
                             float3x3(misOccData[0].xyz,
                                      camVt.oDir.xyz,
                                      camVt.norml.xyz) };
    float surfMatPDF = MatPDF(surfDirs[1],
                              float4(rayOri, false),
                              uint4(camVt.atten.w,
                                    camVt.norml.w,
                                    camVt.pos.w,
                                    true),
                              camVt.refl,
                              camVt.dfOri.w);
    // Accumulate MIS-weighted radiance along the stellar/BXDF rays (as appropriate)
    float3 gathRGB = 0.0f.xxx;
    if (!occData[1].w ||
        !misOccData[1].w)
    {
        // Planetary normals change too quickly for ideal gather integration, so
        // generate biased samples here instead
        if (!occData[1].w)
        {
            gathRGB += Emission(STELLAR_RGB,
                                STELLAR_BRIGHTNESS,
                                occData[0].w) *
                       abs(dot(camVt.norml.xyz, occData[0].xyz)) *
                       MatBXDF(rayOri,
                               AmbFres(0.0f), // Only planetary intersections supported atm
                               surfDirs[0],
                               float4(camVt.atten.w,
                                      camVt.norml.w,
                                      camVt.pos.w,
                                      camVt.dfOri.w),
                               float3(camVt.iDir.w,
                                      camVt.oDir.w,
                                      camVt.pdfIO.w)) *
                       MISWeight(1, stellarPosPDF,
                                 1, surfMatPDF).xxx; // Weight this sample relative to it's chances of reaching the star
                                                     // compared with the BXDF-sampled ray
                                                     // Only one sample from each of two separate distributions for light
                                                     // gathers
        }
        if (!misOccData[1].w)
        {
            gathRGB += Emission(STELLAR_RGB,
                                STELLAR_BRIGHTNESS,
                                misOccData[0].w) *
                       abs(dot(camVt.norml.xyz, misOccData[0].xyz)) *
                       MatBXDF(rayOri,
                               AmbFres(0.0f), // Only planetary intersections supported atm
                               surfDirs[1],
                               float4(camVt.atten.w,
                                      camVt.norml.w,
                                      camVt.pos.w,
                                      camVt.dfOri.w),
                               float3(camVt.iDir.w,
                                      camVt.oDir.w,
                                      camVt.pdfIO.w)) *
                       MISWeight(1, stellarPosPDF,
                                 1, surfMatPDF).xxx * 100.0f; // Weight this sample relative to it's chances of reaching the star
                                                                 // compared with the spatially-sampled ray
                                                                 // Only one sample from each of two separate distributions for light
                                                                 // gathers
        }

        // Return the MIS-weighted blend of the two light gathers
        // Also apply surface color here (should really apply the local BXDF instead...)
        return gathRGB;
    }
    else
    {
        return 0.0f.xxx;
    }
}

// Function handling connections between bi-directional vertices and
// returning path colors for the given connection strategy (defined
// by the number of samples selected for each sub-path)
float3 ConnectBidirVts(PathVt camVt,
                       PathVt lightVt,
                       float adaptEps,
                       inout uint randVal)
{
    // Evaluate visibility between [lightVt] and [camVt]
    float3x4 occData = OccTest(float4(lightVt.pos.xyz, true),
                               float4(normalize(camVt.pos.xyz - lightVt.pos.xyz), camVt.pos.w),
                               float4(camVt.pos.xyz, true),
                               adaptEps,
                               randVal);

    // Use the evaluated visibility to define the generalized visibility function [g]
    float g = !occData[1].w * // Point-to-point visibility
              AngleToArea(lightVt.pos.xyz, // Angle-to-area integral conversion function
                          camVt.pos.xyz,
                          lightVt.norml.xyz,
                          camVt.atten.w != BXDF_ID_VOLUM);

    // AngleToArea[...] implicitly handles facing ratio for incident surfaces ([camVt.bxdfID]),
    // but not emissive ones ([lightVt.bxdfID]); account for that here (if appropriate)
    if (lightVt.atten.w != BXDF_ID_VOLUM)
    {
        g *= abs(dot(camVt.norml.xyz,
                 normalize(camVt.pos.xyz - lightVt.pos.xyz)));
    }

    // We've defined the generalized visibility function [g], so we can safely move through and
    // define [connStratRGB.rgb] with the radiance emitted from [lightVt.pos.xyz] through
    // [camVt.pos.xyz]
    // Assumes 100% transmittance
    float3x3 occDirs[2] = { float3x3(occData[0].xyz,
                                     camVt.oDir.xyz,
                                     camVt.norml.xyz),
                            float3x3(occData[0].xyz,
                                     lightVt.iDir.xyz,
                                     lightVt.norml.xyz) };
    float3 camBXDF = MatBXDF(camVt.pos.xyz,
                             AmbFres(camVt.pdfIO.z), // Not worrying about subsurface scattering atm...
                             occDirs[0],
                             float4(camVt.atten.w,
                                    lightVt.norml.w,
                                    camVt.pos.w,
                                    camVt.dfOri.w),
                             float3(camVt.iDir.w,
                                    camVt.oDir.w,
                                    camVt.pdfIO.w));
    float3 lightBXDF = MatBXDF(lightVt.pos.xyz,
                               AmbFres(lightVt.pdfIO.z), // Not worrying about subsurface scattering atm...
                               occDirs[1],
                               float4(lightVt.atten.w,
                                      camVt.norml.w,
                                      lightVt.pos.w,
                                      lightVt.dfOri.w),
                               float3(lightVt.iDir.w,
                                      lightVt.oDir.w,
                                      lightVt.pdfIO.w));
    return camVt.atten.rgb * camBXDF * // Evaluate + apply attenuated radiance on the camera subpath
           lightVt.atten.rgb * lightBXDF * // Evaluate + apply attenuated radiance on the light subpath;
           g; // Scale the integrated radiance by the relative visibility of [camVt] from
              // [lightVt]'s perspective (and vice-versa, since BDPT is symmetrical)
}