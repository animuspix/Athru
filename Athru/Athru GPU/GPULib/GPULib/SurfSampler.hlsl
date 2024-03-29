#include "LightingUtility.hlsli"
#ifndef MATERIALS_LINKED
    #include "Materials.hlsli"
#endif

// Per-bounce indirect dispatch axes
// (tracing axes in (0...2), sampling axes in (3...5), input traceable
// count in [6], input sampling count in [7]
RWBuffer<uint> dispAxes : register(u12);

// Append/consume counters for traceables + material primitives
RWBuffer<uint> traceCtr : register(u13);
RWBuffer<uint> diffuCtr : register(u14);
RWBuffer<uint> mirroCtr : register(u15);
RWBuffer<uint> refraCtr : register(u16);
RWBuffer<uint> snowwCtr : register(u17);
RWBuffer<uint> ssurfCtr : register(u18);
RWBuffer<uint> furryCtr : register(u19);

// Buffer carrying intersections across ray-march iterations
// Simply ordered, used to minimize dispatch sizes for
// ray-marching
AppendStructuredBuffer<uint> traceables : register(u20);

// Material intersection buffers
// Can save some performance + simplify header structure by converting these
// back to standard buffers and appending/consuming manually; might implement
// that after/during validation
ConsumeStructuredBuffer<uint> diffuIsections : register(u21);
ConsumeStructuredBuffer<uint> mirroIsections : register(u22);
ConsumeStructuredBuffer<uint> refraIsections : register(u23);
ConsumeStructuredBuffer<uint> snowwIsections : register(u24);
ConsumeStructuredBuffer<uint> ssurfIsections : register(u25);
ConsumeStructuredBuffer<uint> furryIsections : register(u26);

