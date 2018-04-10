
#include "Core3D.hlsli"
#include "LightingUtility.hlsli"
#ifndef RASTER_CAMERA_LINKED
    #include "RasterCamera.hlsli"
#endif

// Maximum number of steps to take during ray-marching for area lighting
#define MAX_AREA_GATHER_STEPS 256

// Generalized function for calculating direct illumination at a point
float3 PerPointDirectIllum(float3 samplePoint,
                           float3 sampleDir,
                           FigMat mat,
                           uint bxdfID,
                           uint figID,
                           float3 localNorm,
                           float adaptEps,
                           bool camPath)
{
    // Initialise lighting properties
    //Figure star = figuresReadable[STELLAR_NDX];
    //float3 lightPos = star.pos.xyz;
    //float3 litRGB = 0.0f.xxx;
    //
    //// Evaluate the angular size of the area on the star emitting rays towards the
    //// current planet (allows us to importance-sample direct gathers instead of picking
    //// fully-random points on the local star)
    //// Also evaluate the remaining angles between the boundaries of that area and the origins
    //// of the spherical axes [theta, phi]
    //// All system bodies lie in the y = 0 plane, so we can safely take those offsets as the difference
    //// between half of the emissive angle (i.e. the part of the angle lying in the first quadrant) and
    //// the angle within each quadrant (known ahead of time as 2pi/4, since there are four equal-size
    //// quadrants and [2pi] radians in a circle)
    //float cosMaxEmitAngles = atan(figuresReadable[figID].scaleFactor.x / length(lightPos - figuresReadable[figID].pos.xyz)) * 2.0f;
    //float angleOffsets = (TWO_PI / 4) - (cosMaxEmitAngles / 2);
    //
    //// Evaluate the probability of selecting an arbitrary sampling direction, given the value defined for
    //// [cosMaxEmitAngles] above (assuming light sampling)
    //float lightPDF = StellarPosPDF(cosMaxEmitAngles)(cosMaxEmitAngles);
    //
    //// March towards the local star
    //uint matTraceCounter = 1; // Counter for BSDF-sampled gather rays
    //uint lightTraceCounter = 1; // Counter for light-sampled gather rays
    //for (uint i = 0; i < numDirGaths.x; i += 1)
    //{
    //    // Generate a tracing direction by taking the normalized difference between the position of the star-sample
    //    // and the planet's sampling point
    //    float3 seedNorm = abs(localNorm * 100.0f);
    //    float3 traceDir = StellarDir(StellarSurfPos(cosMaxEmitAngles,
    //                                                angleOffsets,
    //                                                star.scaleFactor.x,
    //                                                lightPos,
    //                                                float2(frand1D(((samplePoint.x + abs(samplePoint.y)) +
    //                                                                (seedNorm.x + seedNorm.y)) +
    //                                                                (matTraceCounter + lightTraceCounter)),
    //                                                       frand1D((samplePoint.z + seedNorm.z) * i))),
    //                                 samplePoint);
    //
    //    // Generate another tracing direction by importance-sampling the material's
    //    // reflectance/scattering/transmittance function
    //    float3 matTraceDir = MatDir(abs(sampleDir * 100.0f).xy,
    //                                bxdfID);
    //
    //    // Cache a reference to the probability density associated with the current surface +
    //    // outgoing direction
    //    // Needed for multiple importance sampling, assumes diffuse surfaces for now
    //    float matPDF = MatPDF(mat,
    //                          RaysToAngles(sampleDir,
    //                                       matTraceDir,
    //                                       false),
    //                          bxdfID);
    //
    //    // We only want to accumulate one light sample for each BSDF sample
    //    // (and vice-versa), so use these switches to avoid sampling more than
    //    // once along either ray during each direct gather
    //    bool lightSampling = true;
    //    bool matSampling = true;
    //
    //    // Begin light sampling
    //    float3 traceVec = 0.0f.xxx;
    //    float3 matTraceVec = 0.0f.xxx;
    //    float currRayDist = 0.01f;
    //    float currMatRayDist = 0.01f;
    //    for (uint j = 0; j < MAX_AREA_GATHER_STEPS; j += 1)
    //    {
    //        // Transform tracing vectors to match the ray distance + starting position
    //        traceVec = (traceDir * currRayDist) + samplePoint;
    //        matTraceVec = (matTraceDir * currMatRayDist) + samplePoint;
    //
    //        // Extract distance to the scene + ID value (in the range [0...9])
    //        // for the closest figure (=> test scene intersections for the source
    //        // illumination sample)
    //        float2 sceneField = SceneField(traceVec,
    //                                       samplePoint,
    //                                       true);
    //
    //        // Also test scene intersesctions for the BSDF illumination sample
    //        float2 matSceneField = SceneField(matTraceVec,
    //                                          samplePoint,
    //                                          true);
    //
    //        // Process the intersections driven by the light-sample if appropriate
    //        bool lightRayContact = sceneField.x < adaptEps &&
    //                               lightSampling;
    //        bool matRayContact = matSceneField.x < adaptEps &&
    //                             matSampling;
    //        if (lightRayContact ||
    //            matRayContact)
    //        {
    //            // Most surfaces will be diffuse, so assume successful light samples
    //            // by default
    //            uint surfID = figID;
    //            float misWeight = MISWeight(lightTraceCounter,
    //                                        lightPDF,
    //                                        matTraceCounter,
    //                                        matPDF);
    //            float pdf = lightPDF;
    //            float3 sampleVec = traceVec;
    //            lightSampling = false;
    //            lightTraceCounter += 1;
    //            if (matRayContact)
    //            {
    //                lightSampling = true;
    //                lightTraceCounter -= 1;
    //                surfID = matSceneField.y;
    //                misWeight = MISWeight(matTraceCounter,
    //                                      matPDF,
    //                                      lightTraceCounter,
    //                                      lightPDF);
    //                pdf = matPDF;
    //                sampleVec = matTraceVec;
    //                matRayContact = false;
    //                matTraceCounter += 1;
    //            }
    //
    //            if (surfID == STELLAR_NDX)
    //            {
    //                // "In" direction is assumed to be an emanating direction from the light source
    //                // "Out" direction is the direction traced from the camera towards the surface
    //                float4 thetaPhiIO = RaysToAngles(traceDir,
    //                                                 sampleDir,
    //                                                 camPath);
    //
    //                litRGB += DirIllumRadiance(sampleVec,
    //                                           localNorm,
    //                                           star.rgbaCoeffs[0].xyz,
    //                                           lightPos,
    //                                           currRayDist,
    //                                           star.rgbaCoeffs[2].x,
    //                                           mat.alb) // Evaluate local radiance
    //                                         * MatBXDF(mat,
    //                                                   thetaPhiIO,
    //                                                   bxdfID) // Apply the surface BRDF (diffuse surfaces assumed until I implement volumetric path tracing)
    //                                         * misWeight // Weight this sample appropriately for Multiple Importance Sampling (MIS)
    //                                         * pdf // Account for the probability of a ray leaving the surface passing through [sampleVec]
    //                                         / numDirGaths.x; // Average over the total number of direct samples
    //            }
    //            else
    //            {
    //                // Assume [samplePoint] is shadowed by the other surface, store [0, 0, 0]
    //                litRGB += 0.0f.xxx;
    //            }
    //            break;
    //        }
    //
    //        // Increase ray distance by the distance to the field (step by the minimum safe distance)
    //        currRayDist += sceneField.x;
    //    }
    //}

    // Return the directly-illuminated/shadowed surface color
    // Assumes all rays that didn't reach a surface would have been occluded
    // (which is wrong, but area gather rays should always intersect with SOMETHING anyway...otherwise
    // the state of the ray is technically unknowable and we need to optimize our field functions)
    return 1.0f.xxx; //litRGB;
}

