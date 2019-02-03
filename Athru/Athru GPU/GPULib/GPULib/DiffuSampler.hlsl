#include "MIS.hlsli"
#include "RenderUtility.hlsli"

// Buffer carrying intersections across ray-march iterations
// Simply ordered, used to minimize dispatch sizes for
// ray-marching
AppendStructuredBuffer<LiBounce> traceables : register(u3);

// Diffuse intersection buffer
ConsumeStructuredBuffer<LiBounce> diffuIsections : register(u6);

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Cache linear thread ID
    uint linDispID = threadID + (groupID.x * 256); // Assumes one-dimensional dispatches from [BouncePrep]
    if (linDispID > (counters[24] - 1)) { return; } // Mask off excess threads (assumes one-dimensional dispatches)

    // Cache intersection info for the sample point
    LiBounce isect = diffuIsections.Consume();
    if (linDispID == 0) // Zero diffuse dispatch sizes
    { counters[0] = 0; }

    // Extract a Philox-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    // Initialize [randBuf] with hashed start times in the first frame
    PhiloStrm randStrm = isect.randStrm;
    float4 rand = iToFloatV(philoxPermu(randStrm));

    // Cache adaptive epsilon
    float eps = bounceInfo.z;

    // Generate + cache the local tangent-space matrix
    float3x3 normSpace = NormalSpace(isect.dirs[1]);

    // Perform surface-sampling + MIS for next-event-estimation
    float4 neeWi = DiffuseDir(isect.dirs[2],
                              isect.dirs[1],
                              rand.zw);
    neeWi.w *= isect.mat[0][(uint)isect.mat[2].z];
    float3x4 occData = OccTest(float4(isect.iP, isect.figID),
                               float4(mul(neeWi.xyz, normSpace), STELLAR_FIG_ID),
                               eps);
    float3 neeSrfRGB = Emission(STELLAR_RGB,
                                STELLAR_BRIGHTNESS,
                                occData[0].w) *
                       abs(dot(isect.dirs[1], occData[0].xyz));
    float3 neeRGB = 0.0f.xxx;
    float4 surf = isect.mat[1];
    if (!isect.gatherOcc.x) // Process source gathers with MIS
    {
        neeRGB += isect.gatherRGB.rgb *
                  DiffuseBRDF(surf,
                              float3x3(isect.gatherOcc.yzw,
                                       isect.dirs[1],
                                       isect.dirs[2])) /
                  MISWeight(1, isect.gatherRGB.w,
                            1, neeWi.w);
    }
    if (!occData[1].w) // Process surface gathers with MIS
    {
        neeRGB += neeSrfRGB *
                  DiffuseBRDF(surf,
                              float3x3(neeWi.xyz,
                                       isect.dirs[1],
                                       isect.dirs[2])) /
                  MISWeight(1, isect.gatherRGB.w,
                            1, neeWi.w);
    }

    // Update bounce gather information
    bool srcGatherOcc = isect.gatherOcc.x;
    isect.gatherOcc.x = (isect.gatherOcc.x && occData[1].w);
    isect.gatherRGB = float4(neeRGB, 1.0f); // PDFs already transformed with the power heuristic +
                                            // applied to gather rays

    // Prepare for the next bounce, but only if next-event-estimation
    // fails
    if (isect.gatherOcc.x)
    {
        // Sample an incoming ray direction for light-transport
        float3 oDir = isect.dirs[0]; // Cache the outgoing direction
        float4 iDir = DiffuseDir(isect.dirs[2],
                                 isect.dirs[1],
                                 rand.xy);
        isect.dirs[0].xyz = mul(iDir.xyz, normSpace); // Trace along the bounce direction
        iDir.w /= isect.mat[0][(uint)isect.mat[2].z]; // Scale surface PDFs by selection probability for the chosen
                                                      // material primitive

        // Shade with the expected incoming direction
        float3 rgb = DiffuseBRDF(surf,
                                 float3x3(iDir.xyz,
                                          isect.dirs[1],
                                          isect.dirs[2])) / ZERO_PDF_REMAP(iDir.w) * abs(dot(isect.dirs[1],
                                                                                             isect.dirs[0]));
        isect.dirs[2] = oDir; // Update outgoing direction
        displayTex[isect.id.yz] *= float4(rgb, 1.0f);
        if (dot(rgb, 1.0f.xxx) > 0.0f) // Only propagate paths with nonzero brightnesss
        {
            isect.eP = isect.iP; // Diffuse bounce, incident and exitant positions are equivalent
            traceables.Append(isect);
            InterlockedAdd(counters[18], 1u); // Update indirect dispatch group size
                                              // Experimenting with one-dimensional dispatches
        }
    }
    else // Otherwise apply next-event-estimated shading into path color
    { displayTex[isect.id.yz] *= isect.gatherRGB; }

    // Update Philox key/state for the current path
    randBuf[isect.id.x] = randStrm;
}
