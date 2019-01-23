
#include "RenderUtility.hlsli"
#ifndef LIGHTING_UTILITIES_LINKED
    #include "LightingUtility.hlsli"
#endif
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif

// Buffer carrying intersections across ray-march iterations
ConsumeStructuredBuffer<LiBounce> traceables : register(u3);

// Buffer carrying intersections between ray-march iterations
// Basically a staging buffer to allow separate intersection tests
// and next-event-estimation/sampling
AppendStructuredBuffer<LiBounce> surfIsections : register(u6);

// The maximum number of steps allowed for the primary
// ray-marcher
#define MAX_VIS_MARCHER_STEPS 512

[numthreads(8, 4, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Automatically dispatched threads can still outnumber data available;
    // mask those cases here
    // Assumes one-dimensional dispatches
    uint numRays = counters[23];
    uint rayID = threadID + groupID.x * 128;
    if (rayID > numRays) { return; }

    // [Consume()] a pixel from the end of [traceables]
    LiBounce traceable = traceables.Consume();

    if (rayID == 0)
    {
        // Zero generic dispatch sizes
        counters[18] = 0;
        counters[19] = 0;
        counters[20] = 0;

        // We're pushing new dispatch sizes, so update assumed threads/group here
        // ([1] because dispatch sizes are incremented naively and scaled down
        // afterwards)
        counters[21] = 1;
    }
    // Extract a Philox-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    // Initialize [randBuf] with hashed start times in the first frame
    PhiloStrm randStrm = randBuf[rayID];
    float4 rand = philoxPermu(randStrm);

    // Small vector for raymarching offsets between bounces
    float3 rayVec = EPSILON_MIN.xxx;

    // Cache input epsilon values
    float eps = bounceInfo.z;

    // Generate a parameterized path culling weight
    const float minCull = 0.1f; // Could optionally set this through [RenderInput]
    const float maxCull = 0.65f; // Could optionally set this through [RenderInput]
    float d = numRays / resInfo.w;
    float m = counters[22] / bounceInfo.x;
    float cullWeight = lerp(m + minCull, maxCull, d * m);

    // Baseline ray distance
    // Maximum ray distance is stored inside the [MAX_RAY_DIST] constant within [Core3D.hlsli]
    float rayDist = (eps * 2.0f);

    displayTex[traceable.id.yz] = iToFloatV(rand);
    return;

    // March towards the next intersection in the scene
    Figure star = figures[STELLAR_FIG_ID];
    for (uint i = 0u; i < MAX_VIS_MARCHER_STEPS; i += 1u)
    {
        // Scale rays through the scene, add bounce/emission origin
        rayVec = float3(rayDist * traceable.dirs[0]) + traceable.eP;

        // Evaluate the scene SDF
        float2x3 sceneField = SceneField(rayVec,
                                         traceable.eP,
                                         true,
                                         FILLER_SCREEN_ID,
                                         eps);
        // Process intersections
        if (sceneField[0].x < eps)
        {
            // Set incident position for the current bounce (ray origin for succeeding rays may be equal to
            // incident or exitant position for previous rays, so we avoid setting that until after we've
            // evaluated materials at the intersection position)
            traceable.eP = sceneField[1];
            // Process light-source intersections
            if (sceneField[0].y == STELLAR_FIG_ID) // Only stellar light sources supported for now
            {
                // Immediately break out on light-source intersections
                displayTex[traceable.id.yz] *= float4(Emission(STELLAR_RGB, STELLAR_BRIGHTNESS, rayDist), 1.0f);
                break; // Exit the marching loop without preparing the current intersection for next-event-estimation
                       // + sampling
            }

            // Stochastically cancel paths with [cullWeight]; prepare surviving intersections for next-event-estimation + sampling
            if (rand.z > cullWeight)
            {
                // Update generic dispatch group sizes
                InterlockedAdd(counters[18], 1u);

                // Update bounce counter
                InterlockedAdd(counters[22], 1u);

                // Upcoming stage [BouncePrep] has no access to [randBuf], so pass Philox key/state through the intersection instead
                traceable.randStrm = randStrm;

                // Pass surviving paths into [surfIsections]
                surfIsections.Append(traceable);
            }

            // Exit the marching loop
            break;
        }
        // Handle participating media + volumetrics
        //if () // Evaluate phase functions
        //{
            // Scale media shading when phase functions return more than [some value]
            // Only atmospheric/cloud scattering supported atm
            //if (atmosScttr) { displayTex[traceable.id.yz] *= atmosT(); }
            //else { displayTex[traceable.id.yz] *= cloudyT(); }
        //}
        // Ignore rays that travel too far into the scene
        if (rayDist >= MAX_RAY_DIST)
        {
            displayTex[traceable.id.yz] *= float4(0.0f.xxx, 1.0f);
            break;
        }
        rayDist += sceneField[0].x;
    }
    // Update Philox key/state for the current path
    randBuf[traceable.id.x] = randStrm;
}
