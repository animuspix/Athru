
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
// Also return a store-able [BidirVert] so we can account for this
// interaction during bi-directional path connection/integration
// Not a huge fan of all these side-effecty [out] variables, but
// it feels like the only single-function way to process arbitrary
// camera/light sub-paths without heavy code duplication in
// [SceneVis.hlsl]'s core marching loop
BidirVert ProcVert(float4 rayVec, // Ray position in [xyz], planetary distance in [w]
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
    // Extract a shading BXDF for illumination at the current vertex
    uint bxdfID = MatBXDFID(MatInfo(float2x3(MAT_PROP_BXDF_FREQS,
                                             figIDDFType.yx,
                                             rayVec.xyz)).x,
                            randVal);

    // Extract surface normal
    float3 normal = tetGrad(rayVec.xyz,
                            adaptEps,
                            figuresReadable[figIDDFType.x]).xyz;

    // Generate an importance-sampled bounce direction
    float3x3 normSpace = NormalSpace(normal);
    float3 bounceDir = mul(MatDir(rayDir,
                                  rayVec.xyz,
                                  randVal,
                                  float3(bxdfID,
                                         figIDDFType.yx)),
                           normSpace);

    // Evaluate the current vertex's outgoing directions + PDF
    // (both needed for attenuation atm + path integration
    // later)
    float2x3 ioDirs = float2x3(bounceDir,
                               rayDir);
    float2 pdfIO = float2(MatPDF(ioDirs,
                                 float4(rayVec.xyz,
                                        lightPath),
                                 float4(bxdfID,
                                        figIDDFType.yx,
                                        true)), // Probability of exiting along [bounceDir] after entering along [rayDir]
                          MatPDF(float2x3(ioDirs[1],
                                          ioDirs[0]),
                                 float4(rayVec.xyz,
                                        lightPath),
                                 float4(bxdfID,
                                        figIDDFType.yx,
                                        true))); // Probability of exiting along [rayDir] after entering along [bounceDir]

    // Generate solid angles for ray integration
    float4 thetaPhiIO = RaysToAngles(bounceDir,
                                     rayDir,
                                     lightPath);
    // Update path attenuation
    atten *= MatBXDF(rayVec.xyz,
                     prevFres,
                     thetaPhiIO,
                     uint3(bxdfID, figIDDFType.yx)) * abs(dot(normal, bounceDir)) * pdfIO.x;

    // Cache the bi-directional vertex representing the current interaction
    BidirVert bdVt = BuildBidirVt(float4(rayVec.xyz, figIDDFType.x),
                                  figPos,
                                  float4(atten, bxdfID),
                                  float4(normal, figIDDFType.y),
                                  thetaPhiIO,
                                  float3(pdfIO, rayVec.w));

    // Update remaining [out] parameters

    // Update light ray direction (if appropriate)
    rayDir = bounceDir;

    // Update light ray starting position
    // Shift the starting position just outside the figure surface so that rays avoid
    // immediately re-intersecting with the surface
    // Small surface repulsion here, most bounces will tend towards the local normal
    // and avoid skimming the scene surface
    rayOri = rayVec.xyz + (normal * adaptEps * 2.0f);

    // Re-set ray distance as appropriate
    rayDist = (adaptEps * 2.0f);

    // Return the bi-directional vertex we cached before
    return bdVt;
}

