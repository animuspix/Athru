
// -- Rendering resources, from lens sampling through to surface shading -- //
// -- Some resources change access between passes (e.g. from append/ to consume/), so those are bound directly in the relevant .hlsl file -- //

#define RENDER_BINDS_LINKED

// Rendering passes inherit generic resources
#ifndef GENERIC_BINDS_LINKED
	#include "GenericBinds.hlsli"
#endif

// State buffer for the rendering-specific random number generator
// (streamed data is used for physics/ecology RNG, which could require much more information)
RWStructuredBuffer<PhiloStrm> randBuf : register(u0);

// Exitant (current) position + input direction for rays at emission/each bounce
RWStructuredBuffer<float2x3> rays : register(u1);

// Incident (previous) position + output direction at each bounce
RWStructuredBuffer<float3> rayOris : register(u2);
RWStructuredBuffer<float3> outDirs : register(u3);

// Most recent index of refraction/absorption per-bounce
RWStructuredBuffer<float2> iors : register(u4);

// Figure IDs at the last intersection, used for figure-specific gradients + material queries
// during shading/bounce preparation
RWBuffer<uint> figIDs : register(u5);

// Position & normal buffers for edge-avoiding wavelet a-trous denoising
// Denoising implemented from this paper: 
// http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.672.3796&rep=rep1&type=pdf
// and this shader: 
// https://www.shadertoy.com/view/ldKBzG
RWBuffer<float3> posBuffer : register(u6);
RWBuffer<float3> nrmBuffer : register(u7);

// Write-allowed reference to the display texture
// (copied to the display each frame on rasterization, carries colors in [rgb] and
// filter values + pixel indices in [w])
RWTexture2D<float4> displayTex : register(u8);

// 8bpp back-buffer proxy, receives tonemapped data from [displayTex]; used because
// the format used with [displayTex] is incompatible with the back-buffer
// (R32G32B32A32_FLOAT vs. R8G8B8A8_UNORM)
RWTexture2D<float4> displayTexLDR : register(u9);

// Same area + dynamic range as [displayTexLDR], but mapped to a buffer resource instead of a
// texture
RWBuffer<float4> screenshotBuf : register(u10);

// Samples + counter variables for each pixel
// (needed for temporal smoothing/AA)
struct PixHistory
{
    float4 currSampleAccum; // Current + incomplete set of [NUM_AA_SAMPLES] samples/filter-coefficients
    uint4 sampleCount; // Cumulative number of samples at the start/end of each frame
};

// Buffer carrying anti-aliasing image samples
// (Athru AA is performed over time rather than space to
// reduce per-frame GPU loading)
RWStructuredBuffer<PixHistory> aaBuffer : register(u11);

// 128x128 tiled blue-noise dither texture for lens sampling; separates LDS iterations for different pixels
// to allow convergent sampling without sacrificing parallelism
// Part of a collection of blue-noise textures licensed to the public domain by Moments In Graphics; the original release
// is available here :
// http://momentsingraphics.de/?p=127
Texture2D<float2> ditherTex : register(t1);
