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

#include "SharedLighting.hlsli"

// Utility function designed to match rays emitted by the camera
// to the shape of the view frustum (stolen from the shader written
// by the tute author as a code guide)
float3 RayDir(float fovRads, float2 viewSizes, uint2 pixID)
{
    float2 xy = pixID - (viewSizes / 2.0);
    float z = viewSizes.y / tan(fovRads / 2.0);
    return normalize(float3(xy, z));
}

[numthreads(4, 4, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // The pixel ID (x/y coordinates) associated with the current
    // thread
    uint2 pixID = groupID.xy;
    pixID.y = (rendPassID.x * 64) + threadID;

    // Return without performing ray-marching if the pixel ID of
    // the current thread sits outside the screen texture
    if (pixID.y > DISPLAY_HEIGHT)
    {
        return;
    }

    float maxRayDist = 64.0f;
    float currRayDist = 0.0f;

    float3 eyePos = cameraPos.xyz;
    float3 rayDir = RayDir(3.14f / 2.0f, float2(DISPLAY_WIDTH, DISPLAY_HEIGHT), pixID);
    for (uint i = 0; i < MAX_VIS_MARCHER_STEPS; i += 1)
    {
        float3 rayVec = mul(float4(currRayDist * rayDir, 1), viewMat).xyz;
        DFData sceneField = SceneField(eyePos + rayVec);
        if (sceneField.dist < rayEpsilon)
        {
            // Apply direct illumination to the current figure
            float3 localRGB = PerPointDirectIllum(eyePos + rayVec,
                                                  sceneField.rgbaColor.rgb, GetNormal(eyePos + rayVec));

            // Write the illuminated color to the display texture
            float4 traceRGBA = float4(localRGB, sceneField.rgbaColor.a);
            displayTex[pixID] = traceRGBA;

            // Pass the illuminated point into the trace buffer so that we can access it
            // during path tracing
            TracePoint tracePt;
            tracePt.coord = float4(eyePos + rayVec, 0);
            tracePt.rgbaSrc = traceRGBA;
            tracePt.figID = sceneField.id.xxxx;
            tracePt.isValid = 0x1.xxxx;
            tracePt.rgbaGI = float4(0.0f.rrr, 1.0f);
            traceables[(pixID.x * 64) + threadID] = tracePt;
            break;
        }

        currRayDist += sceneField.dist;

        if (currRayDist > (maxRayDist - rayEpsilon))
        {
            // Assume the ray has passed through the scene without touching anything;
            // write a neutral color to the scene texture before recording an
            // invalid trace-point and breaking out
            float4 backRGBA = float4(0.0f.rrr, 1.0f);
            displayTex[pixID] = backRGBA;
            TracePoint tracePt;
            tracePt.coord = float4(eyePos + rayVec, 0);
            tracePt.rgbaSrc = backRGBA;
            tracePt.figID = 0x0.xxxx;
            tracePt.isValid = 0x0.xxxx;
            tracePt.rgbaGI = float4(0.0f.rrr, 1.0f);
            traceables[(pixID.x * 64) + threadID] = tracePt;
            break;
        }
    }
}