// Function handling connections between bi-directional vertices and
// returning path colors for the given connection strategy (defined
// by the number of samples selected for each sub-path)
// Cases where the thread loses it's one-to-one relationship with pixels
// in the display texture (e.g. when a gather ray traces back from the
// light subpath towards the camera) are handled by converting the
// incident direction to a pixel ID and storing the result in [w]
// [gatherPDFs] stores the forward/reverse PDFs associated with
// samples taken outside either subpath (like e.g.
// camera/light gathers)
float4 ConnectBidirVts(BidirVert camVt,
                       BidirVert lightVt,
                       float4 sampleInfo, // Distance to previous samples in [zw], number of camera/light samples in [xy]
                       uint2 subpathBounces,
                       bool2 subpathEnded,
                       float adaptEps,
                       float3 eyePos,
                       float4 camInfo,
                       float4 starLinTransf, // Star position in [xyz], star radius in [w]
                       out float2 gatherPDFs,
                       out float3 gatherPos,
                       float pixWidth,
                       uint linPixID,
                       inout uint randVal)
{
    float4 connStratRGB = float4(0.0f.xxx, linPixID); // RGB spectra for the selected connection strategy
    if (sampleInfo.y == 0) // Ignore the light sub-path
    {
        if (subpathEnded.x && // Only valid if the camera sub-path reaches the local star
            (sampleInfo.x - 1) == (int)subpathBounces.x) // Only valid if the given camera vertex is the last vertex
                                                         // in the camera-subpath (since camera rays terminate at the
                                                         // the local star))
        {
            connStratRGB = float4(Emission(STELLAR_RGB, STELLAR_BRIGHTNESS, sampleInfo.z) * camVt.atten.rgb, // Cache attenuated emission
                                  linPixID); // No thread relationship changes here, store the local pixel index in [w]
        }
        else
        {
            connStratRGB = float4(0.0f.xxx,
                                  linPixID);
        }
        // No light or camera gathers here
        gatherPDFs = 0.0f.xx;
        gatherPos = 0.0f.xxx;
    }
    else if (sampleInfo.x == 0) // Ignore the camera sub-path
    {
        if (subpathEnded.y && // Only valid if the light sub-path reaches the lens/pinhole
            (sampleInfo.y - 1) == (int)subpathBounces.y) // Only valid if the given camera vertex is the last vertex in
                                                         // the light-subpath (since light rays terminate at the lens)
        {
            connStratRGB = float4(Emission(STELLAR_RGB, STELLAR_BRIGHTNESS, sampleInfo.w) * lightVt.atten.rgb, // Cache attenuated importance
                                  PRayPix(normalize(cameraPos.xyz - lightVt.pos.xyz),
                                          camInfo.w).z); // Cache the pixel ID associated with the given path
                                                         // (likely to be different from the source pixel for light paths)
        }
        else
        {
            connStratRGB = float4(0.0f.xxx,
                                  linPixID);
        }

        // No light or camera gathers here
        gatherPDFs = 0.0f.xx;
        gatherPos = 0.0f.xxx;
    }
    else if (sampleInfo.y == 1 && sampleInfo.x != 1) // Emit a gather ray towards the light from the given vertex on the camera subpath
    {
        // Initialise ray-marching values
        float3 rayOri = camVt.pos.xyz;
        float3 stellarSurfPos = StellarSurfPos(starLinTransf,
                                               randVal,
                                               camVt.dfOri.xyz);

        // Trace a shadow/gather ray between the last vertex of the camera subpath towards the
        // generated surface position on the light source + shade [rayOri] if the
        // shadow/gather ray is unoccluded (zero [connStratRGB] if [rayOri] is occluded by
        // surfaces in the scene)
        // Deploy an ordinary ray sampled from the star's distribution
        float3x3 normSpace = NormalSpace(camVt.norml.xyz);
        float3x4 occData = OccTest(float4(rayOri, true),
                                   float4(normalize(stellarSurfPos - rayOri), STELLAR_FIG_ID),
                                   float4(stellarSurfPos, false),
                                   adaptEps,
                                   randVal);
        // Deploy a MIS ray sampled from the surface's BXDF
        float3x4 misOccData = OccTest(float4(camVt.pos.xyz, true),
                                      float4(mul(MatDir(AnglesToVec(camVt.ioSrs.zw),
                                                        camVt.pos.xyz,
                                                        randVal,
                                                        uint3(camVt.atten.w,
                                                              camVt.norml.w,
                                                              camVt.pos.w)),
                                                 normSpace), STELLAR_FIG_ID),
                                      float4(cameraPos.xyz, false),
                                      adaptEps,
                                      randVal);
        // Cache the local input/output angles at [camVt.pos.xyz] for each sample
        float2x4 thetaPhiIO = float2x4(VecToAngles(occData[0].xyz),
                                       camVt.ioSrs.zw,
                                       VecToAngles(misOccData[0].xyz),
                                       camVt.ioSrs.zw);

        // Cache probability densities for each sample
        float stellarPosPDF = LightPosPDF(LIGHT_ID_STELLAR);
        float surfMatPDF = MatPDF(float2x3(misOccData[0].xyz,
                                           AnglesToVec(thetaPhiIO[1].zw)),
                                  float4(camVt.pos.xyz, false),
                                  uint4(camVt.atten.w,
                                        camVt.norml.w,
                                        camVt.pos.w,
                                        true));
        if (!occData[1].w ||
            !misOccData[1].w)
        {
            if (!occData[1].w)
            {
                // Accumulate MIS-weighted radiance along the stellar/BXDF rays
                connStratRGB.rgb += DirIllumRadiance(occData[0].xyz,
                                                     camVt.norml.xyz,
                                                     STELLAR_RGB,
                                                     occData[0].w,
                                                     STELLAR_BRIGHTNESS) // Evaluate local radiance
                                                   * MatBXDF(camVt.pos.xyz,
                                                             AmbFres(camVt.pdfIO.z), // Not worrying about subsurface scattering just yet...
                                                             thetaPhiIO[0],
                                                             uint3(camVt.atten.w,
                                                                   camVt.norml.w,
                                                                   camVt.pos.w)) // Apply the surface BRDF
                                                   * stellarPosPDF // Account for the probability of selecting
                                                                   // [stellarSurfPos] out of all possible
                                                                   // positions on the surface of the local
                                                                   // star
                                                   * MISWeight(1, stellarPosPDF,
                                                               1, surfMatPDF); // Weight this sample relative to it's chances of reaching the star
                                                                               // compared with the BXDF-sampled ray
                                                                               // Only one sample from each of two separate distributions for light
                                                                               // gathers
            }
            if (!misOccData[1].w)
            {
                connStratRGB.rgb += DirIllumRadiance(misOccData[0].xyz,
                                                     camVt.norml.xyz,
                                                     STELLAR_RGB,
                                                     misOccData[0].w,
                                                     STELLAR_BRIGHTNESS) // Evaluate local radiance
                                                   * MatBXDF(camVt.pos.xyz,
                                                             AmbFres(camVt.pdfIO.z), // Not worrying about subsurface scattering just yet...
                                                             thetaPhiIO[1],
                                                             uint3(camVt.atten.w,
                                                                   camVt.norml.w,
                                                                   camVt.pos.w)) // Apply the surface BRDF
                                                   * surfMatPDF // Account for the probability of selecting
                                                                // the given BXDF direction out of all possible
                                                                // directions for the surface
                                                   * MISWeight(1, surfMatPDF,
                                                               1, stellarPosPDF); // Weight this sample relative to it's chances of reaching the star
                                                                                  // compared with the spatially-sampled ray
                                                                                  // Only one sample from each of two separate distributions for light
                                                                                  // gathers
            }
        }
        else
        {
            connStratRGB.rgb = 0.0f.xxx;
        }

        // We integrated a sample outside the camera/light subpaths, so define [gatherPDFs] appropriately
        // Using [stellarPosPDF] for forward + reverse probabilities because the directional PDF is only
        // designed to fit the cylindrical distribution used to concentrate primary light-rays in the system
        // disc; "actual" star emission will always be spherical, so we need to use a spherical PDF
        // Should update this for MIS...
        gatherPDFs = float2(stellarPosPDF,
                            stellarPosPDF);

        // Record the gather's position here so we can accurately convert PDFs for MIS weights
        // during integration
        gatherPos = occData[1].xyz;

        // No thread relationship changes here, so store the local pixel index in [w]
        connStratRGB.w = linPixID;
    }
    else if (sampleInfo.x == 1) // Emit gather rays towards the camera from the given vertex on the light subpath
    {
        // Only assign color to [connStratRGB] if the camera-gather is unoccluded; assume the camera's
        // importance is "shadowed" and set [connStratRGB] to [0.0f.xxx] otherwise
        // No default lens intersections rn so MIS-weighted lens gathers are impossible; tempting to
        // change that later to simplify path integration
        float3x4 occData = OccTest(float4(lightVt.pos.xyz, true),
                                   float4(normalize(cameraPos.xyz - lightVt.pos.xyz), LENS_FIG_ID), // Placeholder figure-ID here (no default lens intersections rn)
                                   float4(cameraPos.xyz, true),
                                   adaptEps,
                                   randVal);
        if (!occData[1].w)
        {
            // Attenuate the importance at [lightVt] appropriately, then store the result
            // Assume unit attenuation at the camera and 100% transmittance
            float4 thetaPhiIO = float4(VecToAngles(occData[0].xyz),
                                                   lightVt.ioSrs.xy);
            connStratRGB.rgb = lightVt.atten.rgb * // Emitted color from the source vertex on the light subpath
                               MatBXDF(lightVt.pos.xyz,
                                       AmbFres(lightVt.pdfIO.z), // Not worrying about subsurface scattering just yet...
                                       thetaPhiIO,
                                       uint3(lightVt.atten.w,
                                             lightVt.norml.w,
                                             lightVt.pos.w)); // * // Attenuation by the source interaction's local BXDF
                               RayImportance(occData[0].xyz,
                                             camInfo,
                                             pixWidth) / // Attenuation by the importance emitted from the camera towards [lightVt.pos.xyz]
                               CamAreaPDFIn(lightVt.pos.xyz - camVt.pos.xyz, // Scaling to match the probability of gather rays reaching the lens/pinhole along [occData[0].xyz]
                                            camInfo.xyz);

            // Grains of participating media are treated like perfect re-emitters, so only
            // attenuate lighting by facing ratio for solid surfaces
            if (lightVt.atten.w != BXDF_ID_VOLUM)
            {
                connStratRGB *= dot(occData[0].xyz,
                                    lightVt.norml.xyz);
            }
        }
        else
        {
            connStratRGB.rgb = 0.0f.xxx;
        }

        // We integrated a sample outside the camera/light subpaths, so define [gatherPDFs] appropriately
        gatherPDFs = float2(CamAreaPDFIn(lightVt.pos.xyz - camVt.pos.xyz,
                                         camInfo.xyz),
                            CamAreaPDFOut());

        // Record the gather's position here so we can accurately convert PDFs for MIS weights
        // during integration
        gatherPos = occData[1].xyz;

        // Encode shading pixel location in [w]
        connStratRGB.w = PRayPix(normalize(occData[0].xyz),
                                 camInfo.w).z;
    }
    else // Handle generic bi-directional connection strategies
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
        float2 occVecToSr = VecToAngles(occData[0].xyz);
        float2x4 camLightIO = float2x4(float4(occVecToSr,
                                              camVt.ioSrs.zw),
                                       float4(occVecToSr,
                                              lightVt.ioSrs.zw));
        float3 camBXDF = MatBXDF(camVt.pos.xyz,
                                 AmbFres(camVt.pdfIO.z), // Not worrying about subsurface scattering atm...
                                 camLightIO[0],
                                 uint3(camVt.atten.w,
                                       lightVt.norml.w,
                                       camVt.pos.w));
        float3 lightBXDF = MatBXDF(lightVt.pos.xyz,
                                   AmbFres(lightVt.pdfIO.z), // Not worrying about subsurface scattering atm...
                                   camLightIO[1],
                                   uint3(lightVt.atten.w,
                                         camVt.norml.w,
                                         lightVt.pos.w));
        connStratRGB.rgb = camVt.atten.rgb * camBXDF * // Evaluate + apply attenuated radiance on the camera subpath
                           lightVt.atten.rgb * lightBXDF * // Evaluate + apply attenuated radiance on the light subpath;
                           g; // Scale the integrated radiance by the relative visibility of [camVt] from
                              // [lightVt]'s perspective (and vice-versa, since BDPT is symmetrical)
        // No thread relationship changes here, so store the local pixel index in [w]
        connStratRGB.w = linPixID;

        // No light or camera gathers here
        gatherPDFs = 0.0f.xx;
        gatherPos = 0.0f.xxx;
    }

    // Return the evaluated radiance/importance value for the given vertices
    return connStratRGB;
}