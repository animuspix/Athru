
#include "Lighting.hlsli"
#include "ScenePost.hlsli"
#ifndef RASTER_CAMERA_LINKED
    #include "RasterCamera.hlsli"
#endif
#ifndef MIS_FNS_LINKED
    #include "MIS.hlsli"
#endif

// A two-dimensional texture representing the display; used
// as the image source during core rendering (required because
// there's no support for direct render-target access in
// DirectX)
RWTexture2D<float4> displayTex : register(u0);

// Buffer carrying traceable pixels + pixel information
// Ray directions are in [0][xyz], filter values are in [1][x],
// pixel indices are in [1][y], non-normalized ray z-offsets
// are in 1[z]
ConsumeStructuredBuffer<float2x3> traceables : register(u3);

// Intersection counter ([traceables] without SWR); included
// here because there's no safe way to reset it per-frame
// from within [PathReduce.hlsl]
RWBuffer<int> maxTraceables : register(u4);

// Tracing modes to use for scene visualization
#define TRACE_MODE_REVERSE 0 // Trace single paths out from the camera towards the light source
//#define TRACE_MODE_FORWARD 1 // Trace single paths out from the light source towards the lens

// Trace radiance/importance probes from the lens + the light source and integrate appropriately at each
// vertex
// Automatically defined if forward/reverse path-tracing are enabled at the same time
#ifdef TRACE_MODE_REVERSE
    #ifdef TRACE_MODE_FORWARD
        #define TRACE_MODE_BIDIREC 2
    #endif
#endif

// The maximum number of steps allowed for the primary
// ray-marcher
#define MAX_VIS_MARCHER_STEPS 256

// Scale epsilon values to match the camera's distance from the system
// planets + star
float AdaptEps(float3 camRayOri)
{
    // Evaluate average distance to the camera across all figures
    float avgDist = 0.0f;
    for (uint i = 0; i < MAX_NUM_FIGURES; i += 1)
    {
        avgDist += PlanetDF(camRayOri,
                            figures[i],
                            i,
                            EPSILON_MAX)[0].x;
    }

    // Initialise an epsilon value to [EPSILON_MIN] and make it twice as
    // large for every five-unit increase in average distance (up to a maximum of
    // [EPSILON_MAX])
    // E.g. a ten-unit distance means a two-times increase (2^1)
    //      a fifty-unit distance means a 1024-times increase (2^10)
    //      a seventy-five-unit distance means a 16384-times increase (2^15)
    //      a hundred-unit distance means a ~1,000,000-times increase (2^20)
    // BUT
    // Epsilons are still thresholded at [MARCHER_EPSILON_MAX], so exponential
    // increases in scaling distance never cause significant ray over-stepping
    // (and thus invalid distances)
    float adaptEps = EPSILON_MIN;
    adaptEps *= pow(2, avgDist / 5.0f);
    return min(max(adaptEps, EPSILON_MIN),
               EPSILON_MAX);
}

