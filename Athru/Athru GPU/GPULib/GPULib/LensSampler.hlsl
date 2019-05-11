
#include "ScenePost.hlsli"
#include "RenderBinds.hlsli"
#include "PhiloInit.hlsli"

// Buffer of marcheable/traceable rays processed by [RayMarch.hlsl]
AppendStructuredBuffer<uint> traceables : register(u11);

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
    float2 viewSizes = rndrInfo.resInfo.xy * pixWidth;
    float4 dir = float4(normalize(float3(pixID - (viewSizes / 2.0f),
                                         viewSizes.y / tan(FOV_RADS / 2.0f))), 1.0f);
    return mul(dir, rndrInfo.viewMat).xyz;
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
    pixSampleNdx %= rndrInfo.resInfo.z;

    // Convert the sample index into a super-sampled pixel coordinate
    float pixWidth = sqrt(rndrInfo.resInfo.z);
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
                                 rndrInfo.resInfo.z));
}

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Extract a pixel ID from the given thread/group IDs
    uint2 tileID = uint2((groupID.x * TRACING_GROUP_WIDTH) + (threadID % TRACING_GROUP_WIDTH),
                         (groupID.y * TRACING_GROUP_WIDTH) + (threadID / TRACING_GROUP_WIDTH));
    uint linTileID = tileID.x + (tileID.y * rndrInfo.tilingInfo.x);

    // Mask off excess threads
    if (linTileID > (rndrInfo.tilingInfo.z - 1)) { return; }

    // Select a pixel from within the current tile
    // Try to cover the whole tile as efficiently as possible
    uint frameCtr = uint(gpuInfo.tInfo.z);
    uint3 tilePx = TilePx(tileID,
                          frameCtr,
                          rndrInfo.resInfo.x,
                          rndrInfo.tileInfo.xy);

    // Extract per-path Philox streams from [randBuf]
    PhiloStrm randStrm;
    if (frameCtr < 4)
    { strmBuilder(tilePx.x, randStrm.ctr, randStrm.key); }
    else
    { randStrm = randBuf[tilePx.x]; }
    float4 rand = iToFloatV(philoxPermu(randStrm));

    // Generate an outgoing ray-direction for the current pixel
    // + a corresponding filter value
    float4 pRay = PixToRay(tilePx.yz,
                           aaBuffer[tilePx.x].sampleCount.x + 1,
                           rand.xy);

    // Prepare zeroth "bounce" for the core tracing/intersection shader
    rays[tilePx.x][0] = pRay.xyz;
    rays[tilePx.x][1] = rndrInfo.cameraPos.xyz;
    traceables.Append(tilePx.x);

    // Update PRNG state
    randBuf[tilePx.x] = randStrm;

    // Initialize path values for the current pixel
    //float3 tilePalette[4] = { float3(1.0f, 0.0f.xx),
    //                          float3(0.0f, 1.0f, 0.0f),
    //                          float3(0.0f.xx, 1.0f),
    //                          float3(1.0f.xxx) };
    displayTex[tilePx.yz] = float4(1.0f.xxx,
                                   //tilePalette[(linPx / resInfo.x) % 4],
                                   pRay.w);
}