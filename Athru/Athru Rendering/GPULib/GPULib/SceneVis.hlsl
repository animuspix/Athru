// Powered by magical raymarched rendering;
// techniques implemented from this tutorial:
// http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions/

// Although that tutorial specifically discusses signed distance functions,
// unsigned functions exist as well; this shader calls every distance
// function [DF] to abstract away the distinction (since it doesn't make a
// significant difference to the rendered output anyways)

// Specific ray-based approaches to light transport mined from
// Scratchapixel:
// https://www.scratchapixel.com/index.php
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-ray-tracing/raytracing-algorithm-in-a-nutshell
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-ray-tracing/implementing-the-raytracing-algorithm
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-ray-tracing/adding-reflection-and-refraction
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/diffuse-lambertian-shading
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/ligth-and-shadows
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/shading-spherical-light

// The maximum number of steps allowed for the primary
// ray-marcher
#define MAX_VIS_MARCHER_STEPS 255

#include "Lighting.hlsli"

// Generate a random ray direction
// on a unit hemisphere open towards
// the screen (no point in generating
// rays pointing directly away from the
// scene itself)
float3 RayDir(uint threadID)
{
    return normalize(float3(rand1D(threadID), rand1D(threadID),
                            max(rand1D(threadID), 0.0f)));
}

[numthreads(4, 4, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // The pixel ID (x/y coordinates) associated with the current
    // thread
    uint2 pixID = uint2(groupID.x,
                        (rendPassID.x * 64) + threadID);

    // Return without performing ray-marching if the pixel ID of
    // the current thread sits outside the screen texture
    if (pixID.y > DISPLAY_HEIGHT)
    {
        return;
    }

    // The sampling index associated with the current thread
    // Used to roughly integrate the rendering equation with
    // the Monte Carlo method (basically generating an
    // approximation of the energy at a point by counting lots
    // and lots of areas within the relevant domain (the screen
    // for primary rays, a surface for secondary/tertiary rays))
    // 3D linearization function: Linearize XY space to create a
    // a 2D space ([XY], Z), then linearize that using the standard
    // 2D linear map (x + (y * W))
    // Can be defined as a recursive linear map with the 3D form
    // (x + y * W) + z * (W * H)
    // X: pixel channels
    // Y: pixel rows
    // Z: pixel samples
    uint rayNdx = (groupID.x + (pixID.y * DISPLAY_WIDTH)) + (groupID.y * (DISPLAY_WIDTH * 64));

    float maxRayDist = 64.0f;
    float currRayDist = 0.0f;

    float3 eyePos = cameraPos.xyz;
    float3 rayDir = RayDir(threadID);
    for (uint i = 0; i < MAX_VIS_MARCHER_STEPS; i += 1)
    {
        float3 rayVec = mul(float4(currRayDist * rayDir, 1), viewMat).xyz;
        float2 sceneField = SceneField(eyePos + rayVec);
        if (sceneField.x < EPSILON)
        {
            // Apply direct illumination to the current figure
            // Would invoke color function here...
            float3 illumRGB = PerPointDirectIllum(eyePos + rayVec,
                                                  1.0f.xxx, GetNormal(eyePos + rayVec));

            // Write the illuminated color to the parallel GI buffer
            TracePix trace;
            trace.pixCoord = pixID;
            trace.pixRGBA = float4(illumRGB, 1.0f);
            giCalcBufWritable[rayNdx] = trace;
            break;
        }

        currRayDist += sceneField.x;

        if (currRayDist > (maxRayDist - EPSILON))
        {
            // Assume the ray has passed through the scene without touching anything;
            // attempt to estimate an appropriate color before breaking out
            // Replace current assignment with a planetary/atmospheric color function
            // when possible
            TracePix trace;
            trace.pixCoord = pixID;
            trace.pixRGBA = 1.0f.xxxx;
            giCalcBufWritable[rayNdx] = trace;
            break;
        }
    }
}