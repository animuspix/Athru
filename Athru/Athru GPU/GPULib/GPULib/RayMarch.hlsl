
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
    uint maxID = numRays - 1;
    uint rayID = threadID + groupID.x * 128;
    if (rayID > maxID) { return; }

    // [Consume()] a pixel from the end of [traceables]
    LiBounce traceable = traceables.Consume();
    if (rayID == 0)
    {
        // Zero generic dispatch sizes
        counters[18] = 0;

        // We're pushing new dispatch sizes, so update assumed threads/group here
        // ([1] because dispatch sizes are incremented naively and scaled down
        // afterwards)
        counters[21] = 1;
    }
    // Extract a Philox-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    // Initialize [randBuf] with hashed start times in the first frame
    PhiloStrm randStrm = randBuf[traceable.id.x];
    float4 rand = iToFloatV(philoxPermu(randStrm));

    // Small vector for raymarching offsets between bounces
    float3 rayVec = EPSILON_MIN.xxx;

    // Cache input epsilon values
    float eps = bounceInfo.z;

    // Generate a parameterized path culling weight
    const float minCull = 0.01f; // Could optionally set this through [RenderInput]
    const float maxCull = 0.15f; // Could optionally set this through [RenderInput]
    float d = numRays / tilingInfo.z;
    float m = counters[22] / bounceInfo.x;
    float cullWeight = lerp(minCull, maxCull, lerp(d, m, m));

    // Baseline ray distance
    // Maximum ray distance is stored inside the [MAX_RAY_DIST] constant within [Core3D.hlsli]
    float t = (eps * 2.0f);

    // Optionally output noise to the display, then break out
    //#define NOISE_OUT
    #ifdef NOISE_OUT
        displayTex[traceable.id.yz] = rand;
        return;
    #endif

    // Relevant values for distance relaxation
    float w = 1.0f; // Moving relaxation factor
    const float maxW = 1.2f; // Upper relaxation limit
    float prevF = 0.0f; // Most recent distance at each step
    float prevDt = 0.0f; // Most recent offset at each step
    bool relaxed = true; // On/off switch for distance relaxation

    // Controls debug shading for stellar intersections + escaped rays
    //#define LOST_RAY_DEBUG

    // March towards the next intersection in the scene
    float4 rgba = 1.0f.xxxx;
    Figure star = figures[STELLAR_FIG_ID];
    for (uint i = 0u; i < MAX_VIS_MARCHER_STEPS; i += 1u)
    {
        // Scale rays through the scene, add bounce/emission origin
        rayVec = float3(t * traceable.dirs[0]) + traceable.eP;

        // Evaluate the scene SDF
        float2x3 sceneField = SceneField(rayVec,
                                         traceable.eP,
                                         true,
                                         FILLER_SCREEN_ID,
                                         eps);

        // Revert to standard sphere-tracing on excessive relaxation
        if (prevF + sceneField[0].x < prevDt && relaxed)
        {
            relaxed = false;
            t += prevDt * 1.0f - w;
            rayVec = float3(t * traceable.dirs[0]) + traceable.eP;
            sceneField = SceneField(rayVec,
                                    traceable.eP,
                                    true,
                                    FILLER_SCREEN_ID,
                                    eps);
            w = 1.0f;
        }

        // Process intersections
        if (sceneField[0].x < eps)
        {
            // Set incident position for the current bounce (exitant position depends on the intersected surface,
            // so we avoid setting that until after we've evaluated materials at [rayVec])
            traceable.iP = rayVec;

            // Cache incident SDF type + figure-ID
            traceable.dfType = sceneField[0].y;
            traceable.figID = sceneField[0].z;

            // Process light-source intersections
            if (sceneField[0].z == STELLAR_FIG_ID) // Only stellar light sources supported for now
            {
                // Immediately break out on light-source intersections
                rgba = float4(Emission(STELLAR_RGB, STELLAR_BRIGHTNESS, t), 1.0f);
                #ifdef LOST_RAY_DEBUG
                    rgba = float4(1.0f.xx, 0.0f, 1.0f);
                #endif
                break; // Exit the marching loop without preparing the current intersection for next-event-estimation
                       // + sampling
            }

            // Stochastically cancel paths with [cullWeight]; prepare surviving intersections for next-event-estimation + sampling
            if (rand.z > cullWeight)
            {
                // Update generic dispatch group sizes
                InterlockedAdd(counters[18], 1u);

                // Upcoming stage [BouncePrep] has no access to [randBuf], so pass Philox key/state through the intersection instead
                traceable.randStrm = randStrm;

                // Pass surviving paths into [surfIsections]
                surfIsections.Append(traceable);
            }
            else
            {
                //#define CULL_HIGHLIGHT
                #ifdef CULL_HIGHLIGHT
                    rgba = float4(0.1f, 0.75f, 0.2f, 1.0f);
                #else
                    rgba = float4(0.0f.xxx, 1.0f);
                #endif
            }

            // Exit the marching loop
            break;
        }
        //else if () // Handle participating media + volumetrics
        //           // Evaluate phase functions
        //{
            // Scale media shading when phase functions return more than [some value]
            // Only atmospheric/cloud scattering supported atm
            //if (atmosScttr) { displayTex[traceable.id.yz] *= atmosT(); }
            //else { displayTex[traceable.id.yz] *= cloudyT(); }
        //}
        else if (t >= MAX_RAY_DIST) // Ignore rays that travel too far into the scene
        {
            rgba = float4(0.0f.xxx, 1.0f);
            #ifdef LOST_RAY_DEBUG
                rgba = float4(0.0f, 0.0f, 1.0f, 1.0f);
            #endif
            break;
        }
        else // Update ray-marching values
        {
            prevF = sceneField[0].x; // Update most-recent scene distance
            prevDt = prevF * w; // Update most-recent ray offset
            t += prevDt; // Offset rays at each step
            if (relaxed) { w = lerp(1.0f, maxW, pow(0.9f, sceneField[0].x)); } // Scale [w] closer to [maxW] as rays approach the threshold distance from the scene
        }
    }
    // Apply primary-ray shading
    displayTex[traceable.id.yz] *= rgba;
    // Update Philox key/state for the current path
    randBuf[traceable.id.x] = randStrm;
}
