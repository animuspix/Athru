
#include "RenderUtility.hlsli"
#include "MaterialUtility.hlsli"

// Buffer carrying intersections between ray-march iterations
// Basically a staging buffer to allow separate intersection tests
// and next-event-estimation/sampling
ConsumeStructuredBuffer<LiBounce> surfIsections : register(u6);

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

    // [Consume()] an intersection from the end of [surfIsections]
    LiBounce isect = surfIsections.Consume();

    // Extract a Philox-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    // Initialize [randBuf] with hashed start times in the first frame
    PhiloStrm randStrm = pathBuffer[isect.id].randStrm;

    // Define an adaptive epsilon (marching cutoff) for optimal
    // marching performance (no reason to march to within a hundred-thousandth of
    // a surface when the camera is 150 units away anyways)
    float adaptEps = traceEpsInfo.y;

    // Sample material at the current intersection
    isect.mat = MatGen(isect.iP,
                       isect.dirs[2],
                       isect.dirs[1],
                       isect.etaIO.x,
                       isect.dfType,
                       randStrm);

    // Perform next-event-estimation
    // Just source sampling for now, surface-sampled NEE occurs alongside ray-direction
    // sampling for bounces (also MIS weighting + shading for gather rays)
    Figure star = figures[STELLAR_FIG_ID];
    isect.dirs[1] = PlanetGradient(figures[isect.figID]); // Placeholder gradient, will use figure-adaptive normals later...
    float4 rand = iToFloatV(philoxPermu(randStrm));
    float3 stellarSurfPos = StellarSurfPos(star.linTransf,
                                           rand.xy,
                                           isect.iP);
    float3x4 occData = OccTest(float4(isect.iP, adaptEps),
                               float4(normalize(stellarSurfPos - isect.iP), STELLAR_FIG_ID),
                               randStrm);
    isect.gatherRGB.rgb = Emission(STELLAR_RGB,
                                   STELLAR_BRIGHTNESS,
                                   occData[0].w) *
                          abs(dot(isect.dirs[1], occData[0].xyz));
    isect.gatherRGB.w = StellarPosPDF() * path.mat[0][(uint)path.mat[2].z];
    isect.gatherOcc = float4(occData[1].w,
                             occData[0].xyz);

    // Prepare for surface sampling/surface-sampled-NEE
    // Increment intersection counter for the current material
    InterlockedAdd(counters[(uint)isection.mat[2].z], 1u);
    InterlockedAdd(counters[(uint)isection.mat[2].z + 9u], 1u);

    // Manually append to the appropriate channel in the material-buffer
    //MemoryBarrier();
    uint matOffs = resInfo.w * isection.mat[2].z;
    matIsections[matOffs + counters[(uint)isection.mat[2].z]] = isect;
    matIsections[matOffs + counters[(uint)isection.mat[2].z + 9u] + 
                 (resInfo.w * bounceInfo.x * bounceInfo.y)] = isect;
    
    // Update Philox key/state for the current path
    pathBuffer[isect.id].randStrm = randStrm;
}