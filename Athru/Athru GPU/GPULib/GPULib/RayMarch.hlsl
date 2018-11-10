
#include "RenderUtility.hlsli"
#include "Core3D.hlsli"

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

    // [Consume()] a pixel from the end of [traceables]
    LiBounce traceable = traceables.Consume();

    // Cache path information at the current intersection
    LiPath path = pathBuffer[traceables.id];

    // Extract a Philox-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    // Initialize [randBuf] with hashed start times in the first frame
    uint randNdx = traceable.id;
    PhiloStrm randStrm = path.randStrm;

    // Small vector for raymarching offsets between bounces
    float3 rayVec = EPSILON_MIN.xxx;

    // Define an adaptive epsilon (marching cutoff) for optimal
    // marching performance (no reason to march to within a hundred-thousandth of
    // a surface when the camera is 150 units away anyways)
    float adaptEps = traceEpsInfo.y;

    // Baseline ray distance
    // Maximum ray distance is stored inside the [MAX_RAY_DIST] constant within [Core3D.hlsli]
    float rayDist = (adaptEps * 2.0f);

    // March towards the next intersection in the scene
    Figure star = figures[STELLAR_FIG_ID];
    for (uint i = 0u; i < MAX_VIS_MARCHER_STEPS; i += 1u)
    {
        // Scale rays through the scene, add bounce/emission origin
        rayVec = float3(rayDist * path.dirs[2]) + traceable.iP;

        // Evaluate the scene SDF
        float2x3 sceneField = SceneField(rayVec,
                                         traceable.iP,
                                         true,
                                         FILLER_SCREEN_ID,
                                         adaptEps);
        // Process intersections
        if (sceneField[0].x < adaptEps)
        {
            // Set incident position for the current bounce (ray origin for succeeding rays may be equal to 
            // incident or exitant position for previous rays, so we avoid setting that until after we've
            // evaluated materials at the intersection position)
            traceable.ip = sceneField[1];
            // Process light-source intersections
            if (sceneField[0].y == STELLAR_FIG_ID) // Only stellar light sources supported for now
            {
                // Immediately break out on light-source intersections
                path.mediaRGB *= Emission(STELLAR_RGB, STELLAR_BRIGHTNESS, rayDist); // Treat light intersections
                                                                                     // as ray integrals instead of
                                                                                     // path events (allows us to
                                                                                     // handle them inline instead
                                                                                     // of writing a specialized
                                                                                     // light-intersection shader)
                break; // Exit the marching loop without preparing the current intersection for next-event-estimation
                       // + sampling
            } 

            // Prepare intersections for next-event-estimation + sampling
            surfIsections.Append(traceable);

            // Exit the marching loop
            break;
        }
        // Handle participating media
        //if () // Evaluate phase functions
        //{
            // Scale media shading when phase functions return more than [some value]
            // Only atmospheric/cloud scattering supported atm
            //if (atmosScttr) { path.mediaRGB *= atmosT(); }
            //else { path.mediaRGB *= cloudyT(); }
        //}
        // Ignore rays that travel too far into the scene
        if (rayDist >= MAX_RAY_DIST) { break; }
        rayDist += sceneField[0].x;
    }
    // Update Philox key/state for the current path
    pathBuffer[traceable.id].randStrm = randStrm;
}
