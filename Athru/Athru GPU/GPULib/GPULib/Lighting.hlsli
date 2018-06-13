
#include "Core3D.hlsli"
#include "LightingUtility.hlsli"
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
    float3 bounceDir = mul(MatDir(randVal,
                                  bxdfID),
                           normSpace);

    // Evaluate the current vertex's outgoing directions + PDF
    // (both needed for attenuation atm + path integration
    // later)
    float4 thetaPhiIO = RaysToAngles(bounceDir,
                                     mul(rayDir, normSpace),
                                     lightPath);
    float2 pdfIO = float2(MatPDF(thetaPhiIO,
                                 bxdfID), // Probability of exiting along [bounceDir] after entering along [rayDir]
                          MatPDF(thetaPhiIO.wzyx,
                                 bxdfID)); // Probability of exiting along [rayDir] after entering along [bounceDir]

    // Cache the bi-directional vertex representing the current interaction
    BidirVert bdVt = BuildBidirVt(float4(rayOri, figIDDFType.x),
                                  float4(atten, bxdfID),
                                  float4(normal, figIDDFType.y),
                                  thetaPhiIO,
                                  float3(pdfIO, rayVec.w));

    // Update [out] parameters

    // Update path attenuation
    // Treat all surfaces as diffuse for now, add support for refraction/reflection later
    atten *= MatBXDF(rayVec.xyz,
                     prevFres,
                     thetaPhiIO,
                     true,
                     uint3(bxdfID, figIDDFType.yx)) * dot(normal, bounceDir) * pdfIO.x;

    // Update light ray direction (if appropriate)
    rayDir = bounceDir;

    // Update light ray starting position
    // Shift the starting position just outside the figure surface so that rays avoid
    // immediately re-intersecting with the surface
    rayOri = rayVec.xyz + (normal * adaptEps * 2.0f);

    // Re-set ray distance as appropriate
    rayDist = adaptEps;

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
                       uint numCamSamples,
                       uint numLightSamples,
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
    if (numLightSamples == 0 && // Ignore the light sub-path
        subpathEnded.x && // Only valid if the camera sub-path reaches the local star
        numCamSamples == subpathBounces.x) // Only valid if the given camera vertex is the last vertex
                                           // in the camera-subpath (since camera rays terminate at the
                                           // the local star)
    {
        // Attenuated emission or attenuated radiance here?
        // Very unsure, should probably ask CGSE...
        connStratRGB = float4(STELLAR_BRIGHTNESS.xxx * camVt.atten.rgb, // Cache attenuated emission
                              linPixID); // No thread relationship changes here, store the local pixel index in [w]
    }
    else if (numCamSamples == 0 && // Ignore the camera sub-path
             subpathEnded.y &&
             numLightSamples == subpathBounces.y) // Only valid if the given camera vertex is the last vertex in
                                                  // the light-subpath (since light rays terminate at the lens)
    {
        // Same issue as the camera subpath, attenuated emission or attenuated radiance?
        // Should ask CGSE about that eventually...
        connStratRGB = float4(STELLAR_BRIGHTNESS.xxx * lightVt.atten.rgb,
                              PRayPix(cameraPos.xyz - lightVt.pos.xyz,
                                      camInfo.w).z);
    }
    else if (numCamSamples == 1) // Emit a gather ray towards the camera from the given vertex on the light subpath
    {
        // Only assign color to [connStratRGB] if the camera-gather is unoccluded; assume the camera's
        // importance is "shadowed" and set [connStratRGB] to [0.0f.xxx] otherwise
        float3x4 occData = OccTest(lightVt.pos.xyz,
                                   camVt.pos.xyz,
                                   adaptEps);

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
									   false,
                                       uint3(lightVt.atten.w,
                                             lightVt.norml.w,
                                             lightVt.pos.w)) * // Attenuation by the source interaction's local BXDF
                               RayImportance(occData[0].xyz,
                                             camInfo,
                                             pixWidth) / // Attenuation by the importance emitted from the camera towards [lightVt.pos.xyz]
                               CamAreaPDFIn(lightVt.pos.xyz - camVt.pos.xyz, // Scaling to match the probability of gather rays reaching the lens/pinhole along [occData[0].xyz]
                                            camInfo.xyz);

            // Grains of participating media are treated like perfect re-emitters, so only
            // attenuate lighting by facing ratio for solid surfaces
            if (lightVt.atten.w != BXDF_ID_MEDIA)
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
    else if (numLightSamples == 1) // Emit a gather ray towards the light from the given vertex on the camera subpath
    {
        // Initialise ray-marching values
        float3 rayOri = camVt.pos.xyz;
        float3 randUVW = float3(iToFloat(xorshiftPermu1D(randVal)),
                                iToFloat(xorshiftPermu1D(randVal)),
                                iToFloat(xorshiftPermu1D(randVal)));
        float3 stellarSurfPos = StellarSurfPos(starLinTransf.w,
                                               starLinTransf.xyz,
                                               randUVW);

        // Trace a shadow/gather ray between the last vertex of the camera subpath towards the
        // generated surface position on the light source + shade [rayOri] if the
        // shadow/gather ray is unoccluded (zero [connStratRGB] if [rayOri] is occluded by
        // surfaces in the scene)
        float3x4 occData = OccTest(rayOri,
                                   stellarSurfPos,
                                   adaptEps);
        float stellarPosPDF = StellarPosPDF();
        if (!occData[1].w ||
             occData[2].x == STELLAR_FIG_ID)
        {
            // Cache the local material + normal + input/output angles at [camVt.pos.xyz]
            float4 thetaPhiIO = float4(VecToAngles(occData[0].xyz),
                                       camVt.ioSrs.zw);
            connStratRGB.rgb = DirIllumRadiance(stellarSurfPos,
                                                camVt.norml.xyz,
                                                STELLAR_RGB,
                                                starLinTransf.xyz,
                                                occData[0].w,
                                                STELLAR_BRIGHTNESS) // Evaluate local radiance
                                              * MatBXDF(camVt.pos.xyz,
                                                        AmbFres(camVt.pdfIO.z), // Not worrying about subsurface scattering just yet...
                                                        thetaPhiIO,
                                                        false,
                                                        uint3(camVt.atten.w,
                                                              camVt.norml.w,
                                                              camVt.pos.w)) // Apply the surface BRDF
                                              * stellarPosPDF; // Account for the probability of selecting
                                                               // [stellarSurfPos] out of all possible
                                                               // positions on the surface of the local
                                                               // star
        }
        else
        {
            connStratRGB.rgb = 0.0f.xxx;
        }

        // We integrated a sample outside the camera/light subpaths, so define [gatherPDFs] appropriately
        // Using [stellarPosPDF] for forward + reverse probabilities because the directional PDF is only
        // designed to fit the cylindrical distribution used to concentrate primary light-rays in the system
        // disc; "actual" star emission will always be spherical, so we need to use a spherical PDF
        gatherPDFs = float2(stellarPosPDF,
                            stellarPosPDF);

        // Record the gather's position here so we can accurately convert PDFs for MIS weights
        // during integration
        gatherPos = occData[1].xyz;

        // No thread relationship changes here, so store the local pixel index in [w]
        connStratRGB.w = linPixID;
    }
    else // Handle generic bi-directional connection strategies
    {
        // Evaluate visibility between [lightVt] and [camVt]
        float3x4 occData = OccTest(lightVt.pos.xyz,
                                   camVt.pos.xyz,
                                   adaptEps);

        // Use the evaluated visibility to define the generalized visibility function [g]
        float g = !occData[1].w * // Point-to-point visibility
                  AngleToArea(lightVt.pos.xyz, // Angle-to-area integral conversion function
                              camVt.pos.xyz,
                              lightVt.norml.xyz,
                              camVt.atten.w != BXDF_ID_MEDIA);

        // AngleToArea[...] implicitly handles facing ratio for incident surfaces ([camVt.bxdfID]),
        // but not emissive ones ([lightVt.bxdfID]); account for that here (if appropriate)
        if (lightVt.atten.w != BXDF_ID_MEDIA)
        {
            g *= dot(camVt.norml.xyz,
                     normalize(camVt.pos.xyz - lightVt.pos.xyz));
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
                                 false,
                                 uint3(camVt.atten.w,
                                       lightVt.norml.w,
                                       camVt.pos.w));
        float3 lightBXDF = MatBXDF(lightVt.pos.xyz,
                                   AmbFres(lightVt.pdfIO.z), // Not worrying about subsurface scattering atm...
                                   camLightIO[1],
                                   false,
                                   uint3(lightVt.atten.w,
                                         camVt.norml.w,
                                         lightVt.pos.w);
        connStratRGB.rgb = camVt.atten.rgb * camBXDF * // Evaluate + apply attenuated radiance on the camera subpath
                           lightVt.atten.rgb * lightBXDF * // Evaluate + apply attenuated radiance on the light subpath;
                           g; // Scale the integrated radiance by the relative visibility of [camVt] from
                              // [lightVt]'s perspective (and vice-versa, since BDPT is symmetrical)
        // No thread relationship changes here, so store the local pixel index in [w]
        connStratRGB.w = linPixID;
    }

    // Return the evaluated radiance/importance value for the given vertices
    return connStratRGB;
}