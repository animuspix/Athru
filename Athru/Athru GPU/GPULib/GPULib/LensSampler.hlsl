
#include "AA.hlsli"
#ifndef RENDER_BINDS_LINKED
	#include "RenderBinds.hlsli"
#endif
#include "PhiloInit.hlsli"

// Per-bounce indirect dispatch axes
// (tracing axes in the zeroth channel, sampling axes in (1...6))
RWBuffer<uint> dispAxes : register(u12);

// Append/consume counters for traceables + material primitives
RWBuffer<uint> traceCtr : register(u13);
RWBuffer<uint> diffuCtr : register(u14);
RWBuffer<uint> mirroCtr : register(u15);
RWBuffer<uint> refraCtr : register(u16);
RWBuffer<uint> snowwCtr : register(u17);
RWBuffer<uint> ssurfCtr : register(u18);
RWBuffer<uint> furryCtr : register(u19);

// Buffer of marcheable/traceable rays processed by [RayMarch.hlsl]
AppendStructuredBuffer<uint> traceables : register(u20);

// Lens sampling occurs with tiled pixel positions, so include the pixel/tile-mapper
// here
#include "TileMapper.hlsli"

// Ray direction for a pinhole camera (simple tangent-driven
// pixel projection), given a pixel ID + a two-dimensional
// resolution vector + a super-sampling scaling factor
// Allows for ray projection without the
// filtering/jitter applied by [PixToRay(...)]
#define FOV_RADS 0.5 * PI
float3 PRayDir(uint2 pixID,
               uint pixWidth)
{
    float2 viewSizes = gpuInfo.resInfo.xy * pixWidth;
    float4 dir = float4(normalize(float3(pixID - (viewSizes / 2.0f),
                                         viewSizes.y / tan(FOV_RADS / 2.0f))), 1.0f);
    return mul(dir, gpuInfo.viewMat).xyz;
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
    // blue-noise decorrelation

    // Lock the sample index to the interval [0...NUM_AA_SAMPLES]
    pixSampleNdx %= gpuInfo.resInfo.z;

    // Convert the sample index into a super-sampled pixel coordinate
    // using Martin Roberts' R^2 map, described here:
    // http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
    float pixWidth = sqrt(gpuInfo.resInfo.z);
    const float g = 1.6180339887498948482f;
    const float a1 = 1.0f / g;
    const float a2 = 1.0f / (g * g);
    float2 sampleXY = float2(0.5f + a1 * pixSampleNdx,
                             0.5f + a2 * pixSampleNdx) % 1.0f;
    sampleXY = (sampleXY - 0.5f.xx) * pixWidth;

    // Decorrelate selected coordinates
    float jitterRange = pixWidth;
    sampleXY += (uv01 * jitterRange) - (jitterRange * 0.5f).xx;

    // Restrict samples to the domain of the current pixel
    sampleXY %= pixWidth.xx;

    // Generate + return a ray direction for the super-sampled pixel coordinate
    // Also generate a filter coefficient for the current sample
    // Filter coefficient determines how samples will be blended into final pixel
    // values for post-processing
    uint2 baseSSPixID = (pixID * pixWidth) + ((uint)pixWidth * 0.5f).xx;
    return float4(PRayDir(baseSSPixID + sampleXY,
                          pixWidth),
                  BlackmanHarris(sampleXY,
                                 length((pixWidth).xx),
                                 gpuInfo.resInfo.z));
}

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Extract a pixel ID from the given thread/group IDs
    uint2 tileID = uint2((groupID.x * TRACING_GROUP_WIDTH) + (threadID % TRACING_GROUP_WIDTH),
                         (groupID.y * TRACING_GROUP_WIDTH) + (threadID / TRACING_GROUP_WIDTH));
    uint linTileID = tileID.x + (tileID.y * gpuInfo.tilingInfo.x);
    if (linTileID > (gpuInfo.tilingInfo.z - 1)) { return; }  // Mask off excess threads

    // Select a pixel from within the current tile
    // Try to cover the whole tile as efficiently as possible
    uint frameCtr = uint(gpuInfo.tInfo.z);
    uint3 displayTexel = TilePx(tileID,
                                frameCtr,
                                gpuInfo.resInfo.x,
                                gpuInfo.tileInfo.xy);

    // Generate an outgoing ray-direction for the current pixel
    // + a corresponding filter value
    float4 pRay = PixToRay(displayTexel.yz,
                           aaBuffer[displayTexel.x].sampleCount.x + 1,
                           ditherTex[displayTexel.yz % uint2(128, 128)]);

    // Prepare zeroth bounce for the core tracing/intersection shader
    rays[displayTexel.x][0] = gpuInfo.cameraPos.xyz;
    rays[displayTexel.x][1] = pRay.xyz;
    dispAxes[6] = gpuInfo.tilingInfo.z;
    traceables.Append(displayTexel.x);

    // Initialize path values for the current pixel
    displayTex[displayTexel.yz] = float4(1.0f.xxx,
                                         pRay.w);

    // Initialize denoiser metadata for the current pixel
    posBuffer[displayTexel.x] = 16384.0f.xxx; // Default points in the position/normal-buffers to infinity
	nrmBuffer[displayTexel.x] = 16384.0f.xxx;

    // Initialize per-path Philox streams on the zeroth sample for each pixel
    if (frameCtr < 4u)
    {
        PhiloStrm randStrm;
        strmBuilder(displayTexel.x, randStrm.ctr, randStrm.key);
        randBuf[displayTexel.x] = randStrm;
    }
}