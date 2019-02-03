
#ifndef UTILITIES_LINKED
    #include "GenericUtility.hlsli"
#endif

// Small #define tag to avert multiple inclusions
#define RENDER_UTILITY_CORE_LINKED

// Rendering-specific input from the CPU
cbuffer RenderInput : register(b1)
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
}

// Concrete representation for light bounces after/during ray-marching;
// allows persistent path state during rendering (which occurs across
// multiple shading stages for performance)
struct LiBounce
{
	uint3 id; // Path ID for the current bounce (needed for shading); one-dimensional pixel
			  // offset in [x], two-dimensional index in [yz], [w] is unused
	float4x3 dirs; // Incoming/normal/outgoing/half
	float3 iP; // Incident position
	float3 eP; // Exitant position (usually equivalent to incident position,
			   // variant for volumetric materials)
	float4x4 mat; // Local material properties (including microfacet normal)
	uint figID; // Figure-ID at intersection
    uint dfType; // Distance-function type at intersection
	float4 gatherRGB; // Weighted (but not shaded) contribution from source-sampled
					  // next-event-estimation; includes gather-ray PDF in [w]
	float4 gatherOcc; // Success value for source sampling in [x] (since checking [[float] == 0.0f]
					  // can be funky), sample direction in [yzw]
	float2 etaIO; // Incident/outgoing indices-of-refraction
    // Philox strean for randomness during material synthesis (too few UAV bindings to #include
    // [randBuf] as well as material buffers)
    PhiloStrm randStrm;
};

// Initializer for [LiBounce]
LiBounce LiBounceInitter(uint3 id, float3 oDir,
						 float3 camPos, PhiloStrm philo)
{
	LiBounce b;
	b.id = id; // Assign ID
	b.dirs = float4x3(oDir, oDir,
					  oDir, oDir); // Initialize all [dirs] to the outgoing camera vector
	b.iP = camPos; // Initialize positions to the camera coordinates
	b.eP = camPos;
	b.mat = float4x4(0.0f.xxxx,
                     0.0f.xxxx,
                     0.0f.xxxx,
                     0.0f.xxxx);
	b.figID = 0; // Zero other variables
    b.dfType = 0;
	b.gatherRGB = 0.0f.xxxx;
	b.gatherOcc = 0.0f.xxxx;
	b.etaIO = 0.0f.xx;
    b.randStrm = philo;
	return b;
}

// Generic counter buffer, carries dispatch axis sizes per-material in 0-17,
// generic axis sizes in 18-20, thread count assumed for dispatch axis sizes
// in [21], and a light bounce counter in [22]
// Also raw generic append-buffer lengths in [23], and material append-buffer
// lengths in 24-29
RWBuffer<uint> counters : register(u4);
