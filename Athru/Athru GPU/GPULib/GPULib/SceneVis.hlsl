
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
    uint dispWidth = (uint)timeDispInfo.w;
    uint linGroupID = (groupID.x + groupID.y * dispWidth) +
                      (groupID.z * (dispWidth * dispWidth));
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

    // Cache/generate initial origin + direction for the light/camera subpaths

    // Cache initial origin for the light/camera subpaths
    // Importance-sampled rays are emitted for random planets
    Figure star = figuresReadable[STELLAR_FIG_ID];

    // Define an adaptive epsilon (marching cutoff) for optimal
    // marching performance (no reason to march to within a hundred-thousandth of
    // a surface when the camera is 150 units away anyways)
    float adaptEps = EPSILON_MIN;

    // Pre-fill the first vertex on the camera sub-path with the baseline camera sample
    // No figures or surfaces at the camera pinhole/lens, so fill those with placeholder values
    // (11 => only ever 10 figures in the scene; 5 => only four defined BXDFs)
    float3 lensNormal = mul(float4(0.0f, 0.0f, 1.0f, 1.0f), viewMat).xyz;
    float4 camInfo = float4(lensNormal, traceable[1].x);
    float pixWidth = sqrt(NUM_AA_SAMPLES);

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
    // Also cache a scaled version of the local pixel ID for easy use with
    // [camVts] and [liVts]
    uint linPixID = traceable[1].y;
    uint scaledLinPixID = linPixID * maxNumBounces.x;
    float3 pathRGB = aaBuffer[linPixID].incidentLight.rgb;

    // We want to avoid accumulating incidental radiance separately to per-pixel samples, so zero
    // [aaBuffer[linPixID].incidentLight.rgb] after caching its value in [incidentIllum]
    aaBuffer[linPixID].incidentLight.rgb = 0.0f.xxx;

    displayTex[pixID] = float4(0.5f.xxx,//PathPost(float3(iToFloat(xorshiftPermu1D(randVal)),
                               //                iToFloat(xorshiftPermu1D(randVal)),
                               //                iToFloat(xorshiftPermu1D(randVal))),
                               //         traceable[1].x,
                               //         linPixID),
                               1.0f);
    randBuf[randNdx] = randVal;
    return;

    // Extract bounce-count + status for each local subpath
    uint2 bounceCtr = 0x0.xx;
    bool4 subpathStatus = false.xxxx; // Endedness/escapedness in [xy] and [zw]
    for (uint i = 0; i < maxNumBounces.x; i += 1)
    {
        float2 currVtDists = float2(camVts[scaledLinPixID + i].pdfIO.z,
                                    camVts[scaledLinPixID + i].pdfIO.z);
        subpathStatus = bool4(currVtDists.x < 0.0f, // Check for endedness on the camera subpath
                              currVtDists.x >= 65536.0f, // Check for escapedness on the camera subpath
                              currVtDists.y < 0.0f, // Check for endedness on the light subpath
                              currVtDists.y >= 65536.0f) // Check for endedness on the camera subpath
                              || subpathStatus; // Endedness/escapedness on any vertex is permanent
        bounceCtr += bool2(!subpathStatus.x && !subpathStatus.y,
                           !subpathStatus.z && !subpathStatus.w); // Only accumulate bounces for non-ended, non-escaped subpaths
        if ((subpathStatus.x || subpathStatus.y) &&
            (subpathStatus.z || subpathStatus.w)) { break; } // Break out early if both subpaths have ended/escaped
    }

    // We've collected + reset incident light for the current sensor/pixel, so now we can begin
    // integrating through the camera/light subpaths (if appropriate)
    // Avoid connecting the camera/light subpaths if the initial camera ray never intersects the scene
    if (bounceCtr.x > 0x0)
    {
        // We use path-space regularization (PSR) to blur delta-distributed BRDFs in the light/camera subpaths;
        // PSR's operating bandwidth (=> blur radius) is dependant on per-pixel sample count and can't be
        // known ahead of time, so evaluate it here instead
        float pxPSRRadius = basePSRRadius * pow(aaBuffer[traceable[1].x].sampleCount.x, psrLam);

        // Evaluate the camera-only path through the scene (only if the camera-subpath intersects with
        // the local star)
        #ifdef TRACE_MODE_REVERSE
            if (subpathStatus.x)
            {
                // Camera path radiance is taken as the illumination at the second-last camera vertex in the scene adjusted for attenuation
                // at the final vertex
                PathVt lastCamVt = camVts[scaledLinPixID + bounceCtr.x + 1];
                PathVt secndLastCamVt = camVts[scaledLinPixID + bounceCtr.x];
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
                if (!subpathStatus.x)
                {
                    pathRGB += LiGather(camVts[(linPixID * maxNumBounces.x) + bounceCtr.x],
                                        star.linTransf, // Only stellar light sources atm
                                        adaptEps,
                                        randVal);
                }
            #endif
        #endif

        // Evaluate the light-only path through the scene (only if the light-subpath intersects with the lens)
        #ifdef TRACE_MODE_FORWARD
            if (subpathStatus.z)
            {
                // Camera path radiance is taken as the illumination at the second-last camera vertex in the scene adjusted for attenuation
                // at the final vertex
                PathVt lastLiVt = bidirVts[numBounces.y + 1].lightVt;
                PathVt secndLastLiVt = bidirVts[numBounces.y].lightVt;
                //pathRGB = float3(1.0f, 0.0f.xx);
                aaBuffer[(int)PRayPix(normalize(cameraPos.xyz - lastLiVt.pos.xyz),
                                                camInfo.w).z].incidentLight.rgb += Emission(STELLAR_RGB, STELLAR_BRIGHTNESS, length(secndLastLiVt.pos.xyz - lastLiVt.pos.xyz)) * lastLiVt.atten.rgb;
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
                if (!subpathStatus.w)
                {
                    float4 gathInfo = CamGather(bidirVts[numBounces.y].lightVt,
                                                camInfo,
                                                adaptEps,
                                                randVal);
                    aaBuffer[(int)gathInfo.w].incidentLight.rgb = /*gathInfo.rgb */float3(1.0f, 0.0f.xx);
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
