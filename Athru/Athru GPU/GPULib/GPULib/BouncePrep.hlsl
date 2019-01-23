
#include "RenderUtilityCore.hlsli"
#include "MatSynth.hlsli"
#ifndef LIGHTING_UTILITIES_LINKED
    #include "LightingUtility.hlsli"
#endif
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif

// Buffer carrying intersections between ray-march iterations
// Basically a staging buffer to allow separate intersection tests
// and next-event-estimation/sampling
ConsumeStructuredBuffer<LiBounce> surfIsections : register(u6);

// Material intersection buffers
AppendStructuredBuffer<LiBounce> diffuIsections : register(u7);
AppendStructuredBuffer<LiBounce> mirroIsections : register(u5);
AppendStructuredBuffer<LiBounce> refraIsections : register(u3);
AppendStructuredBuffer<LiBounce> snowwIsections : register(u2);
AppendStructuredBuffer<LiBounce> ssurfIsections : register(u1);
AppendStructuredBuffer<LiBounce> furryIsections : register(u0);

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Automatically dispatched threads can still outnumber data available;
    // mask those cases here
    // Assumes one-dimensional dispatches
    uint numRays = counters[23];
    uint rayID = threadID + groupID.x * 256u;
    if (rayID > numRays) { return; }

    // [Consume()] an intersection from the end of [surfIsections]
    LiBounce isect = surfIsections.Consume();

    // Cache epsilon value used in the current frame
    float eps = bounceInfo.z;

    // Perform source-sampling for next-event-estimation
    Figure star = figures[STELLAR_FIG_ID];
    isect.dirs[1] = PlanetGrad(isect.iP,
                               figures[isect.figID]); // Placeholder gradient, will use figure-adaptive normals later...
    float4 rand = iToFloatV(philoxPermu(isect.randStrm));
    float3 stellarSurfPos = StellarSurfPos(float4(systemOri.xyz,
                                                  figures[STELLAR_FIG_ID].scale.x),
                                           rand.xy,
                                           isect.iP);
    float3x4 occData = OccTest(float4(isect.iP, eps),
                               float4(normalize(stellarSurfPos - isect.iP), STELLAR_FIG_ID),
                               bounceInfo.z);
    isect.gatherRGB.rgb = Emission(STELLAR_RGB,
                                   STELLAR_BRIGHTNESS,
                                   occData[0].w) *
                          abs(dot(isect.dirs[1], occData[0].xyz));
    isect.gatherRGB.w = StellarPosPDF();
    isect.gatherOcc = float4(occData[1].w,
                             occData[0].xyz);

    // Sample material at the current intersection
    isect.mat = MatGen(isect.iP,
                       isect.dirs[2],
                       isect.dirs[1],
                       isect.etaIO.x,
                       isect.dfType,
                       isect.randStrm);

    // Prepare for surface sampling/NEE
    // Update dispatch sizes for the current material
    uint ndx = ((uint)isect.mat[2].z) * 3;
    InterlockedAdd(counters[ndx], 1u);

    // Append to per-material output streams
    switch (((uint)(isect.mat[2].z)))
    {
        case 0:
            diffuIsections.Append(isect);
            break;
        case 1:
            mirroIsections.Append(isect);
            break;
        case 2:
            refraIsections.Append(isect);
            break;
        case 3:
            snowwIsections.Append(isect);
            break;
        case 4:
            ssurfIsections.Append(isect);
            break;
        case 5:
            furryIsections.Append(isect);
            break;
        default:
            return; // Unknown material
    }
}