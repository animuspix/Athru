
// Generic #include for render-specific shader code
// (since lots of things are shared through the render pipeline, and
// I'll eventually have physics + ecologies on the GPU as well)
#include "Core3D.hlsli"
#include "RenderUtility.hlsli"

// Counters carrying the number of active paths on the GPU + the number
// of bounces for each material (either per-bounce or overall)
// Reset to [resolution] or [0] at the start of each frame, gradually
// incremented/decremented as rays terminate or leave the scene during 
// path-tracing
RWBuffer<uint> counters : register(u3);

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Extract a pixel ID from the given thread/group IDs
    uint2 pixID = uint2((groupID.x * TRACING_GROUP_WIDTH) + (threadID % TRACING_GROUP_WIDTH),
                        (groupID.y * TRACING_GROUP_WIDTH) + (threadID / TRACING_GROUP_WIDTH));
    uint linPixID = pixID.x + (pixID.y * resInfo.x);

    // Extract a permutable value from [randBuf]
    // (needed for ray jitter)
    // Same domain as [SceneVis] so no reason to use different
    // indices
    uint randNdx = linPixID;
    PhiloStrm randStrm = philoxVal(randNdx,
                                   timeDispInfo.z);
    float4 rand = iToFloatV(philoxPermu(randStrm));

    // Generate an outgoing ray-direction for the current pixel
    // Also cache the current ray's filter value + z-scale
    float4 rayDir = PixToRay(pixID,
                             aaBuffer[linPixID].sampleCount.x + 1,
                             rand.xy);

    // Reset the path-counter here  
    InterlockedAdd(pathCtr[0], 1);

    // Initialize path values for the current pixel
    pathCtr[linPixID] = LiPathBuilder();
}