// Process + cache surface data for the interaction occurring at
// [rayVec]
// Also return a store-able [BidirVert] so we can account for this
// interaction during bi-directional path connection/integration
// Not a huge fan of all these side-effecty [out] variables, but
// it feels like the only single-function way to process arbitrary
// camera/light sub-paths without heavy code duplication in
// [SceneVis.hlsl]'s core marching loop
BidirVert ProcVert(float3 rayVec,
                   inout float3 rayDir,
                   float adaptEps,
                   uint figID,
                   uint loopSeed,
                   bool lightPath,
                   float3 lastVtPos,
                   inout float3 atten,
                   inout float3 rayOri,
                   out float rayDist,
                   inout uint randVal)
{
    // Extract material for the current figure
    FigMat mat = FigMaterial(rayVec,
                             figID);

    // Extract a shading BXDF for illumination at the current vertex
    uint bxdfID = MatBXDFID(figID,
                            rayVec - figuresReadable[figID].pos.xyz,
                            mat.bxdfWeights,
                            randVal);

    // Extract surface normal
    float3 normal = tetGrad(rayVec, // Per-figure gradient because generic field gradient is too imperformant for my current PC
                            adaptEps,
                            figuresReadable[figID]).xyz;

    // Generate an importance-sampled bounce direction
    float3 bounceDir = mul(MatDir(randVal,
                                  bxdfID),
                           NormalSpace(normal));

    // Update path attenuation
    // Treat all surfaces as diffuse for now, add support for refraction/reflection later
    // Also cache outgoing PDF value so we can re-use it during sub-path
    // integration/connection
    float4 thetaPhiIO = RaysToAngles(bounceDir,
                                     rayDir,
                                     lightPath);
    float pdfO = MatPDF(mat,
                        thetaPhiIO,
                        bxdfID);
    atten *= MatBXDF(mat,
                     thetaPhiIO,
                     bxdfID) * dot(normal, bounceDir) * pdfO;

    // Update light ray direction (if appropriate)
    rayDir = bounceDir;

    // Update light ray starting position
    // Shift the starting position just outside the figure surface so that rays avoid
    // immediately re-intersecting with the surface
    rayOri = rayVec + (normal * adaptEps * 2.0f);

    // Re-set ray distance as appropriate
    rayDist = adaptEps;

    // Evaluate incoming PDF value (needed for sub-path integration/connection), also
    // convert the incoming/outgoing PDF values from densities over solid angle to
    // densities over differential area
    //float pdfIArea = MatPDF(mat,
    //                        thetaPhiIO.yxwz,
    //                        bxdfID) * AngleToArea(lastVtPos,
    //                                              rayVec,
    //                                              normal,
    //                                              bxdfID == BXDF_ID_VOLU);
    //float pdfOArea = pdfO * AngleToArea(rayVec,
    //                                    lastVtPos,
    //                                    normal,
    //                                    bxdfID == BXDF_ID_VOLU);

    // Bundle the core interaction details into a [BidirVert] so we an easily connect
    // the camera/light subpaths after each one has propagated through the scene
    return BuildBidirVt(rayOri,
                        atten,
                        figID,
                        bxdfID);
}

