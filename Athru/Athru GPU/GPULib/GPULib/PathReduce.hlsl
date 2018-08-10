
// A two-dimensional texture representing the display; used
// as the image source during core rendering (required because
// there's no support for direct render-target access in
// DirectX)
RWTexture2D<float4> displayTex : register(u1);

// Buffer carrying known-intersecting pixels, primary-ray directions,
// primary-ray filter values (all camera rays only), and camera
// z-scale (i.e. distance from the camera position to an imaginary
// plane carrying every initial camera ray before normalization; needed
// for reprojecting incoming light samples in [SceneVis] so they can be
// properly mapped to pixel IDs)
// Ray directions are in [0][xyz], filter values are in [1][x],
// pixel indices are in [1][y], z-scale is in [1][z]
AppendStructuredBuffer<float2x3> traceables : register(u4);

// Small counter buffer describing the maximum number of traceable elements
// emitted by each frame (traceable element-count can be randomly reduced
// by stochastic path reduction)
AppendStructuredBuffer<uint> maxTraceables : register(u5);

#include "Lighting.hlsli"
#ifndef RASTER_CAMERA_LINKED
    #include "RasterCamera.hlsli"
#endif
#include "ScenePost.hlsli"

// #define controlling stochastic work reduction; slightly boosts performance and maintains framerate
// through especially dense scenes, but slightly increases AA render time (no filtering for skipped
// pixels)
#define SWR_ENABLED

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Only perform pre-processing/path-reduction when the append-consume buffer is empty
    if (timeDispInfo.z != 0) { return; }

    // Extract a pixel ID from the given thread/group IDs
    uint2 pixID = uint2((groupID.x * TRACING_GROUP_WIDTH) + (threadID % TRACING_GROUP_WIDTH),
                        (groupID.y * TRACING_GROUP_WIDTH) + (threadID / TRACING_GROUP_WIDTH));
    uint linPixID = pixID.x + (pixID.y * DISPLAY_WIDTH);

    // Extract a permutable value from [randBuf]
    // (needed for ray jitter)
    // Same domain as [SceneVis] so no reason to use different
    // indices
    uint randNdx = xorshiftNdx(linPixID);
    uint randVal = randBuf[randNdx];

    // Generate an outgoing ray-direction for the current pixel
    // Also cache the current ray's filter value + z-scale
    float zProj;
    float4 rayDir = PixToRay(pixID,
                             aaBuffer[linPixID].sampleCount.x + 1,
                             zProj,
                             randVal);

    // We're only using random values for jitter here, so write
    // [randVal] back into [randBuf]
    randBuf[randNdx] = randVal;

    // Test intersections for [rayDir] against every figure in the scene
    // except the local star
    // Loops are inefficient as heck, but we can ask FXC to unroll them and
    // hold onto the code readability :)
    bool isectPx = false;
    float3 camPos = cameraPos.xyz;
    float3 rayPt = rayDir.xyz + camPos;
    [unroll]
    for (int i = 1; i < MAX_NUM_FIGURES; i += 1)
    {
        // Pixels count as "intersecting the scene" if their camera rays
        // intersect at least one surface that isn't the local star
        isectPx = BoundingSurfTrace(figuresReadable[i].linTransf,
                                    figuresReadable[i].self.x,
                                    rayPt,
                                    camPos) || isectPx;
    }

    // Append ray-tracing data for [this] to [traceables], but
    // only if [pixIn] intersects at least one non-star figure
    // in the scene
    // Otherwise shade [displayTex] directly
    float3 rgb = 1.0f.xxx;
    if (isectPx)
    {
        #ifdef SWR_ENABLED
            // Difficult to validate [cullFreq] by hand, test with shadertoy...
            const float maxCullThresh = 0.8f; // Never cull more than 80% of traceable pixels
            float cullFreq = prevNumTraceables.x / (float)DISPLAY_AREA; // Match per-frame culling frequency to relative screen density (number of traceable elements/pixel)
            if (iToFloat(xorshiftPermu1D(randVal)) > min(cullFreq, maxCullThresh))
            {
                traceables.Append(float2x3(rayDir.xyz,
                                           float3(rayDir.w,
                                                  linPixID,
                                                  zProj)));
            }
        #else
            traceables.Append(float2x3(rayDir.xyz,
                                       float3(rayDir.w,
                                              linPixID,
                                              zProj)));
        #endif

        // Update the upper-value ray-intersection counter (ignoring SWR)
        const uint marker = 0xFFFFFF;
        maxTraceables.Append(marker);

        // Either we're recycling older color values for these points (SWR elision) or we're waiting until path-tracing to
        // have valid tonemapped radiances; either way, we want to return immediately instead of outputting a processed
        // stellar/background color
        return;
    }
    else if (BoundingSurfTrace(figuresReadable[STELLAR_FIG_ID].linTransf,
                               figuresReadable[STELLAR_FIG_ID].self.x,
                               rayPt,
                               camPos))
    {
        // Test pixels that didn't intersect planets/plants/animals
        // against the local star; shade any that *do* intersect
        // the star according to its emissivity
        // Lazy tonemapping to improve filtering here, will use smth more elegant
        // later...
        rgb = max(Emission(STELLAR_RGB, STELLAR_BRIGHTNESS,
                           length(figuresReadable[STELLAR_FIG_ID].linTransf.xyz - camPos) - figuresReadable[STELLAR_FIG_ID].linTransf.w), 8.0f.xxx);
    }
    else
    {
        // Non-intersecting pixels should be shaded with ambient
        // background light; do that here
        rgb = AMB_RGB;
    }
    displayTex[pixID] = float4(PathPost(rgb,
                                        rayDir.w,
                                        linPixID), 1.0f);
}