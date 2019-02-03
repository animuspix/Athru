
#include "ScenePost.hlsli"
#include "RenderUtility.hlsli"

// Buffer of marcheable/traceable rays processed by [RayMarch.hlsl]
AppendStructuredBuffer<LiBounce> traceables : register(u3);

// Buffer carrying intersections between ray-march iterations
// Basically a staging buffer to allow separate intersection tests
// and next-event-estimation/sampling
// Not referenced for lens sampling, but needed during ray-marching
// (+ it's easier to set up everything once before issuing the lens-sample
// dispatch than to bind new resources near each rendering call)
AppendStructuredBuffer<LiBounce> surfIsections : register(u6);

// Buffer carrying samples to rasterize for the current frame
AppendStructuredBuffer<uint2> rasterPx : register(u7);

// Ray direction for a pinhole camera (simple tangent-driven
// pixel projection), given a pixel ID + a two-dimensional
// resolution vector + a super-sampling scaling factor
// Allows for ray projection without the
// filtering/jitter applied by [PixToRay(...)]
#define FOV_RADS 0.5 * PI
float3 PRayDir(uint2 pixID,
               uint pixWidth)
{
    float2 viewSizes = resInfo.xy * pixWidth;
    float4 dir = float4(normalize(float3(pixID - (viewSizes / 2.0f),
                                         viewSizes.y / tan(FOV_RADS / 2.0f))), 1.0f);
    return mul(dir, viewMat).xyz;
}

// Initial ray direction + filter value
// Assumes a pinhole camera (all camera rays are emitted through the
// sensors from a point-like source)
// Direction returns to [xyz], filter coefficient passes into [w]
float4 PixToRay(uint2 pixID,
                uint pixSampleNdx,
                float2 uv01)
{
    // Ordinary pixel projection (direction calculated as the
    // path taken by a ray leaving a point and approaching the
    // given pixel grid-cell), with temporal super-sampling +
    // ray-jitter applied for basic anti-aliasing

    // Lock the sample index to the interval [0...NUM_AA_SAMPLES]
    pixSampleNdx %= resInfo.z;

    // Convert the sample index into a super-sampled pixel coordinate
    float pixWidth = sqrt(resInfo.z);
    uint2 baseSSPixID = (pixID * pixWidth) + ((uint)pixWidth / 2.0f).xx;
    float2 sampleXY = uint2((float)pixSampleNdx % pixWidth,
                            (float)pixSampleNdx / pixWidth);

    // Jitter the selected coordinate
    float jitterRange = pixWidth;
    sampleXY += (uv01 * jitterRange) - (jitterRange / 2.0f).xx;

    // Restrict samples to the domain of the current pixel
    sampleXY %= pixWidth.xx;

    // Generate + return a ray direction for the super-sampled pixel coordinate
    // Also generate a filter coefficient for the current sample
    // Filter coefficient determines how samples will be blended into final pixel
    // values for post-processing
    return float4(PRayDir(baseSSPixID + sampleXY,
                          pixWidth),
                  BlackmanHarris(sampleXY,
                                 length((pixWidth).xx),
                                 resInfo.z));
}

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Extract a pixel ID from the given thread/group IDs
    uint2 pixID = uint2((groupID.x * TRACING_GROUP_WIDTH) + (threadID % TRACING_GROUP_WIDTH),
                        (groupID.y * TRACING_GROUP_WIDTH) + (threadID / TRACING_GROUP_WIDTH));
    uint linPixID = pixID.x + (pixID.y * tilingInfo.x);
    // Mask off excess threads
    if (linPixID > (tilingInfo.z - 1)) { return; }

    // Select a pixel from within the current tile
    // Try to cover the whole tile as efficiently as possible
    uint frameCtr = uint(tInfo.z);
    uint2 px = uint2(frameCtr % 2,
                     (frameCtr % 3) / 2) + // Compute offset
                     (pixID * tileInfo.x); // Sum into the scaled tile origin
    uint linPx = px.x + px.y * resInfo.x;

    // Extract per-path Philox streams from [randBuf]
    PhiloStrm randStrm = philoxVal(linPx,
                                   frameCtr);
    float4 rand = iToFloatV(philoxPermu(randStrm));

    // Generate an outgoing ray-direction for the current pixel
    // + a corresponding filter value
    float4 pRay = PixToRay(px,
                           aaBuffer[linPx].sampleCount.x + 1,
                           rand.xy);

    // Prepare zeroth "bounce" for the core tracing/intersection shader
    if (linPixID == 0)
    {
        counters[18] = tilingInfo.z; // Update generic dispatch size
        // We're pushing new dispatch sizes, so update assumed threads/group here
        // ([1] because dispatch sizes are incremented naively and scaled down
        // afterwards)
        counters[21] = 1;
        // Zero bounce counter
        counters[22] = 0;
    }
    // Could append indices here instead of full bounces (smaller memory footprint)
    traceables.Append(LiBounceInitter(uint3(linPx, px), pRay.xyz, cameraPos.xyz, randStrm));

    // Cache pixel indices for rasterization
    rasterPx.Append(px);

    // Update PRNG state
    randBuf[linPx] = randStrm;

    // Initialize path values for the current pixel
    displayTex[px] = float4(1.0f.xxx, pRay.w);
}