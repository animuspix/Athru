
// -- Rendering resources, from lens sampling through to surface shading -- //
// -- Some resources change access between passes (e.g. from append/ to consume/), so those are bound directly in the relevant .hlsl file -- //

#define RENDER_BINDS_LINKED

// Rendering passes inherit generic resources
#include "GenericBinds.hlsli"

// Rendering-specific input from the CPU
struct RenderInput
{
    float4 cameraPos; // Camera position in [xyz], [w] is unused
    matrix viewMat;
    matrix iViewMat;
    float4 bounceInfo; // Maximum number of bounces in [x] (integer), path cancellation chance in [x] (real), number of supported surface BXDFs in [y] (integer),
					   // tracing epsilon value in [z] (real), number of valid threads per-pass in [z] (integer), number of patches (groups) per-axis deployed
					   // for path tracing in [w] (integer) ([w]/real is unused)
    uint4 resInfo; // Resolution info carrier; contains app resolution in [xy],
	               // AA sampling rate in [z], and display area in [w]
	uint4 tilingInfo; // Tiling info carrier; contains spatial tile counts in [x]/[y] and cumulative tile area in [z] ([w] is unused)
	uint4 tileInfo; // Per-tile info carrier; contains tile width/height in [x]/[y] and per-tile area in [z] ([w] is unused)
};
ConstantBuffer<RenderInput> rndrInfo : register(b1);

// State buffer for the rendering-specific random number generator
// (streamed data is used for physics/ecology RNG, which could require much more information)
RWStructuredBuffer<PhiloStrm> randBuf : register(u0);

// Exitant (current) position + input direction for rays at emission/each bounce
RWStructuredBuffer<float2x3> rays : register(u1);

// Incident (previous) position + output direction at each bounce
RWBuffer<float3> rayOris : register(u4);
RWBuffer<float3> outDirs : register(u5);

// Reflective color + roughness at each bounce
RWBuffer<float4> surfRGBA : register(u6);

// Most recent index of refraction per-bounce
RWBuffer<float> iors : register(u7);

// Figure IDs at the last intersection, used for figure-specific gradients + material queries
// during shading/bounce preparation
RWBuffer<uint> figIDs : register(u8);

// Write-allowed reference to the display texture
// (copied to the display each frame on rasterization, carries colors in [rgb] and
// filter values + pixel indices in [w])
RWTexture2D<float4> displayTex : register(u9);

// Samples + counter variables for each pixel
// (needed for temporal smoothing/AA)
struct PixHistory
{
    float4 currSampleAccum; // Current + incomplete set of [NUM_AA_SAMPLES] samples/filter-coefficients
    float4 prevSampleAccum; // Accumulated + complete set of [NUM_AA_SAMPLES] samples/filter-coefficients
    uint4 sampleCount; // Cumulative number of samples at the start/end of each frame
    float4 incidentLight; // Incidental samples collected from light paths bouncing off the
                          // scene before reaching the lens; exists because most light
                          // sub-paths enter the camera through pseudo-random sensors rather
                          // than reaching the sensor (i.e. thread) associated with their
                          // corresponding camera sub-path
                          // Separate to [sampleAccum] to prevent simultaneous writes during
                          // path filtering/anti-aliasing
};

// Buffer carrying anti-aliasing image samples
// (Athru AA is performed over time rather than space to
// reduce per-frame GPU loading)
RWStructuredBuffer<PixHistory> aaBuffer : register(u10);