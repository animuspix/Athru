#include "RenderUtility.hlsli"
#include "ScenePost.hlsli"

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Extract a pixel ID from the given thread/group IDs
    uint2 pixID = uint2((groupID.x * TRACING_GROUP_WIDTH) + (threadID % TRACING_GROUP_WIDTH),
                        (groupID.y * TRACING_GROUP_WIDTH) + (threadID / TRACING_GROUP_WIDTH));
    uint linPixID = pixID.x + (pixID.y * resInfo.x);
    if (linPixID > resInfo.w) { return; } // Mask off excess threads
    if (linPixID == 0) { counters[22] = 0; } // Zero the light bounce counter
    // Filter/tonemap, then transfer to the display texture
    float4 tx = displayTex[pixID];
    displayTex[pixID] = float4(PathPost(tx.rgb,
                                        tx.a,
                                        linPixID,
                                        resInfo.z), 1.0f);
                        //tx;
}