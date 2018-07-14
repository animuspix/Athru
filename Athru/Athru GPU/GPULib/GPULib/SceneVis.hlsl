
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
RWTexture2D<float4> displayTex : register(u1);

// Buffer carrying traceable pixels + pixel information
// Ray directions are in [0][xyz], filter values are in [1][x],
// pixel indices are in [1][y], non-normalized ray z-offsets
// are in 1[z]
ConsumeStructuredBuffer<float2x3> traceables : register(u4);

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
        avgDist += FigDF(camRayOri,
                         0.0f.xxx,
                         false,
                         figuresReadable[i])[0].x;
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
    // The number of traceable elements emitted by [PathReduce]
    // (i.e. the number of traceable elements) limits the number
    // of path-tracing threads that can actually execute;
    // immediately exit from any threads outside that limit
    // (required because we want to spread path-tracing threads
    // evenly through 3D dispatch-space (rational cube-roots are
    // uncommon + force oversized dispatches), also because we
    // can't set per-group thread outlook (i.e. [numthreads])
    // programatically before dispatching [this])
    uint groupWidth3D = ceil(pow(abs(timeDispInfo.w), 1.0f / 3.0f));
    uint linGroupID = (groupID.x + groupID.y * groupWidth3D) +
                      (groupID.z * (groupWidth3D * groupWidth3D));
    uint linDispID = (linGroupID * 128) + (threadID + 1);
    if (linDispID > (uint)timeDispInfo.z) { return; }

    // [Consume()] a pixel from the end of [traceables]
    float2x3 traceable = traceables.Consume();

    // Separately cache the pixel's screen position for later reference
    uint2 pixID = uint2(traceable[1].y % DISPLAY_WIDTH,
                        traceable[1].y / DISPLAY_WIDTH);

    // Extract a Xorshift-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    uint randNdx = xorshiftNdx(pixID.x + (pixID.y * DISPLAY_WIDTH));
    uint randVal = randBuf[randNdx];

    // Reduce work for dense views by randomly culling some percentage
    // of intersecting pixels
    // Culling logic compares random numbers to a maximum threshold after
    // scaling by the number of threads/pixel (i.e. the amount of GPU work
    // per-frame); any threads with numbers below the threshold are rejected
    // Stochastic pixel culling directly introduces image noise; we can
    // mitigate this by smudging surrounding pixels through the culled region,
    // but that also introduces blur
    // Scaling the culling threshold by thread density guarantees that image
    // quality (>> blurriness) scales neatly with total workload, so that
    // relatively un-demanding frames (like e.g. a 16,000-pixel traceable
    // circle) are affected much less than highly-demanding frames that fill
    // the whole screen
    //const float maxThresh = 0.5f;
    //float thrdDensity = timeDispInfo.z / DISPLAY_AREA;
    //if (iToFloat(xorshiftPermu1D(randVal)) < (maxThresh * thrdDensity))
    //{
    //    // Write an empty ray color to the display texture; also tag the
    //    // alpha channel so the presentation shader can safely fill in
    //    // [pixID] from nearby pixels
    //    displayTex[pixID] = float4(0.0f.xxx, RAND_PATH_REDUCTION);
    //
    //    // No more random permutations at this point, so commit [randVal] back into
    //    // the GPU random-number buffer ([randBuf])
    //    randBuf[randNdx] = randVal;
    //
    //    // We want to avert path-tracing for culled pixels, so return here
    //    return;
    //}

    // Cache/generate initial origin + direction for the light/camera subpaths

    // Cache initial origin for the light/camera subpaths
    // Importance-sampled rays are emitted for random planets
    Figure star = figuresReadable[STELLAR_FIG_ID];
    float2x3 subpathOris = float2x3(cameraPos.xyz,
                                    LightSurfPos(LIGHT_ID_STELLAR,
                                                 star.linTransf,
                                                 FigDF(0.0f.xxx, // We only care about planet position here, so input an arbitrary ray location + origin and ignore figure boundaries
                                                       0.0f.xxx,
                                                       false,
                                                       figuresReadable[min((xorshiftPermu1D(randVal) % 10) + 1, 9)])[1].xyz,
                                                       randVal));

    // Transform [traceable]'s camera ray direction to match the view matrix, also generate + cache a separate
    // light ray direction
    float3 initLiNorml = normalize(subpathOris[1] - star.linTransf.xyz);
    float3 liDir = mul(LightEmitDir(LIGHT_ID_STELLAR,
                       randVal),
                       NormalSpace(initLiNorml));
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
    float2 initCamAngles = VecToAngles(rayDirs[0].xyz);
    bidirVts[0].camVt = BuildBidirVt(float4(subpathOris[0], LENS_FIG_ID), // Cache world-space sample position + camera figure-ID
                                     cameraPos.xyz, // Cache camera position
                                     float4(atten[0], BXDF_ID_NOMAT), // Cache baseline attenuations + null BXDF ID (no material properties
                                                                      // for lenses atm)
                                     float4(lensNormal, DF_TYPE_LENS), // Cache lens normal + lens distance-function type
                                     float4(initCamAngles,
                                            initCamAngles), // Assume equivalent input/output directions for subpath endpoints
                                     float3(CamAreaPDFOut(), // Output probability, i.e. chance of emitting a ray through [pixID]
                                            1.0f, // Filler input probability; depends on a defined importance/radiance source within the scene
                                                  // so left undefined for now
                                            SceneField(subpathOris[0],
                                                       0.0f.xxx,
                                                       false,
                                                       STELLAR_FIG_ID)[0].x)); // Planetary distance for the lens/pinhole position

    // Pre-fill the first vertex on the light sub-path with the baseline light sample
    // BXDF ID assumes that stars are 100% diffuse emitters
    // In/out ray directions model radiance emission as transmission from the light position through the lens/pinhole
    bidirVts[0].lightVt = BuildBidirVt(float4(subpathOris[1], STELLAR_FIG_ID),
                                       star.linTransf.xyz,
                                       float4(atten[1], BXDF_ID_DIFFU),
                                       float4(initLiNorml, DF_TYPE_STAR),
                                       float4(VecToAngles(liDir),
                                              VecToAngles(liDir)), // Assume equivalent input/output directions for subpath endpoints
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
                                                      FILLER_SCREEN_ID), // Filler filter ID
                                           SceneField(subpathVecs[1],
                                                      subpathOris[1],
                                                      true,
                                                      STELLAR_FIG_ID)); // We want to avoid light rays re-intersecting with the local star

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
                if (sceneField[0].y == STELLAR_FIG_ID) { subpathEnded.x = true; }

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
                    float2 incidAngles = VecToAngles(rayDirs[1].xyz);
                    bidirVts[numBounces.y + 1].lightVt = BuildBidirVt(float4(subpathVecs[1], LENS_FIG_ID),
                                                                      sceneField[3],
                                                                      float4(atten[1], BXDF_ID_NOMAT), // Assume no attenuation/material interactions at the lens
                                                                      float4(lensNormal, DF_TYPE_LENS), // Lens normal + lens distance-function type
                                                                      float4(incidAngles,
                                                                             incidAngles), // Assume equivalent input/output directions for subpath endpoints
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

    // Attempt to integrate the camera/light subpaths if appropriate; otherwise, assign
    // the estimated path color from the radiance at the intersected light source
    // (same result as attempting connection strategies for that context, much cheaper
    // than calling [ConnectBidirVts(...)]) or assign pixel color directly from the
    // ambient shading (no possible connections for rays that never intersected the
    // scene/the light source)

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

        // Bounce vertices are offset from baseline camera/ligh/t vertices, so total vertex count will always be
        // [numBounces + 1u.xx]
        for (int i = 0; i <= (int)numBounces.x + 1; i += 1)
        {
            // No reason to trace light/camera gather-rays at the same time (since the gather-rays + the sub-paths form complete
            // paths by themselves), so step over the s == t == 1 case here
            for (int j = 0; j <= (int)numBounces.y + 1; j += (1 + (i == 1 && j == 0)))
            {
                // This block implicitly computes every sampling strategy for every vertex in the current path
                // before weighing them with the power heuristic and accumulating (integrating) the result
                // (i.e. it applies Multiple Importance Sampling instead of naively accumulating the path vertices);
                // Naive bi-directional MIS is extremely expensive (averaging every light vertex, for every camera
                // vertex, within every light vertex, for every camera vertex), and the variant I'm using here is
                // from the square-complexity algorithm suggested by Pharr, Jakob, and Humphreys in
                // [Physically Based Rendering: From Theory to Implementation]

                // Attempt to connect the light/camera subpaths at the current light/camera indices
                float2 gatherPDFs; // Path integration sometimes gathers importance/radiance from outside either
                                   // subpath and implicitly processes an extra vertex at the intersection point;
                                   // this value carries the forward/reverse probabilities associated with that
                                   // vertex
                float3 gatherPos;
                //uint2 ndces = uint2(, );
                float4 bdVtData = ConnectBidirVts(bidirVts[max(i - 1, 0)].camVt,
                                                  bidirVts[max(j - 1, 0)].lightVt,
                                                  float4(i,
                                                         j,
                                                         length(bidirVts[max(i - 1, 0)].camVt.pos.xyz - bidirVts[max(max(i - 1, 0) - 1, 0)].camVt.pos.xyz),
                                                         length(bidirVts[max(j - 1, 0)].lightVt.pos.xyz - bidirVts[max(max(j - 1, 0) - 1, 0)].lightVt.pos.xyz)),
                                                  numBounces,
                                                  subpathEnded,
                                                  adaptEps,
                                                  subpathOris[0],
                                                  camInfo,
                                                  star.linTransf,
                                                  gatherPDFs,
                                                  gatherPos,
                                                  pixWidth,
                                                  linPixID,
                                                  randVal);

                // Generate a Multiple Importance Sampling (MIS) weight for the current combination
                // of light/camera subpaths
                //float bidirMIS = 1;
                //if (i + j != 2) // Retain the default MIS weight for connection strategies with exactly
                //                // two vertices (only one possible variation on those strategies, so it's
                //                // MIS weight (i.e. the probability of tracing that particular strategy
                //                // between the given vertices) is guaranteed to be [1])
                //{
                //    // Accumulate probability ratios for every possible connection strategy coincident with
                //    // the current light/camera vertices
                //
                //    // Declare a variable to carry the accumulated ratios of forward/reverse probabilities
                //    // across the whole *potential* path space for the generated light/camera subpaths;
                //    // declare another value to cache the
                //    float2 serPDFRatios = 0.0f.xx;
                //    float2 prodPDFRatios = 1.0f.xx;
                //
                //    // Accumulate probabilities over the light/camera subpaths
                //    int2 kInit = int2(max(i - 1, 0),
                //                      max(j - 1, 0));
                //    for (int2 k = kInit; any(k >= (int2)0x0.xx); k -= (int2)0x1.xx)
                //    {
                //        // Generate indices for vertices near the current connection strategy
                //        int3x2 vtIndices;
                //        vtIndices[0] = max(k, 0x0.xx);
                //        vtIndices[1] = max(k - 0x1.xx, 0x0.xx);
                //        vtIndices[2] = min(k + 0x1.xx, numBounces);
                //
                //        // Cache forward/reverse PDFs for the current vertex connection strategy
                //        float4 currPDFIO = float4(bidirVts[vtIndices[0].x].camVt.pdfIO.xy,
                //                                  bidirVts[vtIndices[0].y].lightVt.pdfIO.xy);
                //
                //        // Update probability values for the initial vertex (i.e. the current bi-directional
                //        // connection site) when gathers occur during the current connection strategy
                //        // Also convert non-camera PDFs from probabilities over solid angle to probabilities
                //        // over unit area (needed to guarantee consistency over entire subpaths; solid-angle
                //        // PDFs are strictly defined in terms of specific vertices, which makes the idea of
                //        // "cumulative" PDFs over different subpaths much harder to define)
                //        // (camera PDFs are defined over area by default)
                //        if ((i == 1 && j != 0) && k.x == kInit.x)
                //        {
                //            // Define [currPDFIO.xy] from [gatherPDFs] if a camera gather occurred during
                //            // the current connection strategy
                //            // Lens/pinhole PDFs are naturally defined over area, so avoid the angle-to-area
                //            // conversion here
                //            currPDFIO.zw = gatherPDFs;
                //        }
                //        else if ((j == 1 && i != 0) && k.y == kInit.y)
                //        {
                //            // Define [currPDFIO.zw] from [gatherPDFs] if a light gather occurred during
                //            // the current connection strategy
                //            // Also re-define the given PDF value over area
                //            currPDFIO.xy = gatherPDFs * AnglePDFsToArea(bidirVts[vtIndices[1].x].camVt.pos.xyz,
                //                                                        bidirVts[vtIndices[0].x].camVt.pos.xyz,
                //                                                        gatherPos,
                //                                                        float2x3(bidirVts[vtIndices[1].x].camVt.norml.xyz,
                //                                                                 0.0f.xxx),
                //                                                        bool2(bidirVts[vtIndices[1].x].camVt.atten.w == BXDF_ID_VOLUM,
                //                                                              false)); // No reason to attenuate by facing ratio for vertices on the light's surface
                //        }
                //        else if (all(k == kInit))
                //        {
                //            // Update reverse PDFs for the current connection vertices (i.e. the light/camera vertices
                //            // processed for the current connection strategy); these are neccessarily different to the PDFs
                //            // defined at [bidirVts[k.x].camVt.pdfIO.y] or [bidirVts[k.y].camVt.pdfIO.y] because those
                //            // only describe probabilities within each subpath, not probabilities within the complete
                //            // paths evaluated by each bi-directional connection strategy
                //
                //            // Pre-processing every generated path to prove sampleability is expensive, and we want to
                //            // avoid using (limited) GPU registers to cache sampleability per-vertex during ray
                //            // propagation; use a heuristic method to check sampleability here instead
                //            // Heuristic assumes that any path with an SDS (specular-diffuse-specular) interaction
                //            // would be unsampleable, and ignores point lights or pinhole cameras (since our lens +
                //            // light source (both distance functions) are defined over nonzero area and guaranteed
                //            // sampleable from all visible points in the scene)
                //            bool2 mollify = false.xx;
                //            bool2 nearDelta = bool2(isDeltaBXDF(bidirVts[vtIndices[0].y].lightVt.atten.w) &&
                //                                    isDeltaBXDF(bidirVts[vtIndices[1].y].lightVt.atten.w),
                //                                    isDeltaBXDF(bidirVts[vtIndices[0].x].camVt.atten.w) &&
                //                                    isDeltaBXDF(bidirVts[vtIndices[1].x].camVt.atten.w));
                //            mollify.x = nearDelta.x && bidirVts[vtIndices[0].y].camVt.atten.w == BXDF_ID_DIFFU; // Decide whether to mollify the camera path
                //            mollify.y = nearDelta.y && bidirVts[vtIndices[0].y].lightVt.atten.w == BXDF_ID_DIFFU; // Decide whether to mollify the light path
                //
                //            // Update reverse PDF for the current camera-vertex
                //            currPDFIO.y = BidirMISPDF(float2x4(bidirVts[k.y].lightVt.pos,
                //                                               bidirVts[k.y].lightVt.norml.xyz, bidirVts[k.y].lightVt.atten.w),
                //                                      float2x4(bidirVts[k.x].camVt.pos,
                //                                               bidirVts[k.x].camVt.norml.xyz, bidirVts[k.x].camVt.atten.w),
                //                                      float2x3(bidirVts[max(k.x - 1, 0)].camVt.pos.xyz - bidirVts[k.x].camVt.pos.xyz,
                //                                               lensNormal),
                //                                      float3(bidirVts[k.y].lightVt.pos.w,
                //                                             bidirVts[k.x].camVt.pos.w,
                //                                             pxPSRRadius),
                //                                      bool2(false, mollify.x));
                //
                //            // Update reverse PDF for the current light-vertex
                //            currPDFIO.w = BidirMISPDF(float2x4(bidirVts[k.x].camVt.pos,
                //                                               bidirVts[k.x].camVt.norml.xyz, bidirVts[k.y].camVt.atten.w),
                //                                      float2x4(bidirVts[k.y].lightVt.pos,
                //                                               bidirVts[k.y].lightVt.norml.xyz, bidirVts[k.x].lightVt.atten.w),
                //                                      float2x3(bidirVts[max(k.y - 1, 0)].lightVt.pos.xyz - bidirVts[k.y].lightVt.pos.xyz,
                //                                               lensNormal),
                //                                      float3(bidirVts[k.x].camVt.pos.w,
                //                                             bidirVts[k.y].lightVt.pos.w,
                //                                             pxPSRRadius),
                //                                      bool2(true, mollify.y));
                //        }
                //        //else if (k == (kInit - 0x1.xx))
                //        //{
                //            // Update reverse PDF for the previous camera-vertex in the complete
                //            // connection strategy
                //            //currPDFIO.y = BidirMISPDF(float2x4(bidirVts[k.y].lightVt.pos,
                //            //                                   bidirVts[k.y].lightVt.norml.xyz, bidirVts[k.y].lightVt.atten.w),
                //            //                          float2x4(bidirVts[k.x].camtVt.pos,
                //            //                                   bidirVts[k.x].camtVt.norml.xyz, bidirVts[k.x].camVt.atten.w),
                //            //                          float2x3(bidirVts[max(k.x - 1, 0)].camVt.pos.xyz - bidirVts[k.x].camVt.pos.xyz,
                //            //                                   lensNormal),
                //            //                          false);
//              //
                //            //// Update reverse PDF for the previous light-vertex in the complete
                //            //// connection strategy
                //            //currPDFIO.w = BidirMISPDF(float2x4(bidirVts[k.x].camVt.pos,
                //            //                                   bidirVts[k.x].camVt.norml.xyz, bidirVts[k.x].camVt.atten.w),
                //            //                          float2x4(bidirVts[k.y].lightVt.pos,
                //            //                                   bidirVts[k.y].lightVt.norml.xyz, bidirVts[k.x].lightVt.atten.w),
                //            //                          float2x3(bidirVts[max(k.y - 1, 0)].lightVt.pos.xyz - bidirVts[k.y].lightVt.pos.xyz,
                //            //                                   lensNormal),
                //            //                          true);
                //        //  }
                //        else if (bidirVts[k.x].camVt.norml.w == DF_TYPE_LENS)
                //        {
                //            // Convert non-lens camera vertex PDFs to probabilities over area
                //            currPDFIO.xy *= AnglePDFsToArea(bidirVts[vtIndices[1].x].camVt.pos.xyz,
                //                                            bidirVts[vtIndices[0].x].camVt.pos.xyz,
                //                                            bidirVts[vtIndices[2].x].camVt.pos.xyz,
                //                                            float2x3(bidirVts[vtIndices[1].x].camVt.norml.xyz,
                //                                                     bidirVts[vtIndices[2].x].camVt.norml.xyz),
                //                                            bool2(bidirVts[vtIndices[1].x].camVt.atten.w == BXDF_ID_VOLUM,
                //                                                  bidirVts[vtIndices[2].x].camVt.atten.w == BXDF_ID_VOLUM));
                //        }
                //        else if (!(bidirVts[k.y].lightVt.norml.w == DF_TYPE_LENS))
                //        {
                //            // Convert non-lens light-vertex PDFs into probabilities over area
                //            currPDFIO.zw *= AnglePDFsToArea(bidirVts[vtIndices[1].y].camVt.pos.xyz,
                //                                            bidirVts[vtIndices[0].y].camVt.pos.xyz,
                //                                            bidirVts[vtIndices[2].y].camVt.pos.xyz,
                //                                            float2x3(bidirVts[vtIndices[1].y].camVt.norml.xyz,
                //                                                     bidirVts[vtIndices[2].y].camVt.norml.xyz),
                //                                            bool2(bidirVts[vtIndices[1].y].camVt.atten.w != BXDF_ID_VOLUM,
                //                                                  bidirVts[vtIndices[2].y].camVt.atten.w != BXDF_ID_VOLUM));
                //        }
                //
                //        // Conditionally reduce forward/reverse PDFs to the identity if we've passed the
                //        // start of either subpath
                //        bool4 misMask = k.xxyy < (int4)0x0.xxxx;
                //        currPDFIO = (currPDFIO * !misMask) + misMask;
                //
                //        // Paths with specular vertices may produce zeroed PDFs; process those here to avoid singularities without
                //        // altering overall PDF ratios for the current connection strategy
                //        if (currPDFIO.x == 0.0f)
                //        {
                //            currPDFIO.x == currPDFIO.y;
                //            misMask.xy = true.xx; // Mask out non-samplable variations on the camera subpath
                //        }
                //        if (currPDFIO.z == 0.0f)
                //        {
                //            currPDFIO.z == currPDFIO.w;
                //            misMask.zw = true.xx; // Mask out non-samplable variations on the light subpath
                //        }
                //
                //        // Evaluate PDF ratios for the current light/camera vertices
                //        currPDFIO *= currPDFIO; // We're using the power heuristic, so square both probabilities
                //        prodPDFRatios *= float2(currPDFIO.y / currPDFIO.x,
                //                                currPDFIO.w / currPDFIO.z);
                //
                //        // Accumulate running products into the cumulative pdf-ratio
                //        // series, but only if they were generated for valid vertices
                //        // (i.e. ignore ratios from the light/camera subpaths when either
                //        // axis of [k] is below zero)
                //        serPDFRatios += prodPDFRatios * !misMask.xz;
                //    }
                //
                //    // Generate a bi-directional MIS weight with the power heuristic,
                //    // then store the result in [bidirMIS]
                //    // ...also account for our parallel MIS loop by merging either axis
                //    // of the pdf-ratio series here
                //    bidirMIS = 1.0f / (1.0f + serPDFRatios.x + serPDFRatios.y);
                //}

                // Apply the generated MIS weight to the color value associated with the
                // current bi-directional connection strategy
                //bdVtData.rgb *= bidirMIS;

                // Accumulate radiance over most connection strategies; pass radiance
                // received from [t == 1] rays (i.e. gather rays emitted from the scene
                // towards arbitrary sensors) into a translation buffer for integration
                // in the soonest possible frame
                if (bdVtData.w == (int)linPixID)
                {
                    pathRGB += bdVtData.rgb; // Comment this out to accurately test [t == 1] connection strategies
                }
                else
                {
                    // The light subpath bounced off a surface and hit a different sensor to the one
                    // associated with this thread; pass [bdVtData] into the AA buffer instead so
                    // it can be integrated for the relevant pixel as soon as possible
                    aaBuffer[(int)bdVtData.w].incidentLight.rgb = bdVtData.rgb;
                }
            }
        }
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
