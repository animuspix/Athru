
#ifndef LIGHTING_UTILITIES_LINKED
    #include "LightingUtility.hlsli"
#endif
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif
#include "PhiloInit.hlsli"

// Per-bounce indirect dispatch axes
// (tracing axes in (0...2), sampling axes in (3...5), input traceable
// count in [6], input sampling count in [7]
RWBuffer<uint> dispAxes : register(u12);

// Append/consume counters for traceables + material primitives
RWBuffer<uint> traceCtr : register(u13);
RWBuffer<uint> diffuCtr : register(u14);
RWBuffer<uint> mirroCtr : register(u15);
RWBuffer<uint> refraCtr : register(u16);
RWBuffer<uint> snowwCtr : register(u17);
RWBuffer<uint> ssurfCtr : register(u18);
RWBuffer<uint> furryCtr : register(u19);

// Buffer carrying intersections across ray-march iterations
ConsumeStructuredBuffer<uint> traceables : register(u20);

// Material intersection buffers
AppendStructuredBuffer<uint> diffuIsections : register(u21);
AppendStructuredBuffer<uint> mirroIsections : register(u22);
AppendStructuredBuffer<uint> refraIsections : register(u23);
AppendStructuredBuffer<uint> snowwIsections : register(u24);
AppendStructuredBuffer<uint> ssurfIsections : register(u25);
AppendStructuredBuffer<uint> furryIsections : register(u26);

// The maximum number of steps allowed for the primary
// ray-marcher
#define MAX_VIS_MARCHER_STEPS 512

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Automatically dispatched threads can still outnumber data available;
    // mask those cases here
    uint rayID = threadID + groupID.x * 256; // Assumes one-dimensional dispatches
    if (rayID >= dispAxes[6]) { return; }

    // Map ray-marching threads to pixels
    uint ndx = traceables.Consume();

    // Cache input epsilon values
    float eps = gpuInfo.cameraPos.w;

    // Baseline ray distance
    // Maximum ray distance is stored inside the [MAX_RAY_DIST] constant within [Core3D.hlsli]
    float t = (eps * 2.0f);

    // Relevant values for distance relaxation
    float w = 1.0f;
    float prevF = 0.0f; // Most recent distance at each step
    float prevDt = 0.0f; // Most recent offset at each step

    // Extract a Philox-permutable value from [randBuf]
    PhiloStrm randStrm = randBuf[ndx];
    float4 rand = iToFloatV(philoxPermu(randStrm));

    // Controls debug shading for stellar intersections + escaped rays
    //#define LOST_RAY_DEBUG

    // March towards the next intersection in the scene
    float4 rgba = 1.0f.xxxx;
    Figure star = figures[STELLAR_FIG_ID];
    float2x3 ray = rays[ndx];
    float fSamples[4] = { 0, 0, 0, 0 };
    float fMean = 0.0f;
    float fDeltas = 0.0f;
    float fDeltaSet[4] = { 0, 0, 0, 0 };
    bool relaxed = true;
    for (uint i = 0u; i < MAX_VIS_MARCHER_STEPS; i += 1u)
    {
        // Scale rays through the scene, add bounce/emission origin
        float3 rayVec = float3(t * ray[1]) + ray[0];

        // Evaluate the scene SDF
        float2x3 sceneField = SceneField(rayVec,
                                         ray[0],
                                         true,
                                         FILLER_SCREEN_ID,
                                         eps);

        // Revert to standard sphere-tracing on excessive relaxation
        if (prevF + sceneField[0].x < prevDt && relaxed)
        {
            relaxed = false;
            t += prevDt * 1.0f - w;
            rayVec = float3(t * ray[1]) + ray[0];
            sceneField = SceneField(rayVec,
                                    ray[1],
                                    true,
                                    FILLER_SCREEN_ID,
                                    eps);
        }

        // Process intersections
        if (sceneField[0].x < eps)
        {
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

            // Set incident position for the current bounce (exitant position depends on the intersected surface,
            // so we avoid setting that until after we've evaluated materials at [rayVec])
            rayOris[ndx] = rayVec;

            // Cache incident SDF type + figure-ID
            figIDs[ndx] = sceneField[0].z;

            // Choose a material for the current position, then pass the
            // current ray to a per-material output stream
            //rgba = float4(abs(normalize(rayVec)), 1.0f);
            uint matPrim = MatPrimAt(rayVec, rand.z);
            switch (matPrim)
            {
                case 0:
                    diffuIsections.Append(ndx);
                    break;
                case 1:
                    mirroIsections.Append(ndx);
                    break;
                case 2:
                    refraIsections.Append(ndx);
                    break;
                case 3:
                    snowwIsections.Append(ndx);
                    break;
                case 4:
                    ssurfIsections.Append(ndx);
                    break;
                case 5:
                    furryIsections.Append(ndx);
                    break;
                default:
                    matPrim = 0; //abort();
                    break; // Unknown material
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
            // Maintain running distance average & sum of squared residuals
            // (needed to compute variance along each ray)
            float prevFSample = fSamples[i % 4];
            fSamples[i % 4] = sceneField[0].x / 4.0f;
            fMean += fSamples[i % 4] - prevFSample;
            float d = sceneField[0].x - fMean;
            float prevFDelta = fDeltaSet[i % 4];
            fDeltaSet[i % 4] = d * d;
            fDeltas += fDeltaSet[i % 4] - prevFDelta;

            // Compute variance and clamp to the range (0...1)
            float vari = min(fDeltas / 3.0f, 1.0f);

            // Shift change in relaxation inversely to variance (more for smooth areas, less for rough ones)
            float dw = 1.0f - vari;

            // Cache most-recent step distance + ray offset, offset rays for each step
            prevF = sceneField[0].x; // Update most-recent scene distance
            float ddw = dw * vari;
            //ddw = pow(ddw, 16.0f);
            w = 1.0f + ddw; // Magic! Scaling the relaxation factor by plain variance allows overstepping for
                            // highly-stable geometry ((1.0 - 0.1) * 0.9 = 0.81) but prevents skipping geometry
                            // much more aggressively as variance tends towards 1 ((1.0 - 0.9) * 0.1 = 0.09)
                            // Raising to an exponent increases intensity again for stable areas while reducing
                            // it for variant rays, but the default version works well for most cases
                            // Graphical curve available over here:
                            // https://www.desmos.com/calculator/xys3ltrxym
            prevDt = prevF * w; // Update most-recent ray offset
            t += prevDt;

            // Increase epsilon every step so that deeply iterated rays gradually attract towards the scene
            // surface
            eps *= 1.175f;
        }
    }
    // Apply primary-ray shading
    displayTex[uint2(ndx % gpuInfo.resInfo.x, ndx / gpuInfo.resInfo.x)] *= rgba;

    // Update RNG history
    randBuf[ndx] = randStrm;
}
