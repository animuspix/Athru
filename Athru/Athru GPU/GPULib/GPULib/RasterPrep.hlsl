#include "RenderBinds.hlsli"
#include "ScenePost.hlsli"

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
    uint linTileID = tileID.x + (tileID.y * rndrInfo.tilingInfo.x);
    if (linTileID > rndrInfo.tilingInfo.z - 1) { return; } // Mask off excess threads

    // Generate per-frame pixel coordinate, sample texel
    uint3 px = TilePx(tileID,
                      gpuInfo.tInfo.z,
                      rndrInfo.resInfo.x,
                      rndrInfo.tileInfo.xy);
    float4 tx = displayTex[px.yz];

    // Would perform denoising here...

    // Filter/tonemap, then transfer to LDR output
    displayTexLDR[px.yz] = float4(PathPost(tx.rgb,
								  tx.a,
								  px.x,
								  rndrInfo.resInfo.z), 1.0f);
}