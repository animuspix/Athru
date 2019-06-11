#include "LightingUtility.hlsli"
#ifndef MATERIALS_LINKED
    #include "Materials.hlsli"
#endif

// Per-bounce indirect dispatch axes
// (tracing axes in (0...2), sampling axes in (3...5), input traceable
// count in [6], input sampling count in [7]
RWBuffer<uint> dispAxes : register(u9);

// Append/consume counters for traceables + material primitives
RWBuffer<uint> traceCtr : register(u10);
RWBuffer<uint> diffuCtr : register(u11);
RWBuffer<uint> mirroCtr : register(u12);
RWBuffer<uint> refraCtr : register(u13);
RWBuffer<uint> snowwCtr : register(u14);
RWBuffer<uint> ssurfCtr : register(u15);
RWBuffer<uint> furryCtr : register(u16);

// Buffer carrying intersections across ray-march iterations
// Simply ordered, used to minimize dispatch sizes for
// ray-marching
AppendStructuredBuffer<uint> traceables : register(u17);

// Material intersection buffers
// Can save some performance + simplify header structure by converting these
// back to standard buffers and appending/consuming manually; might implement
// that after/during validation
ConsumeStructuredBuffer<uint> diffuIsections : register(u18);
ConsumeStructuredBuffer<uint> mirroIsections : register(u19);
ConsumeStructuredBuffer<uint> refraIsections : register(u20);
ConsumeStructuredBuffer<uint> snowwIsections : register(u21);
ConsumeStructuredBuffer<uint> ssurfIsections : register(u22);
ConsumeStructuredBuffer<uint> furryIsections : register(u23);

void SampleDiffu(uint ndx, float3x3 normSpace, float3 n, float3 wo,
				 float4 rand, float eps, float3 pt, uint figID, float starScale,
				 float2 pix)
{
	// Perform sampling for next-event-estimation
	float4 surf = float4(SurfRGB(wo, n), SurfAlpha(pt, 0u));
	float diffuP = DiffuChance(pt);
	float4 nee = DiffuLiGather(DiffuseDir(wo, n, rand.xy) / float4(1.0f.xxx, diffuP),
							   surf,
							   n,
							   wo,
							   pt,
							   StellarPosPDF(),
							   normSpace,
							   starScale,
							   gpuInfo.systemOri.xyz,
							   figID,
							   eps,
							   rand.zw);
	//float4 iDir = DiffuseDir(wo,
	//						 n,
	//						 rand.xy);
	//iDir.xyz = mul(iDir.xyz, normSpace); // Trace along the bounce direction
	//iDir.w /= diffuP; // Scale surface PDFs by selection probability for the chosen
	//				  // material primitive
	//rays[ndx] = //float2x3(pt, -normalize(pt));//iDir.xyz);
	//			float2x3(pt, iDir.xyz);
	//displayTex[pix] *= float4(dot(n, -wo).xxx, 1.0f);
	//traceables.Append(ndx);
	//return;
	// Prepare for the next bounce, but only if next-event-estimation fails
	if (!nee.a)
	{
		// Sample an incoming ray direction for light-transport
		float4 iDir = DiffuseDir(wo,
								 n,
								 rand.zw);
		iDir.xyz = mul(iDir.xyz, normSpace); // Trace along the bounce direction
		iDir.w /= diffuP; // Scale surface PDFs by selection probability for the chosen
						  // material primitive

		// Shade with the expected incoming direction
		float3 rgb = DiffuseBRDF(surf,
								 float3x3(iDir.xyz,
								 		  n,
								 		  wo)) / ZERO_PDF_REMAP(iDir.w);// *
								 //abs(dot(n, -wo));
		displayTex[pix] *= float4(rgb, 1.0f);
		if (dot(rgb, 1.0f.xxx) > 0.0f) // Only propagate paths with nonzero brightnesss
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
    uint ndx;
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
			abort(); // Material IDs are only well-defined for the range (0...5)
	}	
	uint2 pix = uint2(ndx % gpuInfo.resInfo.x, ndx / gpuInfo.resInfo.x);

    // Extract a Philox-permutable value from [randBuf]
    // Also cache the accessor value used to retrieve that value in the first place
    // Initialize [randBuf] with hashed start times in the first frame
    PhiloStrm randStrm = randBuf[ndx];
    float4 rand = iToFloatV(philoxPermu(randStrm));

    // Cache adaptive epsilon
    float eps = gpuInfo.bounceInfo.z;

    // Cache intersections + shift outside the local figure
    float2x3 ray = float2x3(rayOris[ndx],
							rays[ndx][1]);
    float3 pt = ray[0];
    #ifndef APPROX_PLANET_GRAD
        pt = PtToPlanet(pt, figures[figIDs[ndx]].scale.x),
    #endif
    pt -= ray[1] * eps;

    // Generate + cache the local gradient
    Figure star = figures[STELLAR_FIG_ID];
    uint figID = figIDs[ndx];
    float3 n = PlanetGrad(pt,
                          figures[figID]); // Placeholder gradient, will use figure-adaptive normals later...

    // Generate + cache the local tangent-space matrix
    float3x3 normSpace = NormalSpace(n);

	// Match an intersected material to the current thread, then sample it
	// Minimal divergence impact because all threads in a group should sample the same material primitive
	switch (matID)
	{
		case 0:
			SampleDiffu(ndx, normSpace, n, ray[1],
					    rand, eps, pt, figID, star.scale.x,
					    pix); // Perform diffuse sampling
			//displayTex[pix] *= float4(1.0f, 0.5f, 0.25f, 0.125f);
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
			abort(); // Material IDs are only well-defined for the range (0...5)
	}

	// Update Philox key/state for the current path
	randBuf[ndx] = randStrm;
}