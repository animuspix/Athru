
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif
#ifndef ANTI_ALIASING_LINKED
    #include "AA.hlsli"
#endif

// Small flag to mark whether [this] has already been included somewhere
// in the build process
#define RASTER_CAMERA_LINKED

// Simple function to generate clean, non-correlated, well-distributed
// primary ray jitter
float2 rayJitter(uint2 aaPixID,
                 float jitterRange,
                 inout uint randVal)
{
    // Each pixel is assigned one of [jitterRange] nearby samples
    // (in a [sqrt(jitterRange) * sqrt(jitterRange)] box)
    return float2(iToFloat(xorshiftPermu1D(randVal)) * jitterRange,
                  iToFloat(xorshiftPermu1D(randVal)) * jitterRange);
}

// Ray direction for a pinhole camera (simple tangent-driven
// pixel projection), given a pixel ID + a two-dimensional
// resolution vector + a super-sampling scaling factor
// Allows for ray projection without the
// filtering/jitter applied by [PixToRay(...)]
#define FOV_RADS 0.5 * PI
float3 PRayDir(uint2 pixID,
               uint pixWidth,
               inout float zProj)
{
    float2 viewSizes = DISPLAY_SIZE_2D * pixWidth;
    zProj = viewSizes.y / tan(FOV_RADS / 2.0f);
    float4 dir = float4(normalize(float3(pixID - (viewSizes / 2.0f),
                                         zProj)), 1.0f);
    return mul(dir, viewMat).xyz;
}

// Exactly equivalent to [PRayDir(...)] (see above), but returns a referentially-transparent
// value without any hidden writes to [zProj]; also takes [zProj] as a pre-defined value
// for efficiency (avoids one division + one tangent function)
float3 safePRayDir(uint2 pixID,
                   uint pixWidth,
                   float zProj)
{
    float2 viewSizes = DISPLAY_SIZE_2D * pixWidth;
    return mul(float4(normalize(float3(pixID - (viewSizes / 2.0f),
                                       zProj)), 1.0f), viewMat).xyz;
}

// Inverse of [PRayDir] (see above); transforms a given world-space
// (but camera-relative) direction into a rasterizer-friendly pixel
// coordinate (needed if we want to cache the arbitrarily-oriented
// importance gathers traced during the [t == 1] BDPT connection
// strategy)
// Returns 2D pixel ID in [xy] and a linearized equivalent in [z]
// Assumes a pinhole camera, also relies on the [cosTheta(...)] term
// in [PerPointImportance(...)] to filter out rays placed behind the
// camera
uint3 PRayPix(float3 incidentDir,
              float zProj) // Standard non-normalized z-value for the chosen FOV
{
    // Flip [incidentDir] into an outgoing camera direction
    // (rather than an incoming light direction), also transform
    // it into baseline camera space + flatten it into the
    // [z = DISPLAY_HEIGHT / tan(FOV_RADS / 2.0f)] focal plane
    incidentDir *= -1.0f;
    incidentDir /= incidentDir.z / zProj;

    // Pixel ID can be related to direction (within the [z = DISPLAY_HEIGHT / tan(FOV_RADS / 2.0f)]
    // focal plane) with the following definitions; apply those here
    // pixID.x: dir.x + width / 2.0f
    // pixID.y: dir.y + height / 2.0f
    uint2 pixID = incidentDir.xy + (DISPLAY_SIZE_2D / 2.0f); // Ignore multi-sampling for pixel re-projection

    // Return the generated pixel ID in [xy] + a linearized version in [z]
    // Nothing stopping this from returning coordinates outside the actual
    // display size; should validate returned positions in [SceneVis.hlsl]
    // before sending radiance/importance values to the display texture
    return uint3(pixID,
                 pixID.x + (pixID.y * DISPLAY_WIDTH));
}

// Initial ray direction + filter value
// Assumes a pinhole camera (all camera rays are emitted through the
// sensors from a point-like source)
// Direction returns to [xyz], filter coefficient passes into [w]
float4 PixToRay(uint2 pixID,
                uint pixSampleNdx,
                out float zProj,
                inout uint randVal)
{
    // Ordinary pixel projection (direction calculated as the
    // path taken by a ray leaving a point and approaching the
    // given pixel grid-cell), with temporal super-sampling +
    // ray-jitter applied for basic anti-aliasing

    // Lock the sample index to the interval [0...NUM_AA_SAMPLES]
    pixSampleNdx %= NUM_AA_SAMPLES;

    // Convert the sample index into a super-sampled pixel coordinate
    float pixWidth = sqrt(NUM_AA_SAMPLES);
    uint2 baseSSPixID = (pixID * pixWidth) + ((uint)pixWidth / 2.0f).xx;
    float2 sampleXY = uint2((float)pixSampleNdx % pixWidth,
                            (float)pixSampleNdx / pixWidth);

    // Jitter the selected coordinate
    float jitterRange = pixWidth;
    sampleXY += rayJitter(baseSSPixID + sampleXY,
                          jitterRange,
                          randVal) - (jitterRange / 2.0f).xx;

    // Restrict samples to the domain of the current pixel
    sampleXY %= pixWidth.xx;

    // Generate + return a ray direction for the super-sampled pixel coordinate
    // Also generate a filter coefficient for the current sample
    // Filter coefficient determines how samples will be blended into final pixel
    // values for post-processing
    return float4(PRayDir(baseSSPixID + sampleXY,
                          pixWidth,
                          zProj),
                  BlackmanHarris(sampleXY,
                                 length((pixWidth).xx)));
}

