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
#define MAX_VIS_MARCHER_STEPS 256

#include "Lighting.hlsli"

// Initial ray direction
float3 PRayDir(float fovRads,
			   float2 viewSizes,
			   uint2 pixID)
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
    // a 2D space ([XY], Z), then scale [XY] by the maximum possible
	// number of values for each Z and fill the space between units
	// of [XY] space with the Z values orthogonal to each left-hand
	// unit
    // Can be defined as a basic linear map with the form
	// ((x + y * W) * Z) + z
	// Previous linearization function was the map
    // (x + y * W) + z * (W * H)
	// This is well-defined and easy to understand, but defines Z with
	// "sheets" of 2D space rather than the "ranks" of 1D space used for
	// in the current function; that's an imperfect model for a system where
	// every 2D value is associated with an orthogonal one-dimensional
	// series of ray samples
    // X: pixel channels
    // Y: pixel rows
    // Z: pixel samples
    uint rayNdx = ((pixID.x + (threadID * DISPLAY_WIDTH)) * GI_SAMPLES_PER_RAY) + groupID.y;

	// Need to adjust to support arbitrary object distances
    float maxRayDist = 16384.0f;
    float currRayDist = 0.0f;

    float3 eyePos = cameraPos.xyz;
    float rayScale = sqrt(GI_SAMPLES_PER_RAY);
    float3 rayDir = PRayDir(0.5 * PI,
                            float2(DISPLAY_WIDTH * rayScale,
                                   DISPLAY_HEIGHT * rayScale), // Scale image to sampling space (x, y * sqrt(SAMPLES))
                            uint2((pixID.x * rayScale) + (groupID.y % rayScale),
                                   pixID.y * rayScale) + (groupID.y % rayScale)); // Re-map pixel positions appropriately (scale and grow by the modulo of the scaling factor)

    float3 rayRGB = float3(0.0f.xxx);
    float3 fauxRayRGB = float3(0.0f.xxx);
    //bool escaping = false;
    //if (escaping) { break; }

    // Define an adaptive epsilon (marching cutoff) for optimal
    // marching performance (no reason to march to within a hundred-thousandth of
    // a surface when the camera is 150 units away anyways)

    // Evaluate average distance to the eye across all figures
    float avgDist = 0;
    for (uint i = 0; i < MAX_NUM_FIGURES; i += 1)
    {
        avgDist += (FigDF(eyePos, figuresReadable[i]).x / MAX_NUM_FIGURES);
    }

    // Initialise an epsilon value to [MARCHER_EPSILON_MIN] and make it twice as
    // large for every five-unit increase in average distance (up to a maximum of
    // [MARCHER_EPSILON_MAX])
    // E.g. a ten-unit distance means a two-times increase (2^1)
    //      a fifty-unit distance means a 1024-times increase (2^10)
    //      a seventy-five-unit distance means a 16384-times increase (2^15)
    //      a hundred-unit distance means a ~1,000,000-times increase (2^20)
    // BUT
    // Epsilons are still thresholded at [MARCHER_EPSILON_MAX], so exponential
    // increases in scaling distance never cause significant ray under-stepping
    // (and thus invalid distances)
    float adaptEps = MARCHER_EPSILON_MIN;
    adaptEps *= pow(10, avgDist / 5.0f);
    adaptEps = min(max(adaptEps, MARCHER_EPSILON_MIN),
                   MARCHER_EPSILON_MAX);

    // March the scene
    for (uint j = 0; j < MAX_VIS_MARCHER_STEPS; j += 1)
    {
        // Transform ray vectors to match the view matrix + eye position
        float3 rayVec = mul(float4(currRayDist * rayDir, 1), viewMat).xyz;
        rayVec += eyePos;

        // Extract distance to the scene + ID value (in the range [0...9])
        // for the closest figure
        float2 sceneField = SceneField(rayVec);

        // Process the intersected surface if appropriate
        fauxRayRGB += float3(0.01f, 0.03f, 0.02f);
        if (sceneField.x < adaptEps)
        {
            // Extract material for the current figure
            FigMat mat = FigMaterial(rayVec,
                                     sceneField.y);

            // Adjust ray direction through the local BRDF
            // Just diffuse reflection for now, too little time to implement a unified reflection model
            //rayDir = DiffuseBRDF(rayNdx, GetNormal(eyePos + rayVec));

            // Apply direct illumination to the current figure
            // Would invoke color function here...
            rayRGB += fauxRayRGB * mat.rgb; //1.0f.xxx; //PerPointDirectIllum(eyePos + rayVec,
                      //                    1.0f.xxx, GetNormal(eyePos + rayVec));
            break;
        }

        // Increase ray distance by the distance to the field (step by the minimum safe distance)
        currRayDist += sceneField.x;

        // Break out if the current ray distance passes the threshold defined by [maxRayDist]
        // Subtraction by epsilon is unneccessary and just provides consistency with the limiting
        // epsilon value used for intersection checks
        if (currRayDist > (maxRayDist - adaptEps))
        {
            // Assume the ray has passed through the scene without touching anything;
            // attempt to estimate an appropriate color before breaking out
            // Replace current assignment with a planetary/atmospheric color function
            // when possible
            rayRGB = 0.0f.xxx;
            //escaping = true;
            break;
        }
    }

    // Write the final ray color to the parallel GI buffer
    TracePix trace;
    trace.pixCoord = float4(pixID, 0.0f.xx);
    trace.pixRGBA = float4(rayRGB, 1.0f);
    giCalcBufWritable[rayNdx] = trace;
}