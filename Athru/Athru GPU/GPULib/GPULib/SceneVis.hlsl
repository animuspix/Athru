
#include "Lighting.hlsli"
#include "ScenePost.hlsli"
#ifndef RASTER_CAMERA_LINKED
    #include "RasterCamera.hlsli"
#endif
#ifndef MIS_FNS_LINKED
    #include "MIS.hlsli"
#endif

// A two-dimensional texture representing the display; used
// as the image source during core rendering (required because
// there's no support for direct render-target access in
// DirectX)
RWTexture2D<float4> displayTex : register(u0);

// Buffer carrying traceable pixels + pixel information
// Ray directions are in [0][xyz], filter values are in [1][x],
// pixel indices are in [1][y], non-normalized ray z-offsets
// are in 1[z]
ConsumeStructuredBuffer<float2x3> traceables : register(u3);

// Intersection counter ([traceables] without SWR); included
// here because there's no safe way to reset it per-frame
// from within [PathReduce.hlsl]
RWBuffer<int> maxTraceables : register(u4);

// The maximum number of steps allowed for the primary
// ray-marcher
#define MAX_VIS_MARCHER_STEPS 512

// Scale epsilon values to match the camera's distance from the system
// planets + star
float AdaptEps(float3 camRayOri)
{
    // Evaluate average distance to the camera across all figures
    float avgDist = 0.0f;
    for (uint i = 0; i < MAX_NUM_FIGURES; i += 1)
    {
        avgDist += PlanetDF(camRayOri,
                            figures[i],
                            i,
                            EPSILON_MAX)[0].x;
    }

    // Initialise an epsilon value to [EPSILON_MIN] and make it twice as
    // large for every five-unit increase in average distance (up to a maximum of
    // [EPSILON_MAX])
    // E.g. a ten-unit distance means a two-times increase (2^1)
    //      a fifty-unit distance means a 1024-times increase (2^10)
    //      a seventy-five-unit distance means a 16384-times increase (2^15)
    //      a hundred-unit distance means a ~1,000,000-times increase (2^20)
    // BUT
    // Epsilons are still thresholded at [MARCHER_EPSILON_MAX], so exponential
    // increases in scaling distance never cause significant ray over-stepping
    // (and thus invalid distances)
    float adaptEps = EPSILON_MIN;
    adaptEps *= pow(2, avgDist / 5.0f);
    return min(max(adaptEps, EPSILON_MIN),
               EPSILON_MAX);
}

