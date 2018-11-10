
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
    // Zero the mirrorlike per-bounce intersection counter
    InterlockedAnd(counters[1], 0u);

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

    // Cache intersection info for the sample point
    LiBounce isect = matIsections[resInfo.w + linDispID];

    // Extract a Philox-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    // Initialize [randBuf] with hashed start times in the first frame
    PhiloStrm randStrm = path.randStrm;

    // Cache adaptive epsilon
    float adaptEps = traceEpsInfo.y;

    // Sample an incoming ray direction for light-transport
    isect.dirs[0] = MirroDir(isect.dirs[2],
                             isect.mat[3].xyz,
                             isect.dirs[1],
                             isect.mat[1].w,
                             isect.dirs[3]);

    // Perform surface-sampling + MIS for next-event-estimation
    float3 neeMuNorml = GGXMicroNorml(isect.mat[1].w,
                                      philoxPermu(randStrm).xy);
    float3 h = 0.0f.xxx;
    float4 neeWi = MirroDir(isect.dirs[2],
                            neeMuNorml,
                            isect.dirs[1],
                            isect.mat[1].w,
                            h);
    float3x4 occData = OccTest(float4(isect.iP, adaptEps),
                               float4(neeWi.xyz, STELLAR_FIG_ID),
                               randStrm);
    float3 neeSrfRGB = Emission(STELLAR_RGB,
                                STELLAR_BRIGHTNESS,
                                occData[0].w) *
                       abs(dot(isect.dirs[1], occData[0].xyz));
    float3 neeRGB = 0.0f.xxx;
    float4 surf = isect.mat[1];
    if (!isect.gatherOcc.x) // Process source gathers with MIS
    {
        neeRGB += isect.gatherRGB.rgb *
                  MirroBRDF(float4x3(isect.gatherOcc.yzw, 
                                     normalize(isect.gatherOcc.yzw + isect.dirs[2]),
                                     isect.dirs[2],
                                     isect.dirs[1]),
                            float4(surf.rgb, surf.a * surf.a),
                            isect.mat[2].xy) / 
                  MISWeight(1, isect.gatherRGB.w,
                            !occData[1].w, neeWi.w);
    }
    if (!occData[1].w) // Process surface gathers with MIS
    {
        neeRGB += neeSrfRGB *
                  MirroBRDF(float4x3(occData[0].xyz, 
                                     h,
                                     isect.dirs[2],
                                     isect.dirs[1]),
                            float4(surf.rgb, surf.a * surf.a),
                            isect.mat[2].xy) / 
                  MISWeight(!isect.gatherOcc.x, isect.gatherRGB.w,
                            1, neeWi.w);
    }

    // Update bounce gather information
    isect.gatherOcc.x = !isect.gatherOcc.x || !occData[1].w;
    isect.gatherRGB = float4(neeRGB, 1.0f); // PDFs already transformed with the power heuristic +
                                            // applied to gather rays

    // Update shading data for the current intersection
    matIsections[resInfo.w + counters[10u] + 
                 (resInfo.w * bounceInfo.x * bounceInfo.y)] = isect;

    // Prepare for the next bounce, but only if next-event-estimation
    // fails
    if (isect.gatherOcc.x && occData[1].w)
    { 
        path.bounceCtr += 1; // Increment bounce counter
        isect.dirs[2] = isect.dirs[0]; // Trace along the bounce direction
        traceables.Append(isect); 
    }
   
    // Update Philox key/state for the current path
    pathBuffer[isect.id].randStrm = randStrm;
}
