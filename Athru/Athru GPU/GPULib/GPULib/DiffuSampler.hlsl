#include "LightingUtility.hlsli"
#ifndef MATERIALS_LINKED
    #include "Materials.hlsli"
#endif

// Buffer carrying intersections across ray-march iterations
// Simply ordered, used to minimize dispatch sizes for
// ray-marching
AppendStructuredBuffer<uint> traceables : register(u11);

// Diffuse intersection buffer
ConsumeStructuredBuffer<uint> diffuIsections : register(u12);

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Cache linear thread ID
    uint linDispID = threadID + (groupID.x * 256); // Assumes one-dimensional dispatches from [RayMarch]
    if (linDispID > (counters[24] - 1)) { return; } // Mask off excess threads (assumes one-dimensional dispatches)

    // Cache intersection info for the sample point
    uint ndx = diffuIsections.Consume();
    if (linDispID == 0) // Zero diffuse dispatch sizes
    { counters[0] = 0; }

    // Extract a Philox-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    // Initialize [randBuf] with hashed start times in the first frame
    PhiloStrm randStrm = randBuf[ndx];
    float4 rand = iToFloatV(philoxPermu(randStrm));

    // Cache adaptive epsilon
    float eps = rndrInfo.bounceInfo.z;

    // Cache intersections + shift outside the local figure
    float2x3 ray = rays[ndx];
    float3 pt = ray[0];
    #ifndef APPROX_PLANET_GRAD
        pt = PtToPlanet(pt, figures[figIDs[ndx]].scale.x),
    #endif
    pt -= (-ray[1]) * eps;

    // Generate + cache the local gradient
    Figure star = figures[STELLAR_FIG_ID];
    uint figID = figIDs[ndx];
    float3 n = PlanetGrad(pt,
                          figures[figID]); // Placeholder gradient, will use figure-adaptive normals later...

    // Generate + cache the local tangent-space matrix
    float3x3 normSpace = NormalSpace(n);

    // Perform sampling for next-event-estimation
    float3 wo = outDirs[ndx];
    float4 surf = float4(SurfRGB(wo, n), SurfAlpha(pt, 0u));
    float diffuP = DiffuChance(pt);
    float4 nee = DiffuLiGather(DiffuseDir(wo, n, rand.xy) / float4(1.0f.xxx, diffuP),
                               surf,
                               n,
                               wo,
                               pt,
                               StellarPosPDF(),
                               normSpace,
                               star.scale.x,
                               figID,
                               eps,
                               rand.xy);

    // Prepare for the next bounce, but only if next-event-estimation fails
    if (!nee.a)
    {
        // Sample an incoming ray direction for light-transport
        float4 iDir = DiffuseDir(wo,
                                 n,
                                 rand.zw);
        iDir.xyz = mul(iDir.xyz, normSpace); // Trace along the bounce direction
        iDir.w /= diffuP; // Scale surface PDFs by selection probability for the chosen
                          // material primitive

        // Shade with the expected incoming direction
        float3 rgb = DiffuseBRDF(surf,
                                 float3x3(iDir.xyz,
                                          n,
                                          wo)) / ZERO_PDF_REMAP(iDir.w) * abs(dot(n,
                                                                                  ray[1]));
        displayTex[ndx] *= float4(rgb, 1.0f);
        if (dot(rgb, 1.0f.xxx) > 0.0f) // Only propagate paths with nonzero brightnesss
        {
            // Update current direction + position, also previous position
            // (diffuse bounce, incident and exitant positions are equivalent)
            rays[ndx] = float2x3(ray[0], iDir.xyz);
            rayOris[ndx] = ray[0];

             // Update outgoing direction
            outDirs[ndx] = ray[1];

            // Propagate path
            traceables.Append(ndx);
        }
    }
    else // Otherwise apply next-event-estimated shading into path color
    { displayTex[ndx] *= float4(nee.rgb, 1.0f); } // PDFs already applied for [nee.rgb]

    // Update Philox key/state for the current path
    randBuf[ndx] = randStrm;
}