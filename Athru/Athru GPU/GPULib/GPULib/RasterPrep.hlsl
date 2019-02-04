#include "RenderUtility.hlsli"
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
    uint linTileID = tileID.x + (tileID.y * tilingInfo.x);
    if (linTileID > tilingInfo.z - 1) { return; } // Mask off excess threads
    if (linTileID == 0) { counters[22] = 0; } // Zero the light bounce counter
    // Filter/tonemap, then transfer to the display texture
    uint3 px = TilePx(tileID,
                      tInfo.z,
                      resInfo.x,
                      tileInfo.xy);
    float4 tx = displayTex[px.yz];
    displayTex[px.yz] = float4(PathPost(tx.rgb,
                                        tx.a,
                                        px.x,
                                        resInfo.z), 1.0f);
}