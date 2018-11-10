
#include "RenderUtility.hlsli"

// Buffer carrying intersections across ray-march iterations
// Simply ordered, used to minimize dispatch sizes for
// ray-marching
AppendStructuredBuffer<LiBounce> traceables : register(u6);

// Relational, used to organize intersections for surface
// sampling + shading
// Multidimensional intersection-buffer with different BXDFs on 
// each axis and two separate spaces for different contexts; 
// zeroth space carries intersections/material per-bounce, 
// first space carries intersections/material globally
RWStructuredBuffer<LiBounce> matIsections : register(u7);

// Counters associated with [matIsections]; per-bounce in 0...8,
// globally in 9...17
RWBuffer<uint> counters : register(u3);

[numthreads(8, 4, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Zero the global diffuse intersection counter
    InterlockedAnd(counters[9], 0u);

    // The number of traceable elements at each bounce limits the 
    // number of path-tracing threads that can actually execute;
    // immediately exit from any threads outside that limit
    // (required because we want to spread path-tracing threads
    // evenly through 3D dispatch-space (rational cube-roots are
    // uncommon + force oversized dispatches), also because we
    // can't set per-group thread outlook (i.e. [numthreads])
    // programatically before dispatching [this])
    uint dispWidth = (uint)timeDispInfo.w;
    uint linGroupID = (groupID.x + groupID.y * dispWidth) +
                      (groupID.z * (dispWidth * dispWidth));
    uint linDispID = (linGroupID * 128) + (threadID + 1);
    if (linDispID > counters[16]) { return; }

    // Evaluate, cache shading for the local bounce
    matIsections[resInfo * linDispID + 
                 (resInfo.w * bounceInfo.x * bounceInfo.y)].rho = MirroBRDF(float4x3(isect.dirs[0], 
                                                                                     isect.dirs[3],
                                                                                     isect.dirs[2],
                                                                                     isect.dirs[1]),
                                                                            float4(surf.rgb, surf.a * surf.a),
                                                                            isect.mat[2].xy);
}
