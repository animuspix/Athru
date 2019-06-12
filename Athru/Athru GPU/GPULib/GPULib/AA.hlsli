
// Only include GPU utilities if they haven't already
// been included by another header
#ifndef RENDER_BINDS_LINKED
    #include "RenderBinds.hlsli"
#endif

// Small flag to mark whether [this] has already been
// included somewhere in the build process
#define ANTI_ALIASING_LINKED

// Texture giving approach velocity for the data imaged at each pixel; data quickly moving away
// from the camera are sampled less than static data to minimize ghosting (controllable with a motion-blur value
// interpolating between maximum & scaled sampling rates)
//RWTexture2D<float4> veloTex : register(u3);

// Smooths frames by applying temporal anti-aliasing (sample accumulation occurs in
// time and space simultaneously); filtering is supported by applying the generalized
// sample integrator ([x, y] are sample coordinates, [L] is the radiance function,
// [f] is the filter function)
//
// [Sum(f(xRelative, yRelative) * L(x, y))]
// div
// [Sum(f(xRelative, yRelative))]
//
// If we were using the box filter, this would degenerate into a simple mean:
// [Sum(L(x, y))]
// div
// [NumSamples]
//
// The values described by [f(xRelative, yRelative)] (the filter coefficients) are
// generated and stored at the start of every frame
float3 FrameSmoothing(float3 rgb,
                      float filtCoeff,
                      uint linPixID,
                      uint numAASamples)
{
    // Cache a local copy of the given pixel's smoothing history
    PixHistory pixHistory = aaBuffer[linPixID];

    // Update the given pixel's sample count
    uint sampleCount = pixHistory.sampleCount.x + 1;
    uint sampleNdx = pixHistory.sampleCount.x % numAASamples;

    // Update accumulated filter values for the current sample-set
    // Also filter [rgb] with the given filter-coefficient, then
    // accumulate it into the current sample-set
    float4 currAccum = float4(pixHistory.currSampleAccum.rgb + (rgb * filtCoeff),
                              pixHistory.currSampleAccum.w + filtCoeff);

    // Check if the (un-updated) sample-count is exactly modulo-[numAASamples] and update
    // [prevSampleAccum] (+ reset [currSampleAccum]) if appropriate
    float4 prevAccum = pixHistory.prevSampleAccum;
    if (sampleCount % numAASamples == 0 &&
        sampleCount != 0)
    {
        currAccum -= pixHistory.currSampleAccum;
        prevAccum = pixHistory.currSampleAccum;
    }
    else if (sampleCount < numAASamples)
    {
        // Prevent current/previous sample blending until the previous sample is well-defined
        // (i.e. until at least one full set of [numAASamples]-samples have been computed,
        // elided from [currSampleAccum], and passed into [prevSampleAccum])
        prevAccum = pixHistory.currSampleAccum;
    }

    // Interpolate between the most-recent/current sample-sets/filter-coefficients
    // Only apply interpolation to the displayed color, not the sample actually cached
    // in [aaBuffer]
    float4 retRGBW = lerp(prevAccum, currAccum, ((float)sampleCount / (float)numAASamples) % 1.0f);

    // Pass the updated pixel history back into the AA buffer
    // (also update the pixel's sample-set to include the current sample)
    aaBuffer[linPixID].currSampleAccum = currAccum;
    aaBuffer[linPixID].prevSampleAccum = prevAccum;
    aaBuffer[linPixID].sampleCount = sampleCount.xxxx;

    // Return the updated sample
    // Some filter functions can push average values below zero; counter this clamping out negative channels
    // with [max(...)]
    return max(retRGBW.rgb / retRGBW.w, 0.0f.xxx);
}

// Small functions returning the Blackman-Harris filter coefficient
// for a given 2D point
// Blackman-Harris interpolates sparse samples much more effectively
// than naive filtering (=> applying a box filter), allowing for more
// effective anti-aliasing at lower sampling rates
// Core Blackman-Harris filter
float BlackmanHarrisPerAxis(float filtVal,
                            uint numAASamples)
{
    // Blackman-Harris alpha parameters
    // Parameter values taken from:
    // https://en.wikipedia.org/wiki/Window_function#Blackman�Harris_window
    const float alph0 = 0.35875f;
    const float alph1 = 0.48829f;
    const float alph2 = 0.14128f;
    const float alph3 = 0.01168f;

    // Core Blackman-Harris filter function
    const float constFract = (PI * filtVal) / (numAASamples - 1);
    return alph0 - (alph1 * cos(2.0f * constFract)) +
                   (alph2 * cos(4.0f * constFract)) -
                   (alph3 * cos(6.0f * constFract));
}

// Wrapper function applying Blackman-Harris to the given sampling point within the
// given sampling radius
float BlackmanHarris(float2 sampleXY,
                     float sampleRad,
                     uint numAASamples)
{
    float2 filtAxes = abs(sampleXY);
    return (BlackmanHarrisPerAxis(filtAxes.x, numAASamples) *
            BlackmanHarrisPerAxis(filtAxes.y, numAASamples));
}