// Function handling connections between bi-directional vertices and
// returning path colors for the given connection strategy (defined
// by the number of samples selected for each sub-path)
// Cases where the thread loses it's one-to-one relationship with pixels
// in the display texture (e.g. when a gather ray traces back from the
// light subpath towards the camera) are handled by converting the
// incident direction to a pixel ID and storing the result in [w]
float4 ConnectBidirVts(BidirVert camVt,
                       BidirVert lightVt,
                       float2x3 camIODirs,
                       float2x3 lightIODirs,
                       uint numCamSamples,
                       uint numLightSamples,
                       uint2 subpathBounces,
                       bool camPathEnded,
                       Figure star,
                       float cosMaxEmitAngles,
                       float3 selPlanetPos,
                       float adaptEps,
                       float3 eyePos,
                       float3 lensNormal,
                       uint linPixID,
                       inout uint randVal)
{
    float4 connStratRGB = 0.0f.xxxx; // RGB spectra for the selected connection strategy
    if (numLightSamples == 0 && // Ignore the light sub-path
        camPathEnded && // Only valid if the camera sub-path reaches the local star
        numCamSamples == subpathBounces.x) // Only valid if the given camera vertex is the last vertex
                                           // in the camera-subpath (since camera rays terminate at the
                                           // the local star)
    {
        // Attenuated emission or attenuated radiance here?
        // Very unsure, should probably ask CGSE...
        connStratRGB = float4(Emission(star) * camVt.atten.rgb, // Cache attenuated emission
                              linPixID); // No thread relationship changes here, store the local pixel index in [w]
    }
    else if (numCamSamples == 1) // Emit a gather ray towards the camera from the given vertex on the light subpath
    {
        // Gather importance for the terminal light sample from the camera
        float2x3 importance = PerPointImportance(lightVt.pos.xyz,
                                                 camVt.pos.xyz,
                                                 lensNormal);

        // Only assign color to [connStratRGB] if the camera-gather is unoccluded; assume the camera's
        // importance is "shadowed" and set [connStratRGB] to [0.0f.xxx] otherwise
        if (!OccTest(lightVt.pos.xyz,
                     camVt.pos.xyz,
                     adaptEps)[1].w)
        {
            // Cache the local normal at [lightVt.pos.xyz]
            float3 normal = tetGrad(lightVt.pos.xyz,
                                    adaptEps,
                                    figuresReadable[lightVt.pos.w]).xyz;
            // Attenuate the importance at [lightVt] appropriately, then store the result
            // Assume unit attenuation at the camera and 100% transmittance
            connStratRGB.rgb = lightVt.atten.rgb * MatBXDF(FigMaterial(lightVt.pos.xyz,
                                                                       lightVt.pos.w),
                                                           RaysToAngles(lightIODirs[0],
                                                                        importance[1].xyz,
                                                                        true),
                                                           lightVt.atten.w);

            // Grains of participating media are treated like perfect re-emitters, so only
            // attenuate lighting by facing ratio for solid surfaces
            if (lightVt.atten.w != BXDF_ID_VOLU)
            {
                connStratRGB *= dot(importance[1].xyz,
                                    normal);
            }
        }
        else
        {
            connStratRGB.rgb = 0.0f.xxx;
        }

        // Encode shading pixel location in [w]
        //connStratRGB.rgb = float3(0.0f, 1.0f, 0.0f);
        connStratRGB.w = PRayPix(normalize(importance[1].xyz)).z;
    }
    else if (numLightSamples == 1) // Emit a gather ray towards the light from the given vertex on the camera subpath
    {
        // Initialise ray-marching values
        float3 rayOri = camVt.pos.xyz;
        float3 absCamVt = abs(camVt.pos.xyz);
        float3 absLiVt = abs(lightVt.pos.xyz);
        float3 randUVW = (float3(iToFloat(xorshiftPermu1D(randVal)),
                                 iToFloat(xorshiftPermu1D(randVal)),
                                 iToFloat(xorshiftPermu1D(randVal))) * 2.0f.xxx) - 1.0f.xxx;
        float3 stellarSurfPos = AltStellarSurfPos(star.scaleFactor.x,
                                                  star.pos.xyz,
                                                  randUVW);

        // Trace a shadow/gather ray between the last vertex of the camera subpath towards the
        // generated surface position on the light source + shade [rayOri] if the
        // shadow/gather ray is unoccluded (zero [connStratRGB] if [rayOri] is occluded by
        // surfaces in the scene)
        float3x4 occData = OccTest(rayOri,
                                   stellarSurfPos,
                                   adaptEps);
        if (!occData[1].w ||
             occData[2].x == STELLAR_NDX)
        {
            // Cache the local material + normal + input/output angles at [camVt.pos.xyz]
            FigMat mat = FigMaterial(camVt.pos.xyz,
                                     camVt.pos.w);
            float3 normal = tetGrad(camVt.pos.xyz,
                                    adaptEps,
                                    figuresReadable[camVt.pos.w]).xyz;
            float4 thetaPhiIO = RaysToAngles(occData[0].xyz,
                                             camIODirs[0],
                                             false);
            connStratRGB.rgb = DirIllumRadiance(stellarSurfPos,
                                                normal,
                                                star.rgbaCoeffs[0].xyz,
                                                star.pos.xyz,
                                                occData[0].w,
                                                star.rgbaCoeffs[2].x) // Evaluate local radiance
                                              * MatBXDF(mat,
                                                        thetaPhiIO,
                                                        camVt.atten.w) // Apply the surface BRDF
                                              * StellarPosPDF(); // Account for the probability of selecting
                                                                 // [stellarSurfPos] out of all possible
                                                                 // positions on the surface of the local
                                                                 // star
        }
        else
        {
            connStratRGB.rgb = 0.0f.xxx;
        }

        // No thread relationship changes here, so store the local pixel index in [w]
        connStratRGB.w = linPixID;
    }
    else // Handle generic bi-directional connection strategies
    {
        // Evaluate visibility between [lightVt] and [camVt]
        float3x4 occData = OccTest(lightVt.pos.xyz,
                                   camVt.pos.xyz,
                                   adaptEps);

        // Extract the local normals at [camVt.pos.xyz] and
        // [lightVt.pos.xyz]
        float3 camNormal = tetGrad(camVt.pos.xyz,
                                   adaptEps,
                                   figuresReadable[camVt.pos.w]).xyz;
        float3 lightNormal = tetGrad(lightVt.pos.xyz,
                                     adaptEps,
                                     figuresReadable[lightVt.pos.w]).xyz;

        // Use the evaluated visibility to define the generalized visibility function [g]
        float g = !occData[1].w * // Point-to-point visibility
                  AngleToArea(lightVt.pos.xyz, // Angle-to-area integral conversion function
                              camVt.pos.xyz,
                              lightNormal,
                              camVt.atten.w != BXDF_ID_VOLU);

        // AngleToArea[...] implicitly handles facing ratio for incident surfaces ([camVt.bxdfID]),
        // but not emissive ones ([lightVt.bxdfID]); account for that here (if appropriate)
        if (lightVt.atten.w != BXDF_ID_VOLU)
        {
            g *= dot(camNormal,
                     normalize(camVt.pos.xyz - lightVt.pos.xyz));
        }

        // We've defined the generalized visibility function [g], so we can safely move through and
        // define [connStratRGB.rgb] with the radiance emitted from [lightVt.pos.xyz] through
        // [camVt.pos.xyz]
        // Assumes 100% transmittance
        float4 thetaPhiIOCamera = RaysToAngles(occData[0].xyz * -1.0f,
                                               camIODirs[0],
                                               false);
        float4 thetaPhiIOLight = RaysToAngles(occData[0].xyz,
                                              lightIODirs[0],
                                              true);
        float3 camBXDF = MatBXDF(FigMaterial(camVt.pos.xyz,
                                             camVt.pos.w),
                                 thetaPhiIOCamera,
                                 camVt.atten.w);
        float3 lightBXDF = MatBXDF(FigMaterial(lightVt.pos.xyz,
                                               lightVt.pos.w),
                                   thetaPhiIOLight,
                                   lightVt.atten.w);
        connStratRGB.rgb = camVt.atten.rgb * camBXDF * // Evaluate + apply attenuated radiance on the camera subpath
                           lightVt.atten.rgb * lightBXDF * // Evaluate + apply attenuated radiance on the light subpath;
                           g; // Scale the integrated radiance by the relative visibility of [camVt] from
                              // [lightVt]'s perspective (and vice-versa, since BDPT is symmetrical)
        // No thread relationship changes here, so store the local pixel index in [w]
        connStratRGB.w = linPixID;
    }

    // Filter evaluated radiance/importance values with MIS (Multiple Importance Sampling)
    return connStratRGB; // * MISWeight();
}