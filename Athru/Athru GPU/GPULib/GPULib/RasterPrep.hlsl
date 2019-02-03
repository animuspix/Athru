#include "RenderUtility.hlsli"
#include "ScenePost.hlsli"

// Buffer carrying samples to rasterize for the current frame
ConsumeStructuredBuffer<uint2> rasterPx : register(u3);

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Extract a pixel ID from the given thread/group IDs
    uint2 pixID = uint2((groupID.x * TRACING_GROUP_WIDTH) + (threadID % TRACING_GROUP_WIDTH),
                        (groupID.y * TRACING_GROUP_WIDTH) + (threadID / TRACING_GROUP_WIDTH));
    uint linPixID = pixID.x + (pixID.y * tilingInfo.x);
    if (linPixID > tilingInfo.z) { return; } // Mask off excess threads
    if (linPixID == 0) { counters[22] = 0; } // Zero the light bounce counter
    // Filter/tonemap, then transfer to the display texture
    uint2 px = rasterPx.Consume();
    float4 tx = displayTex[px];
    displayTex[px] = float4(PathPost(tx.rgb,
                                     tx.a,
                                     linPixID,
                                     resInfo.z), 1.0f);
}