// Evaluate probability of a ray leaving from any one
// pixel (sensor) in the camera
#define LENS_AREA 1.0f
float CamAreaPDFOut()
{
    return 1.0f / LENS_AREA;
}

// Evaluate probability of a ray following any one
// direction as it leaves the camera
// Similar to spatial probability, but not identical
// [camInfo] expects the lens normal in [xyz] and the
// camera's z-scale in [w]
float CamDirPDFOut(float3 dir, // Outgoing ray direction
                   float4 camInfo,
                   float sensArea) // Area of the sensor plane at [z == zProj])
{
    float cosTheta = max(dot(camInfo.w, dir), 0.0f);
    float3 distVec = dir / (dir.z / camInfo.w) / cosTheta;
    float distSqr = dot(distVec,
                        distVec);
    return distSqr / (sensArea * cosTheta * cosTheta);
}

// Evaluate probability of a ray entering the camera
// through a differential area on the lens surface
// We're using a pinhole camera (i.e. a differential
// lens), so we only have one possible entrance area
// to think about; the remaining factors (squared
// distance, cosine attenuation) exist to match ray
// probabilities to the perspective-projection we
// use in [PixToRay(...)]
float CamAreaPDFIn(float3 inVec,
                   float3 lensNormal)
{
    return dot(inVec, inVec) * // Evaluate squared length
           max(dot(lensNormal, inVec), 0.0f) / // Apply cosine attenuation (more rays closer to the lens normal)
           LENS_AREA; // Account for even distribution over the lens area ([1.0f] for pinhole cameras)
}

// Small utility function to retrieve the area of the camera's sensor plane given
// an anti-aliasing pixel width + the
float SensArea(float2 sensInfo)
{
    // Generate vectors passing through upper-right/lower-left corners
    // of the film plane
    float2x3 boundVecs = float2x3(safePRayDir(DISPLAY_SIZE_2D - 1.0f.xx,
                                              sensInfo.x, sensInfo.y),
                                  safePRayDir(0.0f.xx,
                                              sensInfo.x, sensInfo.y));

    // Take the bounding vectors to the plane where [z == [zProj]]
    // (projection here is *technically* arbitrary, but using [zProj] makes everything
    // look more consistent anyway :P)
    boundVecs /= float2x3(boundVecs[0].zzz / sensInfo.yyy,
                          boundVecs[1].zzz / sensInfo.yyy);

    // Evaluate the area of sensor plane at the given z-scale ([zProj]
    // here, but unit projection would work too)
    return abs((boundVecs[0].x - boundVecs[1].x) * // Width from difference on [x]
               (boundVecs[0].y - boundVecs[1].y)); // Height from difference on [y]
}

// Compute the importance (bidirectional equivalent to radiance)
// emitted by the camera through the given direction
// Assumes a pinhole camera (all camera rays are emitted through the
// sensors from a point-like source)
// [camInfo] carries the sensor-plane's normal in [xyz] and the focal
// depth (i.e. the z-value used for perspective projection) in [w];
// both values are known-constant in each frame because we're using a
// pinhole camera (i.e. a uniform lens), but they'd need to be
// re-evaluated per-ray for physically-based models
float3 RayImportance(float3 camRayDir,
                     float4 camInfo,
                     float pixWidth)
{
    // Extract the cosine of the angle between the given ray direction and sensor plane's
    // surface normal (assumed parallel to local-[z])
    // Thresholded to [R, 0] to guarantee that rays entering from behind the film plane
    // will always record zero importance (ray importance carries a cos^2 function;
    // squaring e.g. -0.1 would give +0.01 as the angular contribution even though the
    // incoming ray would have been approaching from behind the lens/pinhole)
    float cosTheta = max(dot(camInfo.xyz, camRayDir), 0.0f);

    // Generate + return an importance value for the given outgoing direction
    float cosThetaQrt = cosTheta * cosTheta * cosTheta * cosTheta;
    return (1.0f / (SensArea(pixWidth) * cosThetaQrt)).xxx;
}
