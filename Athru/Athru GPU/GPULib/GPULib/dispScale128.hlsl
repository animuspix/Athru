#include "RenderUtility.hlsli"
#include "ScenePost.hlsli"

groupshared int base = 0;
#define MAX_CTR_NDX 21
[numthreads(2, 3, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Extract a pixel ID from the given thread/group IDs
    uint2 pixID = uint2((groupID.x * TRACING_GROUP_WIDTH) + (threadID % TRACING_GROUP_WIDTH),
                        (groupID.y * TRACING_GROUP_WIDTH) + (threadID / TRACING_GROUP_WIDTH));
    uint linPixID = pixID.x + (pixID.y * resInfo.x);
    if (linPixID > MAX_CTR_NDX) { return; } // Mask off excess threads
    InterlockedAdd(base, counters[21]); // Cache the previous threads-per-group within [base]
    if (linPixID < 21)
    {
        counters[linPixID] *= base; // Undo previous dispatch-scales
        if (base == 1) // Record one-based dispatch sizes inside the 23rd-29th counters
        {
            int recordNdx = (linPixID < 18) ? (24 + linPixID / 3) : 23;
            counters[recordNdx] = counters[linPixID / 3];
        }
        uint scaled = counters[linPixID] / 128;
        counters[linPixID] = max(scaled, 1); // Keep dispatch axes above zero
    }
    else if (linPixID == 21) { counters[linPixID] = 128; } // Record 128x scaling factor
}