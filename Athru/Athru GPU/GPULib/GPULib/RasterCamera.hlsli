
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
               float pixWidth,
               inout float zProj)
{
    float2 viewSizes = (DISPLAY_SIZE_2D * pixWidth);
    zProj = viewSizes.y / tan(FOV_RADS / 2.0f);
    float4 dir = float4(normalize(float3(pixID - (viewSizes / 2.0f),
                                         zProj)), 1.0f);
    return mul(dir, viewMat).xyz;
}

// Small utility function to compute the camera's world-space bounds at [EPSILON_MIN]
// units away from the camera's centroid
float2x3 WorldCamBounds()
{
    // Compute initial hemispheric bounds at [length == 1]
    float pixWidth = sqrt(NUM_AA_SAMPLES);
    float zProj;
    float2x3 minMaxXY = float2x3(PRayDir(0.0f.xx,
                                         pixWidth,
                                         zProj),
                                 PRayDir(float2(DISPLAY_WIDTH - 1,
                                                DISPLAY_HEIGHT - 1),
                                         pixWidth,
                                         zProj));

    // Transform bounds to match the view matrix
    // (is this a matrix or a tensor operation? Should check
    // whether HLSL has an appropriate intrinsic here...)
    minMaxXY[0] = mul(float4(minMaxXY[0], 1.0f), viewMat).xyz;
    minMaxXY[1] = mul(float4(minMaxXY[1], 1.0f), viewMat).xyz;

    // Flatten the the generated bounds into a bounding
    // quadrilateral at [z == EPSILON_MIN], then return the
    // result
    return float2x3(minMaxXY[0] / (minMaxXY[0].z / EPSILON_MIN),
                    minMaxXY[1] / (minMaxXY[1].z / EPSILON_MIN));
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
uint3 PRayPix(float3 incidentDir/*,
              float3 lensNormal*/,
              float zProj) // Standard non-normalized z-value for the chosen FOV
{
    // Flip [incidentDir] into an outgoing camera direction
    // (rather than an incoming light direction), also transform
    // it into baseline camera space + flatten it into the
    // [z = DISPLAY_HEIGHT / tan(FOV_RADS / 2.0f)] focal plane
    incidentDir *= -1.0f;
    //incidentDir = mul(float4(incidentDir, 1.0f), iViewMat).xyz;
    float2 viewSizes = DISPLAY_SIZE_2D; // Ignore multi-sampling for pixel re-projection
    incidentDir /= incidentDir.z / zProj;

    // Pixel ID can be related to direction (within the [z = DISPLAY_HEIGHT / tan(FOV_RADS / 2.0f)]
    // focal plane) with the following definitions; apply those here
    // pixID.x: dir.x + width / 2.0f
    // pixID.y: dir.y + height / 2.0f
    uint2 pixID = incidentDir.xy + (viewSizes / 2.0f);

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
    uint2 baseSSPixID = (pixID * pixWidth) + ((uint)pixWidth / 2).xx;
    float2 sampleXY = uint2(pixSampleNdx % pixWidth,
                            pixSampleNdx / pixWidth);

    // Jitter the selected coordinate
    float jitterRange = pixWidth;
    sampleXY += rayJitter(baseSSPixID + sampleXY,
                          jitterRange,
                          randVal) - (jitterRange / 2).xx;

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

// Compute the importance (bidirectional equivalent to radiance)
// emitted into the scene from some point on the film plane
// (i.e. the display); also compute spatial (chance that a sensor
// ray is exiting from any one point on the lens) and directional
// (chance that a ray is exiting in any one direction) probability
// density functions (PDFs)
// Assumes a pinhole camera (all camera rays are emitted through the
// sensors from a point-like source); the film plane is assumed to
// lie [EPSILON_MIN] units away from the pinhole
// [x] contains importance for the given ray, [y] contains the spatial
// PDF, [z] contains the directional PDF
// Lens normal is known-constant in each frame because we're using a
// pinhole camera; will need to be re-calculated for physically-based
// models
float3 RayImportance(float3 camRayDir,
                     float3 lensNormal)
{
    // Evaluate the squared distance between the given ray and
    // the film plane
    float3 distVec = camRayDir / (camRayDir.z / EPSILON_MIN);
    float dSqr = dot(distVec,
                     distVec);

    // Extract the cosine of the angle between the
    // given ray direction and film plane's surface
    // normal (assumed parallel to local-[z])
    // Thresholded to [R, 0] to guarantee that rays
    // entering from behind the film plane will
    // always record zero importance (ray importance
    // carries a cos^2 function; squaring e.g. -0.1
    // would give +0.01 as the angular contribution
    // even though the incoming ray would have been
    // approaching from behind the lens/pinhole)
    float cosTheta = max(dot(camRayDir, lensNormal), 0.0f);

    // Evaluate the area of the film at [EPSILON_MIN]

    // Model the film area with minimum/maximum keypoints
    float2x3 camBounds = WorldCamBounds();

    // Extract width and height for the film from each
    // axis of the keypoints, then take the area by
    // multiplying them together
    float projArea = (camBounds[1].x - camBounds[0].x) *
                     (camBounds[1].y - camBounds[0].y);

    // Combine the above terms into a returnable importance
    // function
    float cosThetaSqr = cosTheta * cosTheta;
    float rayImportance = (dSqr / (projArea * EPSILON_MIN * cosThetaSqr));

    // Return the generated importance value, as well as it's spatial/directional PDFs
    // (spatial in [y], directional in [z])
    // Possibility to streamline PDF vaues into a simple multiplication and return
    // scalar importance rather than importance + PDFs...
    return float3(rayImportance,
                  1.0f, // All rays exit from the same point in a pinhole camera, so the chance of choosing that point must be 100%
                  dSqr / (projArea * cosTheta));
}

// Evaluate the importance emitted towards the camera from some point in the scene
// Assumes a pinhole camera
// Relies on foundational symmetry in BPT; light and importance are the same, thus
// light reflected into the camera is exactly the same as importance emitted from
// the lens into the scene
// Generated importance value passes into [0][x]; probability that the sample ray
// emitted towards the camera travelled from the given surface point through the eye
// position passes into [0][y]
// Connecting vector (useful for evaluating volumetric transmission/scattering over
// the length of the gather ray) stored in [1][xyz]
float2x3 PerPointImportance(float3 rayVec,
                            float3 eyePos,
                            float3 lensNormal)
{
    // Trace a ray back from [rayVec] towards the camera (i.e. send a gather
    // ray towards the lens), then invert it (as if it had been emitted from the
    // camera in the first place) and evaluate outgoing importance for the given
    // ray direction
    // Possibility to streamline PDF vaues into a simple multiplication and return
    // scalar importance rather than importance + PDFs...
    // Probability of emitted rays hitting any point on the lens is locked to [1]
    // (since the lens is an ideal single-point pinhole), so no need to scale the
    // PDF by [1 / lensArea]
    float3 camVec = eyePos - rayVec;
    float surfDist = length(camVec);
    return float2x3(float3(RayImportance((camVec / surfDist) * -1.0f, lensNormal).x,
                           (surfDist * surfDist) / dot(lensNormal, camVec),
                           0.0f),
                    camVec);
}
