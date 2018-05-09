
#include "Lighting.hlsli"
#include "ScenePost.hlsli"
#include "TraceableIDs.hlsli"
#ifndef RASTER_CAMERA_LINKED
    #include "RasterCamera.hlsli"
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
                         figuresReadable[i]).x;
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
    uint groupWidth3D = ceil(pow(numPathPatches.y, 1.0f / 3.0f));
    uint linGroupID = (groupID.x + groupID.y * groupWidth3D) +
                      (groupID.z * (groupWidth3D * groupWidth3D));
    uint linDispID = (linGroupID * 128) + (threadID + 1);
    if (linDispID > traceableCtr.x) { return; }

    // [Consume()] a pixel from the end of [traceables]
    float2x3 traceable = traceables.Consume();

    // Separately cache the pixel's screen position for later reference
    uint2 pixID = uint2(traceable[1].y % DISPLAY_WIDTH,
                        traceable[1].y / DISPLAY_WIDTH);

    // Extract a Xorshift-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    uint randNdx = xorshiftNdx(pixID.x + (pixID.y * DISPLAY_WIDTH));
    uint randVal = randBuf[randNdx];

    // Cache/generate initial origin + direction for the light/camera subpaths

    // Cache initial origin for the light/camera subpaths
    // Only emitting importance-sampled rays for the zeroth planet atm
    Figure star = figuresReadable[STELLAR_NDX];
    Figure initPlanetShading = figuresReadable[1];
    float cosMaxEmitAngles = cos(EmissiveAnglesPerPlanet(initPlanetShading.scaleFactor.x,
                                                         star.scaleFactor.x));
    float2x3 subpathOris = float2x3(cameraPos.xyz,
                                    AltStellarSurfPos(star.scaleFactor.x, star.pos.xyz, float3(iToFloat(xorshiftPermu1D(randVal)),
                                                                                               iToFloat(xorshiftPermu1D(randVal)),
                                                                                               iToFloat(xorshiftPermu1D(randVal)))));
                                    //StellarSurfPos(star.scaleFactor.x,
                                    //               star.pos.xyz,
                                    //               initPlanetShading.pos.xyz,
                                    //               cosMaxEmitAngles,
                                    //               float2(frand1D(pixID.x * threadID),
                                    //                      frand1D(pixID.y * rendPassID.x))));

    // Transform [traceable]'s camera ray direction to match the view matrix, also generate + cache a separate
    // light ray direction
    float3 localStarNormal = normalize(subpathOris[1] - star.pos.xyz);
    float2x3 rayDirs = float2x3(traceable[0],
                                localStarNormal);

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
    // and local radiance divided by the probability of selecting the chosen light sample (for the light source)
    float2x3 atten = float2x3(1.0f.xxx,
                              Emission(star) * StellarPosPDF());

    // Paired array of camera/light sub-path vertices within the scene; needed for flexible path generation in BDPT
    BidirVtPair bidirVts[MAX_BOUNCES_PER_SUBPATH + 1]; // One extra vertex pair to store the camera's pinhole + the sampled position
                                                       // on the light source

    // Pre-fill the first vertex on the camera sub-path with the baseline camera sample
    // No figures or surfaces at the camera pinhole/lens, so fill those with placeholder values
    // (11 => only ever 10 figures in the scene; 5 => only four defined BXDFs)
    float3 lensNormal = mul(float4(0.0f, 0.0f, 1.0f, 1.0f), viewMat).xyz;
    float camRayPDFArea = RayImportance(rayDirs[0],
                                        lensNormal).z; // Not sure about how I designed this at all...will need to double-check PBR
    // In/out ray directions model importance emission as transmission from the camera position through the lens/pinhole
    bidirVts[0].camVt = BuildBidirVt(subpathOris[0], // Cache world-space sample position
                                     atten[0], // Cache baseline attenuation
                                     lensNormal,
                                     11, // Filler figure ID
                                     5); // Filler BXDF ID

    // Pre-fill the first vertex on the light sub-path with the baseline light sample
    // BXDF ID assumes that stars are 100% diffuse emitters
    // In/out ray directions model radiance emission as transmission from the light position through the lens/pinhole
    bidirVts[0].lightVt = BuildBidirVt(subpathOris[1],
                                       atten[1],
                                       localStarNormal,
                                       STELLAR_NDX,
                                       BXDF_ID_DIFFUSE);

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
            float4 sceneField = float4(SceneField(subpathVecs[0],
                                                  subpathOris[0],
                                                  true,
                                                  11), // Filler filter ID
                                       SceneField(subpathVecs[1],
                                                  subpathOris[1],
                                                  true,
                                                  STELLAR_NDX)); // We want to avoid light rays re-intersecting with the local star

            // Process camera-ray intersections if appropriate
            if (sceneField.x < adaptEps &&
                rayActiVec.x)
            {
                // Process the current intersection, then return + store a BDPT-friendly surface interaction
                // Storage at [k + 1] because the zeroth index in either sub-path is reserved for the initial
                // light/camera sample in the path
                bidirVts[numBounces.x + 1].camVt = ProcVert(subpathVecs[0],
                                                            rayDirs[0],
                                                            adaptEps,
                                                            sceneField.y,
                                                            numBounces.y,
                                                            false,
                                                            bidirVts[numBounces.x].camVt.pos.xyz,
                                                            atten[0],
                                                            subpathOris[0],
                                                            rayDistVec.x,
                                                            randVal);

                // Ignore the current intersection while the light-ray marches through the scene
                rayActiVec.x = false;
            }

            // Process light-ray intersections if appropriate
            // (+ test for lens intersection)
            float lensDist = LensDF(subpathVecs[1], adaptEps);
            bool lightIsect = (sceneField.z < adaptEps ||
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
                    bidirVts[numBounces.y + 1].lightVt = ProcVert(subpathVecs[1],
                                                                  rayDirs[1],
                                                                  adaptEps,
                                                                  sceneField.w,
                                                                  numBounces.x,
                                                                  true,
                                                                  bidirVts[numBounces.y].lightVt.pos.xyz,
                                                                  atten[1],
                                                                  subpathOris[1],
                                                                  rayDistVec.y,
                                                                  randVal);
                }
                else
                {
                    // We intersected the lens, so cache the intersection position and flag the end of the light-subpath
                    // (since we aren't expecting light to bounce back out of the camera)
                    bidirVts[numBounces.y + 1].lightVt = BuildBidirVt(subpathVecs[1],
                                                                      float3(1.0f, 0.0f.xx), // Filler surface normal
                                                                      atten[1], // Assume no attenuation at the lens
                                                                      11, // Filler figure ID
                                                                      5); // Filler BXDF ID
                    subpathEnded[1] = true;
                }
                // Ignore the current intersection while the camera-ray marches through the scene
                rayActiVec.y = false;
            }

            // Increase ray distances by per-path field distance (take the largest steps possible
            // without passing through a surface boundary)
            rayDistVec += sceneField.xz;

            // Break out if the current ray distance passes the threshold defined by [maxRayDist]
            // Subtraction by epsilon is unneccessary and just provides consistency with the limiting
            // epsilon value used for intersection checks
            subpathEscaped = (rayDistVec > (MAX_RAY_DIST - adaptEps).xx);

            // Ignore escaped rays while contained rays marche through the scene
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
        // Bounce vertices are offset from baseline camera/light vertices, so total vertex count will always be
        // [numBounces + 1u.xx]
        for (int i = 0; i <= (int)numBounces.x + 1; i += 1)
        {
            // No reason to trace light/camera gather-rays at the same time (since the gather-rays + the sub-paths form complete
            // paths by themselves), so step over the s == t == 1 case here
            for (int j = 0; j <= (int)numBounces.y + 1; j += (1 + (i == 1 && j == 1)))
            {
                // Extract indices for the light/camera sub-paths
                int camNdx = max(i - 1, 0);
                int lightNdx = max(j - 1, 0);

                // Evaluate input/output directions for the current light/camera indices

                // Evaluate baseline vertex indices
                // Cameras / lights are assumed to emit/receive importance/radiance along
                // single initial/terminal rays
                uint2x4 ioNdices = uint2x4(uint4(camNdx, max(camNdx - 1, 0),
                                                 camNdx, min(camNdx + 1, numBounces.x)),
                                           uint4(lightNdx, max(lightNdx - 1, 0),
                                                 lightNdx, min(lightNdx + 1, numBounces.y)));

                // Convert the generated indices into useful vertex directions
                float2x3 camIODirs = float2x3(normalize(bidirVts[ioNdices[0].x].camVt.pos.xyz -
                                                        bidirVts[ioNdices[0].y].camVt.pos.xyz),
                                              normalize(bidirVts[ioNdices[0].z].camVt.pos.xyz -
                                                        bidirVts[ioNdices[0].w].camVt.pos.xyz));
                float2x3 lightIODirs = float2x3(normalize(bidirVts[ioNdices[1].x].lightVt.pos.xyz -
                                                          bidirVts[ioNdices[1].y].lightVt.pos.xyz),
                                                normalize(bidirVts[ioNdices[1].z].lightVt.pos.xyz -
                                                          bidirVts[ioNdices[1].w].lightVt.pos.xyz));

                // Attempt to connect the light/camera subpaths at the current light/camera indices; also
                // pass the generated input/output directions along to the connection function
                float4 bdVtData = ConnectBidirVts(bidirVts[camNdx].camVt,
                                                  bidirVts[lightNdx].lightVt,
                                                  camIODirs,
                                                  lightIODirs,
                                                  i,
                                                  j,
                                                  numBounces,
                                                  subpathEnded,
                                                  star,
                                                  cosMaxEmitAngles,
                                                  initPlanetShading.pos.xyz,
                                                  adaptEps,
                                                  subpathOris[0],
                                                  lensNormal,
                                                  traceable[1].z,
                                                  linPixID,
                                                  randVal);

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