void SampleDiffu(uint ndx, float3x3 normSpace, float3 n, float3 wo,
				 float4 rand, inout PhiloStrm randStrm, float eps,
				 float3 pt, uint figID, float starScale, float2 pix)
{
	// Perform sampling for next-event-estimation
	float4 surf = float4(SurfRGB(wo, n), SurfAlpha(pt, 0u));
	float diffuP = DiffuChance(pt);
	rand = iToFloatV(philoxPermu(randStrm));
	float4 nee = DiffuLiGather(surf,
							   diffuP,
							   n,
							   wo,
							   pt,
							   normSpace,
							   starScale,
							   gpuInfo.systemOri.xyz,
							   figID,
							   eps,
							   rand);
	// Prepare for the next bounce, but only if next-event-estimation fails
	if (nee.a)
	{
		// Sample an incoming ray direction for light-transport
		rand = iToFloatV(philoxPermu(randStrm));
		float4 iDir = DiffuseDir(wo,
								 n,
								 rand.xy);
		iDir.xyz = mul(iDir.xyz, normSpace); // Trace along the bounce direction
		iDir.w /= diffuP; // Scale surface PDFs by selection probability for the chose`n
						  // material primitive

		// Shade with the expected incoming direction
		float3 rgb = DiffuseBRDF(surf,
								 float3x3(iDir.xyz,
								 		  n,
								 		  wo)) / ZERO_PDF_REMAP(iDir.w) *
								 abs(dot(n, -wo));
		float4 rho = displayTex[pix] * float4(rgb, 1.0f);
		displayTex[pix] = rho;
		if (dot(rho.rgb, 1.0f.xxx) > 0.0f) // Only propagate paths with nonzero brightnesss
		{
			// Update current direction + position, also previous position
			// (diffuse bounce, incident and exitant positions are equivalent)
			rays[ndx] = float2x3(pt, iDir.xyz);
			rayOris[ndx] = pt;

			// Update outgoing direction
			outDirs[ndx] = wo;

			// Propagate path
			traceables.Append(ndx);
		}
	}
	else // Otherwise apply next-event-estimated shading into path color
	{ displayTex[pix] *= float4(nee.rgb, 1.0f); } // PDFs already applied for [nee.rgb]
}

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Cache linear thread ID
    uint linDispID = threadID + (groupID.x * 256); // Assumes one-dimensional dispatches from [RayMarch]
    if (linDispID >= dispAxes[7]) { return; } // Mask off excess threads (assumes one-dimensional dispatches)

	// Match thread-groups to material primitives
	uint ctrs[6] = { diffuCtr[0], mirroCtr[0], refraCtr[0],
					 snowwCtr[0], ssurfCtr[0], furryCtr[0] };
	uint matID = 0; // Default to diffuse sampling
	if (linDispID < ctrs[1])
	{ matID = 1; } // Perform mirrorlike sampling
	else if (linDispID < (ctrs[1] + ctrs[2]))
	{ matID = 2; } // Perform refractive sampling
	else if (linDispID < (ctrs[1] + ctrs[2] + ctrs[3]))
	{ matID = 3; } // Sample a snowy subsurface integrator
	else if (linDispID < (ctrs[1] + ctrs[2] + ctrs[3] + ctrs[4]))
	{ matID = 4; } // Sample a generic subsurface scattering integrator
	else if (linDispID < (ctrs[1] + ctrs[2] + ctrs[3] + ctrs[4] + ctrs[5]))
	{ matID = 5; } // Sample a furry subsurface scattering integrator

    // Cache intersection info for the sample point
    uint ndx = 0;
	switch (matID)
	{
		case 0:
	 		ndx = diffuIsections.Consume();
			break;
		case 1:
	 		ndx = mirroIsections.Consume();
			break;
		case 2:
	 		ndx = refraIsections.Consume();
			 break;
		case 3:
	 		ndx = snowwIsections.Consume();
			 break;
		case 4:
	 		ndx = ssurfIsections.Consume();
			 break;
		case 5:
	 		ndx = furryIsections.Consume();
			break;
		default:
            ndx = 0; //abort(); // Material IDs are only well-defined for the range (0...5)
    }
	uint2 pix = uint2(ndx % gpuInfo.resInfo.x, ndx / gpuInfo.resInfo.x);

    // Extract a Philox-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    // Initialize [randBuf] with hashed start times in the first frame
    PhiloStrm randStrm = randBuf[ndx];
    float4 rand = iToFloatV(philoxPermu(randStrm));

    // Cache adaptive epsilon
    float eps = gpuInfo.cameraPos.w;

    // Cache intersections + shift outside the local figure
    float2x3 ray = float2x3(rayOris[ndx],
							rays[ndx][1]);
    float3 pt = ray[0] - ray[1] * eps;

    // Generate + cache the local gradient
    uint figID = figIDs[ndx];
    float3 n = PlanetGrad(pt,
                          figures[figID],
						  eps); // Placeholder gradient, will use figure-adaptive normals later...

    // Generate + cache the local tangent-space matrix
    float3x3 normSpace = NormalSpace(n);

	// Cache ray intersections + local normals on the first bounce
	// (needed for edge-avoiding denoising during presentation)
	if (all(rays[ndx][0] == gpuInfo.cameraPos.xyz))
	{
		posBuffer[ndx] = pt;
		nrmBuffer[ndx] = n;
	}

	// Match an intersected material to the current thread, then sample it
	// Minimal divergence impact because all threads in a group should sample the same material primitive
    Figure star = figures[STELLAR_FIG_ID];
	switch (matID)
	{
		case 0:
			SampleDiffu(ndx, normSpace, n, ray[1],
					    rand, randStrm, eps, pt, figID, star.scale.x,
					    pix); // Perform diffuse sampling
			//displayTex[pix] *= float4(abs(normalize(rayOris[ndx])), 1.0f);
							   //float4(1.0f, 0.5f, 0.25f, 0.125f);
			break;
		case 1:
			displayTex[pix] *= float4(0.8f, 0.0f.xx, 1.0f); // Mirrorlike reflections unimplemented atm, output bright red shading instead
			break;
		case 2:
			displayTex[pix] *= float4(0.0f, 0.8f, 0.0f, 1.0f); // Refractive transmission unimplemented atm, output bright green shading instead
			break;
		case 3:
			displayTex[pix] *= float4(0.0f, 0.0f, 0.8f, 1.0f); // Snowy subsurface scattering unimplemented atm, output bright green shading instead
			break;
		case 4:
			displayTex[pix] *= float4(0.8f, 0.8f, 0.0f, 1.0f); // Generic subsurface scattering unimplemented atm, output bright yellow shading instead
			break;
		case 5:
			displayTex[pix] *= float4(0.0f, 0.8f, 0.8f, 1.0f); // Furry subsurface scattering unimplemented atm, output turquoise shading instead
			break;
		default:
            displayTex[pix] *= 0.0f.xxxx; // Zero shading for bad surface indices

    }

	// Update Philox key/state for the current path
	randBuf[ndx] = randStrm;
}