[numthreads(8, 4, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // The maximum number of traceables for this frame was cached on the CPU
    // immediately after [PathReduce.hlsl] finished executing, so zero
    // [maxTraceables] here
    InterlockedAnd(maxTraceables[0], 0);

    // The number of traceable elements emitted by [PathReduce]
    // (i.e. the number of traceable elements) limits the number
    // of path-tracing threads that can actually execute;
    // immediately exit from any threads outside that limit
    // (required because we want to spread path-tracing threads
    // evenly through 3D dispatch-space (rational cube-roots are
    // uncommon + force oversized dispatches), also because we
    // can't set per-group thread outlook (i.e. [numthreads])
    // programatically before dispatching [this])
    uint dispWidth = (uint)timeDispInfo.w;
    uint linGroupID = (groupID.x + groupID.y * dispWidth) +
                      (groupID.z * (dispWidth * dispWidth));
    uint linDispID = (linGroupID * 128) + (threadID + 1);
    if (linDispID > (uint)timeDispInfo.z) { return; }

    // [Consume()] a pixel from the end of [traceables]
    float2x3 traceable = traceables.Consume();

    // Separately cache the pixel's screen position for later reference
    uint2 pixID = uint2(traceable[1].y % resInfo.x,
                        traceable[1].y / resInfo.x);

    // Extract a Xorshift-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    uint randNdx = xorshiftNdx(pixID.x + (pixID.y * resInfo.x));
    uint randVal = randBuf[randNdx];

    // Cache/generate initial origin + direction for the light/camera subpaths

    // Cache initial origin for the light/camera subpaths
    // Importance-sampled rays are emitted for random planets
    Figure star = figures[STELLAR_FIG_ID];
    float2x3 subpathOris = float2x3(cameraPos.xyz,
                                    LightSurfPos(LIGHT_ID_STELLAR,
                                                 star.linTransf,
                                                 PlanetDF(0.0f.xxx, // We only care about planet position here, so input an arbitrary ray location + origin and ignore figure boundaries
                                                          figures[1],
                                                          0x1,
                                                          EPSILON_MAX)[1].xyz,
                                                 randVal));

    // Transform [traceable]'s camera ray direction to match the view matrix, also generate + cache a separate
    // light ray direction
    float3 initLiNorml = normalize(subpathOris[1] - star.linTransf.xyz);
    float3 liDir = LightEmitDir(LIGHT_ID_STELLAR,
                                subpathOris[1],
                                randVal);
    float2x3 rayDirs = float2x3(traceable[0],
                                liDir);

    // Matrix defining the light/camera-relative position of rays during light/camera-subpath construction
    float2x3 subpathVecs = float2x3(0.001f.xxx,
                                    0.001f.xxx);

    // Define an adaptive epsilon (marching cutoff) for optimal
    // marching performance (no reason to march to within a hundred-thousandth of
    // a surface when the camera is 150 units away anyways)
    float adaptEps = AdaptEps(subpathOris[0]);

    // Baseline ray distances
    // Maximum ray distance is stored inside the [MAX_RAY_DIST] constant within [Core3D.hlsli]
    float2 rayDistVec = (adaptEps * 2.0f).xx;

    // March the scene

    // Rays might reach their target light source before the maximum number of bounces; this switch checks
    // for that and allows the bounce-loop to end as soon as possible
    bool2 subpathEnded = false.xx;

    // Some rays pass harmlessly through the scene because they were never emitted in the first place; check
    // for that case here so we can color those rays appropriately
    bool2 subpathEscaped = false.xx;

    // Spectral sub-path attenuation over multiple bounces and materials, initialized to one (for the camera)
    // and local radiance scaled by the probability of selecting the chosen light sample (for the light source)
    float2x3 atten = float2x3(1.0f.xxx,
                              Emission(STELLAR_RGB, STELLAR_BRIGHTNESS.xxx, 0.0f) * LightPosPDF(LIGHT_ID_STELLAR));

    // Paired array of camera/light sub-path vertices within the scene; needed for flexible path generation in BDPT
    BidirVtPair bidirVts[MAX_BOUNCES_PER_SUBPATH + 1]; // One extra vertex pair to store the camera's pinhole + the sampled position
                                                       // on the light source

    // Pre-fill the first vertex on the camera sub-path with the baseline camera sample
    // No figures or surfaces at the camera pinhole/lens, so fill those with placeholder values
    // (11 => only ever 10 figures in the scene; 5 => only four defined BXDFs)
    float3 lensNormal = mul(float4(0.0f, 0.0f, 1.0f, 1.0f), viewMat).xyz;
    float4 camInfo = float4(lensNormal, traceable[1].x);
    float pixWidth = sqrt(NUM_AA_SAMPLES);

    // In/out ray directions model importance emission as transmission from the camera position through the lens/pinhole
    //float2 initCamAngles = VecToAngles(rayDirs[0].xyz);
    bidirVts[0].camVt = BuildBidirVt(float4(subpathOris[0], LENS_FIG_ID), // Cache world-space sample position + camera figure-ID
                                     cameraPos.xyz, // Cache camera position
                                     float4(atten[0], BXDF_ID_NOMAT), // Cache baseline attenuations + null BXDF ID (no material properties
                                                                      // for lenses atm)
                                     float4(lensNormal, DF_TYPE_LENS), // Cache lens normal + lens distance-function type
                                     float2x3(rayDirs[0],
                                              rayDirs[0]), // Assume equivalent input/output directions for subpath endpoints
                                     float2x4(0.0f.xxxx,
                                              0.0f.xxxx), // Null material since light never explicitly interacts with the lens atm
                                     float3(CamAreaPDFOut(), // Output probability, i.e. chance of emitting a ray through [pixID]
                                            1.0f, // Filler input probability; depends on a defined importance/radiance source within the scene
                                                  // so left undefined for now
                                            SceneField(subpathOris[0],
                                                       0.0f.xxx,
                                                       false,
                                                       STELLAR_FIG_ID,
                                                       adaptEps)[0].x)); // Planetary distance for the lens/pinhole position

    // Pre-fill the first vertex on the light sub-path with the baseline light sample
    // BXDF ID assumes that stars are 100% diffuse emitters
    // In/out ray directions model radiance emission as transmission from the light position through the lens/pinhole
    bidirVts[0].lightVt = BuildBidirVt(float4(subpathOris[1], STELLAR_FIG_ID),
                                       star.linTransf.xyz,
                                       float4(atten[1], BXDF_ID_DIFFU),
                                       float4(initLiNorml, DF_TYPE_STAR),
                                       float2x3(liDir,
                                                liDir), // Assume equivalent input/output directions for subpath endpoints
                                       MatGen(subpathOris[1],
                                              initLiNorml,
                                              DF_TYPE_STAR,
                                              randVal),
                                       float3(StellarPosPDF(),
                                              StellarPosPDF(),
                                              EPSILON_MAX * 2.0f)); // No way points on the star would ever be less than [EPSILON_MAX] from a
                                                                    // planetary surface, so set this to a reasonably-high filler value
                                                                    // instead of calculating it directly from [SceneField(...)]
    // Bounces + core ray-marching loop
    // Generate camera/light sub-paths here
    uint2 numBounces = 0u.xx; // While-loop so we can get correct averages across the local number of bounces in each path
    while (any(numBounces < maxNumBounces.xx &&
               !subpathEnded &&
               !subpathEscaped))
    {
        // Small flag to record whether or not intersections from a camera/light ray should be processed during
        // ray-marching; marching ends when neither ray should be processed (i.e. both have intersected with the scene/escaped)
        bool2 rayActiVec = numBounces < maxNumBounces.xx;
        while (any(rayActiVec))
        {
            // Scale rays into the scene, add source position (star/camera origin)
            subpathVecs = float2x3((rayDistVec.x * rayDirs[0]) + subpathOris[0],
                                   (rayDistVec.y * rayDirs[1]) + subpathOris[1]);

            // Extract distance to the scene + ID value (in the range [0...9])
            // for the closest figure
            // Result for the camera sub-path go in [xy], results for the light
            // sub-path go in [zw]
            float4x3 sceneField = float4x3(SceneField(subpathVecs[0],
                                                      subpathOris[0],
                                                      true,
                                                      FILLER_SCREEN_ID,
                                                      adaptEps), // Filler filter ID
                                           SceneField(subpathVecs[1],
                                                      subpathOris[1],
                                                      true,
                                                      STELLAR_FIG_ID,
                                                      adaptEps)); // We want to avoid light rays re-intersecting with the local star

            // Process camera-ray intersections if appropriate
            if (sceneField[0].x < adaptEps &&
                rayActiVec.x)
            {
                // Process the current intersection, then return + store a BDPT-friendly surface interaction
                // Storage at [k + 1] because the zeroth index in either sub-path is reserved for the initial
                // light/camera sample in the path
                bidirVts[numBounces.x + 1].camVt = ProcVert(float4(subpathVecs[0],
                                                                   sceneField[0].x),
                                                            sceneField[1],
                                                            rayDirs[0],
                                                            adaptEps,
                                                            sceneField[0].yz,
                                                            false,
                                                            bidirVts[numBounces.x].camVt.pos.xyz,
                                                            atten[0],
                                                            subpathOris[0],
                                                            AmbFres(sceneField[0].x), // Not worrying about subsurface scattering atm
                                                            rayDistVec.x,
                                                            randVal);

                // End camera-subpaths if they intersect with the local star
                subpathEnded.x = (sceneField[0].y == STELLAR_FIG_ID);

                // Ignore the current intersection while the light-ray marches through the scene
                rayActiVec.x = false;
            }

            // Process light-ray intersections if appropriate
            // (+ test for lens intersection)
            float lensDist = LensDF(subpathVecs[1], adaptEps);
            bool lightIsect = (sceneField[2].x < adaptEps ||
                               lensDist < adaptEps);
            if (lightIsect &&
                rayActiVec.y)
            {
                // We intersected a surface (either in the scene or the lens), so store a BDPT-friendly
                // vertex entity with the relevant position + attenuation
                // Vertices are stored at n + 1 because the zeroth index in either sub-path is reserved for the
                // initial sample in that subpath (i.e. a light surface position for light rays, a lens
                // position for camera rays)
                if (lensDist > adaptEps)
                {
                    // We intersected a scene surface, so process the local vertex and return + store a
                    // BDPT-friendly surface interaction
                    bidirVts[numBounces.y + 1].lightVt = ProcVert(float4(subpathVecs[1],
                                                                         sceneField[2].x),
                                                                  sceneField[3],
                                                                  rayDirs[1],
                                                                  adaptEps,
                                                                  sceneField[2].yz,
                                                                  true,
                                                                  bidirVts[numBounces.y].lightVt.pos.xyz,
                                                                  atten[1],
                                                                  subpathOris[1],
                                                                  AmbFres(sceneField[2].x), // Not worrying about subsurface scattering just yet..
                                                                  rayDistVec.y,
                                                                  randVal);
                }
                else
                {
                    // We intersected the lens, so cache the intersection position and flag the end of the light-subpath
                    // (since we aren't expecting light to bounce back out of the camera)
                    //float2 incidAngles = VecToAngles(rayDirs[1].xyz);
                    bidirVts[numBounces.y + 1].lightVt = BuildBidirVt(float4(subpathVecs[1], LENS_FIG_ID),
                                                                      sceneField[3],
                                                                      float4(atten[1], BXDF_ID_NOMAT), // Assume no attenuation/material interactions at the lens
                                                                      float4(lensNormal, DF_TYPE_LENS), // Lens normal + lens distance-function type
                                                                      float2x3(rayDirs[1],
                                                                               rayDirs[1]), // Assume equivalent input/output directions for subpath endpoints
                                                                      float2x4(0.0f.xxxx,
                                                                               0.0f.xxxx), // Null material for lens interactions
                                                                      float3(CamAreaPDFIn(bidirVts[numBounces.y].lightVt.pos.xyz,
                                                                                          lensNormal), // Probability of the light-subpath reaching the camera
                                                                                                       // from its previous bounce
                                                                             CamAreaPDFOut(), // Probability of a ray leaving the camera passing through [rayDirs[1]]
                                                                             sceneField[2].x)); // Scene distance at the lens
                    subpathEnded.y = true;
                }
                // Ignore the current intersection while the camera-ray marches through the scene
                rayActiVec.y = false;
            }

            // Increase ray distances by per-path field distance (take the largest steps possible
            // without passing through a surface boundary)
            rayDistVec += float2(sceneField[0].x,
								 sceneField[2].x);

            // Break out if the current ray distance passes the threshold defined by [maxRayDist]
            // Subtraction by epsilon is unneccessary and just provides consistency with the limiting
            // epsilon value used for intersection checks
            subpathEscaped = (rayDistVec > (MAX_RAY_DIST - adaptEps).xx);

            // Ignore escaped rays while contained rays march through the scene
            rayActiVec = rayActiVec && !subpathEscaped;
        }
        // Step into the next bounce for each sub-path (if appropriate)
        numBounces += (!subpathEscaped && !subpathEnded && (numBounces < maxNumBounces.xx));
    }

    // Estimated path color associated with the current pixel
    // Random camera-gathers emitted in previous frames might have deposited light
    // on the current sensor/pixel, so add that light into the current path
    // color if appropriate
    // Cache a linearized version of the local pixel ID for later reference
    uint linPixID = traceable[1].y;
    float3 pathRGB = aaBuffer[linPixID].incidentLight.rgb;

    // We want to avoid accumulating incidental radiance separately to per-pixel samples, so zero
    // [aaBuffer[linPixID].incidentLight.rgb] after caching its value in [incidentIllum]
    aaBuffer[linPixID].incidentLight.rgb = 0.0f.xxx;

    // We've collected + reset incident light for the current sensor/pixel, so now we can begin
    // integrating through the camera/light subpaths (if appropriate)
    // Avoid connecting the camera/light subpaths if the initial camera ray never intersects the scene
    if (numBounces.x != 0)
    {
        // We use path-space regularization (PSR) to blur delta-distributed BRDFs in the light/camera subpaths;
        // PSR's operating bandwidth (=> blur radius) is dependant on per-pixel sample count and can't be
        // known ahead of time, so evaluate it here instead
        float pxPSRRadius = basePSRRadius * pow(aaBuffer[traceable[1].x].sampleCount.x, psrLam);

        // Evaluate the camera-only path through the scene (only if the camera-subpath intersects with
        // the local star)
        #ifdef TRACE_MODE_REVERSE
            if (subpathEnded.x)
            {
                // Camera path radiance is taken as the illumination at the second-last camera vertex in the scene adjusted for attenuation
                // at the final vertex
                PathVt lastCamVt = bidirVts[numBounces.x + 1].camVt;
                PathVt secndLastCamVt = bidirVts[numBounces.x].camVt;
                pathRGB += Emission(STELLAR_RGB, STELLAR_BRIGHTNESS, length(secndLastCamVt.pos.xyz - lastCamVt.pos.xyz)) * lastCamVt.atten.rgb;
            }
        #endif
        #ifdef TRACE_MODE_REVERSE
            // Evaluate MIS-weighted light gathers for every surface vertex in the camera subpath (for bi-directional integration) or just
            // for final vertices in paths that failed to reach a light source (for uni-directional integration)
            #ifdef TRACE_MODE_BIDIREC
                for (int i = 1; i <= numBounces.x; i += 1) // Zeroth vertices lie on the lens/the light
                {
                    pathRGB += LiGather(bidirVts[i].camVt,
                                        star.linTransf, // Only stellar light sources atm
                                        adaptEps,
                                        randVal);
                }
            #else
                if (!subpathEnded.x)
                {
                    pathRGB += LiGather(bidirVts[min(numBounces.x + 1, (maxNumBounces.x - 1))].camVt,
                                        star.linTransf, // Only stellar light sources atm
                                        adaptEps,
                                        randVal) * bidirVts[numBounces.x].camVt.atten.rgb; // Should technically scale this by the previous vertex's attenuation, buuuuuuut it looks worse so
                                                  // I'm sticking to my biased alternative for now :)
                }
            #endif
        #endif

        // Evaluate the light-only path through the scene (only if the light-subpath intersects with the lens)
        #ifdef TRACE_MODE_FORWARD
            if (subpathEnded.y)
            {
                // Camera path radiance is taken as the illumination at the second-last camera vertex in the scene adjusted for attenuation
                // at the final vertex
                PathVt lastLiVt = bidirVts[numBounces.y + 1].lightVt;
                PathVt secndLastLiVt = bidirVts[numBounces.y].lightVt;
                pathRGB = float3(1.0f, 0.0f.xx);
                //aaBuffer[(int)PRayPix(normalize(cameraPos.xyz - lastLiVt.pos.xyz),
                //                                camInfo.w).z].incidentLight.rgb += Emission(STELLAR_RGB, STELLAR_BRIGHTNESS, length(secndLastLiVt.pos.xyz - lastLiVt.pos.xyz)) * lastLiVt.atten.rgb;
            }
        #endif
        #ifdef TRACE_MODE_FORWARD
            // Evaluate MIS-weighted camera gathers for every surface vertex in the light subpath (for bi-directional integration) or just
            // for final vertices in paths that failed to reach the lens (for uni-directional integration)
            #ifdef TRACE_MODE_BIDIREC
                for (int j = 1; j <= numBounces.y; j += 1) // Zeroth vertices lie on the lens/the light
                {
                    float4 gathInfo = CamGather(bidirVts[j].lightVt,
                                                camInfo,
                                                adaptEps,
                                                randVal);
                    aaBuffer[(int)gathInfo.w].incidentLight.rgb = /*gathInfo.rgb */float3(1.0f, 0.0f.xx);
                }
            #else
                if (/*!subpathEnded.y && */numBounces.y > 0)
                {
                    float4 gathInfo = CamGather(bidirVts[numBounces.y].lightVt,
                                                camInfo,
                                                adaptEps,
                                                randVal);
                    aaBuffer[/*(int)gathInfo.w*/linPixID].incidentLight.rgb = 1.0f.xxx;//gathInfo.rgb;
                }
            #endif
        #endif
        #ifdef TRACE_MODE_BIDIREC
            // Attempt connections between every surface vertex in either subpath
            for (int k = 1; k <= numBounces.x; k += 1) // Zeroth vertices lie on the lens/the light
            {
                for (int l = 1; l <= numBounces.y; l += 1)
                {
                    // Subpath connections are always assumed to contribute towards the pixel
                    // associated with the camera subpath
                    pathRGB += ConnectBidirVts(bidirVts[k].camVt,
                                               bidirVts[k].lightVt,
                                               adaptEps,
                                               randVal);
                }
            }
        #endif
    }
    else
    {
        // Shade empty pixels with the ambient background color
        pathRGB = AMB_RGB;
    }

    // Apply frame-smoothing + convert to HDR
    pathRGB = PathPost(pathRGB,
                       traceable[1].x,
                       linPixID);

    // Write the final ray color to the display texture
    displayTex[pixID] = float4(pathRGB, 1.0f);

    // No more random permutations at this point, so commit [randVal] back into
    // the GPU random-number buffer ([randBuf])
    randBuf[randNdx] = randVal;
}
