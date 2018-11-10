
#ifndef UTILITIES_LINKED
    #include "GenericUtility.hlsli"
#endif
#include "Materials.hlsli"

// Rendering-specific input from the CPU
cbuffer RenderInput
{
    float4 cameraPos;
    matrix viewMat;
    matrix iViewMat;
    uint2 bounceInfo; // Maximum number of bounces in [x], number of supported surface BXDFs in [y],
					  // nothing in [zw]
    uint4 resInfo; // Resolution info carrier; contains app resolution in [xy],
	               // AA sampling rate in [z], and display area in [w]
}

// Concrete representation for light bounces after/during ray-marching;
// allows persistent path state during rendering (which occurs across
// multiple shading stages for performance)
struct LiBounce
{
	uint id; // Path ID for the current bounce (needed for shading)
	float4x3 dirs; // Incoming/normal/outgoing/half
	float3 iP; // Incident position
	float3 eP; // Exitant position (usually equivalent to incident position, 
			   // variant for volumetric materials)
	float4x4 mat; // Local material properties (including microfacet normal)
	uint figID; // Figure-ID at intersection
	float4 gatherRGB; // Weighted (but not shaded) contribution from source-sampled 
					  // next-event-estimation; includes gather-ray PDF in [w]
	float4 gatherOcc; // Success value for source sampling in [x] (since checking [[float] == 0.0f]
					  // can be funky), sample direction in [yzw]
	float2 etaIO; // Incident/outgoing indices-of-refraction
	float3 rho; // Reflected surface color (only defined after shading)
	float pdf; // Probability of rays bouncing through sampled incoming directions
};

// Light bounces collect into paths defined per-pixel
struct LiPath
{
	PhiloStrm randStrm; // Stream state for the current path at the current bounce
	float4 camInfo; // Camera ray-direction in [xyz], filter value in [w]
	uint bounceCtr; // Prefer incrementing this with [InterlockedAdd(...)]
	float3 voluRGB; // Accumulated color from volumetric scattering along the path
	LiBounce vts[MAX_NUM_BOUNCES]; // Bounces/vertices contributing to [this]
};

// Constructor for [LiPath]
LiPath LiPathBuilder(PhiloStrm randStrm,
                     float4 pRayInfo,
                     float3 camPos)
{
    LiPath liPath;
    liPath.randStrm = randStrm;
    liPath.camInfo = camInfo;
    liPath.bounceCtr = 0u;
    liPath.mediaRGB = 0.0f.xxx;
    liPath.vts[0].iP = camPos;
    liPath.vts[0].eP = camPos;
    return liPath;
}

// Light-path buffer, allows efficient per-pixel integration between shading stages
RWStructuredBuffer<LiPath> pathBuffer : register(u2);