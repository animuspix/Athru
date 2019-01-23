// Leaving for now, will implement with volumetric light transport

// Compilable placeholder code
#include "RenderUtility.hlsli"

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Extract a pixel ID from the given thread/group IDs
    uint2 pixID = uint2((groupID.x * TRACING_GROUP_WIDTH) + (threadID % TRACING_GROUP_WIDTH),
                        (groupID.y * TRACING_GROUP_WIDTH) + (threadID / TRACING_GROUP_WIDTH));
    uint linPixID = pixID.x + (pixID.y * resInfo.x);
    if (linPixID > resInfo.w) { return; } // Mask off excess threads
    // Output random noise to the display
    displayTex[pixID] = float4(iToFloatV(philoxPermu(randBuf[linPixID])).rgb, 1.0f);
}