[numthreads(8, 4, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // The maximum number of traceables for this frame was cached on the CPU
    // immediately after [PathReduce.hlsl] finished executing, so zero
    // [maxTraceables] here
    InterlockedAnd(maxTraceables[0], 0);

    // The number of traceable elements emitted by [PathReduce]
    // (i.e. the number of traceable elements) limits the number
    // of path-tracing threads that can actually execute;
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
    if (linDispID > (uint)numTraceables.x) { return; }

    // [Consume()] a pixel from the end of [traceables]
    float2x3 traceable = traceables.Consume();

    // Separately cache the pixel's screen position for later reference
    uint2 pixID = uint2(traceable[1].y % resInfo.x,
                        traceable[1].y / resInfo.x);

    // Extract a Philox-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    // Initialize [randBuf] with hashed start times in the first frame
    uint randNdx = traceable[1].y;
    PhiloStrm randStrm = philoxVal(randNdx,
                                   timeDispInfo.z);
    //#define RNG_TEST
    #ifdef RNG_TEST
        float4 rand = iToFloatV(philoxPermu(randStrm));
        displayTex[pixID] = rand;
        randBuf[randNdx] = randStrm;
        return;
    #endif

    // Cache/generate initial path information
    Path path;
    path.distToPrev = 0.0f;
    path.attens[0] = 1.0f.xxx;
    path.attens[1] = 1.0f.xxx;
    path.mat = float4x4(0.0f.xxxx,
                        0.0f.xxxx,
                        0.0f.xxxx,
                        0.0f.xxxx); // Placeholder material
    path.rd = float4(traceable[0], 0); // Placeholder figure-ID for starting paths
    path.ro = cameraPos.xyz;
    path.norml = mul(float4(UNIT_Z, 1.0f), viewMat).xyz; // Placeholder BXDF-ID for starting paths
    path.p = path.ro;
    path.srfP = path.ro;
    path.ambFrs = IOR_VACUU; // Feel I'll drive this from local surfaces later (i.e. initialise to atmosphere in
                             // atmosphere, water underwater, etc.)
                             // Will like to push in IOR_ATMOS and use atmospheric scattering sometime, but am avoiding
                             // that to keep things simple rn

    // Small vector for raymarching offsets between bounces
    float3 rayVec = EPSILON_MIN.xxx;

    // Define an adaptive epsilon (marching cutoff) for optimal
    // marching performance (no reason to march to within a hundred-thousandth of
    // a surface when the camera is 150 units away anyways)
    float adaptEps = 0.1f;//AdaptEps(path.ro);

    // March the scene
    //#define DEMO_VOXELS
    #ifdef DEMO_VOXELS
        // Raytrace the zeroth planet's bounding-box
        float3 isect = BoundingBoxTrace(path.ro + path.rd.xyz * EPSILON_MIN,
                                        path.ro,
                                        100,
                                        float3(350.0f, 0.0f, 80.0f));
        if (isect.z)
        {
            // Checking/debugging rasterized SDF distances here
            float3 v = path.ro + path.rd.xyz * (isect.x + adaptEps);//(sin(timeDispInfo.z * 0.125) * 128.0f);
            float4 linTransf = float4(float3(350.0f, 0.0f, 80.0f),
                                      100.0f);
            float t = EPSILON_MIN;
            float step = 1.0f / numTraceables.z;
            uint i = 0;
            while (i < 256 &&
                   t < abs(isect.y - isect.x))
            {
                // Lazily interpolate box distances
                // Would like to use trilinear interpolation here instead...
                float3 vT = v + path.rd.xyz * t;
                float3 pludXYZ = float3(rasterAtlas[vecToNdx(vT + float3(step, 0.0f, 0.0f),
                                                             0,
                                                             DF_TYPE_PLANET,
                                                             linTransf)],
                                        rasterAtlas[vecToNdx(vT + float3(0.0f, step, 0.0f),
                                                             0,
                                                             DF_TYPE_PLANET,
                                                             linTransf)],
                                        rasterAtlas[vecToNdx(vT + float3(0.0f, 0.0f, step),
                                                             0,
                                                             DF_TYPE_PLANET,
                                                             linTransf)]);
                float3 mindXYZ = float3(rasterAtlas[vecToNdx(vT - float3(step, 0.0f, 0.0f),
                                                    0,
                                                    DF_TYPE_PLANET,
                                                    linTransf)],
                                        rasterAtlas[vecToNdx(vT - float3(0.0f, step, 0.0f),
                                                             0,
                                                             DF_TYPE_PLANET,
                                                             linTransf)],
                                        rasterAtlas[vecToNdx(vT - float3(0.0f, 0.0f, step),
                                                             0,
                                                             DF_TYPE_PLANET,
                                                             linTransf)]);
                float3 vS = pOffs(vT, linTransf);
                float2x3 dt = float2x3(pludXYZ,
                                       mindXYZ);
                float dist = lerp(lerp(dt._11_21[!sign(vT.x)].x,
                                       dt._12_22[!sign(vT.y)].x, vS.x),
                                  dt._13_23[!sign(vT.z)].x, vS.z);
                if (dist < 0.1f)
                {
                    displayTex[pixID] = (float(i) / 256).xxxx;
                    return;
                }
                else
                {
                    t += max(dist, step);
                    i += 1;
                }
            }
            displayTex[pixID] = 0.0f.xxxx;
        }
        else
        {
            displayTex[pixID] = 0.0f.xxxx;
        }
        return;
    #endif

    // Baseline ray distance
    // Maximum ray distance is stored inside the [MAX_RAY_DIST] constant within [Core3D.hlsli]
    float rayDist = (adaptEps * 2.0f);

    // Pre-fill the first vertex on the camera sub-path with the baseline camera sample
    // No figures or surfaces at the camera pinhole/lens, so fill those with placeholder values
    // (11 => only ever 10 figures in the scene; 5 => only four defined BXDFs)
    float3 lensNormal = mul(float4(0.0f, 0.0f, 1.0f, 1.0f), viewMat).xyz;
    float4 camInfo = float4(lensNormal, traceable[1].x);
    float pixWidth = sqrt(NUM_AA_SAMPLES);

    // Bounces + ray-marching loop
    uint numBounces = 0u;
    bool pathEnded = false;
    Figure star = figures[STELLAR_FIG_ID];
    while (numBounces < maxNumBounces.x &&
           !pathEnded)
    {
        for (uint i = 0u; i < MAX_VIS_MARCHER_STEPS; i += 1u)
        {
            // Scale rays through the scene, add bounce/emission origin
            rayVec = float3(rayDist * path.rd.xyz) + path.ro;
            // Evaluate the scene SDF
            float2x3 sceneField = SceneField(rayVec,
                                             path.ro,
                                             true,
                                             FILLER_SCREEN_ID,
                                             adaptEps);
            // Process intersections if appropriate
            if (sceneField[0].x < adaptEps)
            {
                // Update previous attenuation
                path.attens[1] = path.attens[0];

                // Update previous ray distance
                path.distToPrev = rayDist;

                // Update the local surface origin
                path.srfP = sceneField[1];

                // Process light-source intersections
                if (sceneField[0].y == STELLAR_FIG_ID) // Only stellar light sources supported for now
                {
                    // Immediately break out on light-source intersections
                    path.attens[0] *= Emission(STELLAR_RGB, STELLAR_BRIGHTNESS, rayDist);
                    pathEnded = true;
                    break;
                }

                // Process surface intersections
                path.attens[0] *= ProcVert(float4(rayVec,
                                                  sceneField[0].x),
                                           sceneField[1],
                                           path.rd.xyz,
                                           path.norml,
                                           adaptEps,
                                           sceneField[0].yz,
                                           path.ro,
                                           path.ambFrs,
                                           path.mat,
                                           rayDist,
                                           randStrm);
                // Exit the intersection loop
                break;
            }
            // Ignore rays that travel too far into the scene
            if (rayDist >= MAX_RAY_DIST)
            {
                path.attens[0] = AMB_RGB; // Assume deep rays failed to intersect the scene, shade with ambient background color
                pathEnded = true;
                break;
            }
            rayDist += sceneField[0].x;
        }
        numBounces += 1u;
    }

    // Perform explicit sampling on paths missing light sources
    if (!pathEnded && numBounces > 0)
    {
        path.attens[0] *= LiGather(path,
                                   star.linTransf, // Only stellar light sources atm
                                   adaptEps,
                                   randStrm);
    }

    // Estimated path color associated with the current pixel
    // Random camera-gathers emitted in previous frames might have deposited light
    // on the current sensor/pixel, so add that light into the current path
    // color if appropriate
    // Cache a linearized version of the local pixel ID for later reference
    uint linPixID = traceable[1].y;

    // Apply frame-smoothing + convert to HDR
    path.attens[0] = PathPost(path.attens[0],
                              traceable[1].x,
                              linPixID);

    // Write the final ray color to the display texture
    displayTex[pixID] = float4(path.attens[0], 1.0f);

    // No more random permutations at this point, so commit [randStrm] back into
    // the GPU random-number buffer ([randBuf])
    randBuf[randNdx] = randStrm;
}
