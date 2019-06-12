#include "RenderBinds.hlsli"
#include "ScenePost.hlsli"
#include "PhiloInit.hlsli"

// Per-bounce indirect dispatch axes
// (tracing axes in the zeroth channel, sampling axes in (1...6))
RWBuffer<uint> dispAxes : register(u9);

// Append/consume counters for traceables + material primitives
RWBuffer<uint> traceCtr : register(u10);
RWBuffer<uint> diffuCtr : register(u11);
RWBuffer<uint> mirroCtr : register(u12);
RWBuffer<uint> refraCtr : register(u13);
RWBuffer<uint> snowwCtr : register(u14);
RWBuffer<uint> ssurfCtr : register(u15);
RWBuffer<uint> furryCtr : register(u16);

// Filtering/AA occurs with tiled pixel positions, so include the pixel/tile-mapper
// here
#include "TileMapper.hlsli"

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Extract a pixel ID from the given thread/group IDs
    uint2 tileID = uint2((groupID.x * TRACING_GROUP_WIDTH) + (threadID % TRACING_GROUP_WIDTH),
                         (groupID.y * TRACING_GROUP_WIDTH) + (threadID / TRACING_GROUP_WIDTH));
    uint linTileID = tileID.x + (tileID.y * gpuInfo.tilingInfo.x);
    if (linTileID > gpuInfo.tilingInfo.z - 1) { return; } // Mask off excess threads

    // Clear counters & dispatch axes in the zeroth thread
    if (threadID == 0)
    {
        if (linTileID == 0)
        {
            dispAxes[0] = 0.0f;
            dispAxes[1] = 0.0f;
            dispAxes[2] = 0.0f;
            dispAxes[3] = 0.0f;
            dispAxes[4] = 0.0f;
            dispAxes[5] = 0.0f;
            traceCtr[0] = 0;
            diffuCtr[0] = 0;
            mirroCtr[0] = 0;
            refraCtr[0] = 0;
            snowwCtr[0] = 0;
            ssurfCtr[0] = 0;
            furryCtr[0] = 0;
        }
    }

    // Generate per-frame pixel coordinate, sample texel
    uint3 px = TilePx(tileID,
                      gpuInfo.tInfo.z,
                      gpuInfo.resInfo.x,
                      gpuInfo.tileInfo.xy);
	float4 tx = displayTex[float2(px.y, gpuInfo.resInfo.y - px.z)]; // Write to the output texture with flipped coordinates (zero is top-left
                                                                    // in window space)
    // Would perform denoising here...

    // Filter/tonemap, then transfer to LDR output
    displayTexLDR[px.yz] = //float4(tx.rgb, 1.0f);
                           float4(PathPost(tx.rgb,
						   				   tx.a,
						   				   linTileID,
						   				   gpuInfo.resInfo.z), 1.0f);
}