
#ifndef LIGHTING_UTILITIES_LINKED
	#include "LightingUtility.hlsli"
#endif
#ifndef RASTER_CAMERA_LINKED
	#include "RasterCamera.hlsli"
#endif
#ifndef MATERIALS_LINKED
    #include "Materials.hlsli"
#endif

// Evaluate the power heuristic for Multiple Importance Sampling (MIS);
// used to balance importance-sampled radiances taken with different
// sampling strategies (useful for e.g. sampling arbitrary materials
// where some materials (like specular surfaces) respond better to
// BSDF sampling whereas others (like diffuse surfaces) respond better
// if you sample light sources directly)
// This implementation is from Physically Based Rendering: From Theory
// to Implementation (Pharr, Jakob, Humphreys)
// We're working in high-performance shader code, so variadic functions
// aren't really a thing; that means this is only good for sampling
// strategies with exactly two distributions. Strategies with arbitrary
// distributions (like e.g. BDPT vertex integration) should use
// hard-coded MIS implementations instead
float MISWeight(uint samplesDistroA,
                float distroAPDF,
                uint samplesDistroB,
                float distroBPDF)
{
    float distroA = samplesDistroA * distroAPDF;
    float distroB = samplesDistroB * distroBPDF;
    float distroASqr = distroA * distroA;
    float distroBSqr = distroB * distroB;
    return distroASqr / (distroASqr * distroBSqr);
}

// Bi-directional MIS-specific PDF function; evaluates probabilities for
// arbitrary vertices on lens, scene, or emissive interfaces
// Incoming/outgoing vertices contain positions in [0].xyz,
// distance-functions in [0].w, local normals in [1].xyz, and BXDF-IDs
// in [1].w (when appropriate)
// [dirs] carrys the path's outgoing direction (i.e. the direction
// towards the previous vertex on [outVt]s subpath) in [0] and the lens
// normal (the direction "faced" by the camera) in [1]
float BidirMISPDF(float2x4 inVt,
                  float2x4 outVt,
                  float2x3 dirs,
                  float3 ioInfo, // Input figure ID in [x], output figure ID in [y],
                                 // mollification radius (for specular interactions)
                                 // in [z]
                                 // Kaplanyan and Dachsbacher discuss an extra weight for
                                 // BDPT that only mollifies the most distant vertices in
                                 // the shortest candidate paths, but I wanted to avoid
                                 // the preprocessing cost to discover those paths ahead
                                 // of time; Athru just uses the standard mollification
                                 // bandwidth and allows bias to fall over time (as the
                                 // the bandwidth shrinks) instead
                  bool2 pathInfo) // Whether integration is occurring on the light path (x) +
                                  // whether to mollify specular interactions (y)
{
    float pdf = 0;
    bool cvtToArea = true;
    switch(outVt[0].w)
    {
        // Camera in/out PDFs are defined over area by default, so no
        // need to convert probabilities in that case
        case DF_TYPE_LENS:
            if (pathInfo.x)
            {
                pdf = CamAreaPDFOut();
            }
            else
            {
                pdf = CamAreaPDFIn(outVt[0].xyz - inVt[0].xyz,
                                   dirs[1]);
            }
            cvtToArea = false;
            break;
        case DF_TYPE_STAR:
            pdf = StellarPosPDF();
            break;
        default: // Assume non-star, non-lens interfaces are valid scene materials
            float3 connVec = (inVt[0].xyz - outVt[0].xyz);
            float ioDist = length(connVec);
            float2x3 ioDirs = float2x3(connVec / ioDist,
                                       dirs[0]);
            pdf = MatPDF(ioDirs,
                         float4(outVt[0].xyz,
                                pathInfo.x),
                         float4(outVt[1].w,
                                float2(outVt[0].w,
                                       ioInfo.y),
                                false));
            if (pathInfo.y)
            {
                if (outVt[1].w == BXDF_ID_MIRRO ||
                    outVt[1].w == BXDF_ID_REFRA)
                {
                    // Mollify reflection at [outVt]
                    pdf = PSRMollify(ioInfo.z,
                                     ioDirs,
                                     ioDist);
                }
            }
            break;
    }

    // Convert [pdf] to a probability over area if appropriate
    if (cvtToArea)
    {
        return pdf * AngleToArea(inVt[0].xyz,
                                 outVt[0].xyz,
                                 outVt[1].xyz,
                                 outVt[1].w == BXDF_ID_VOLUM);
    }
    else
    {
        return pdf;
    }
}
