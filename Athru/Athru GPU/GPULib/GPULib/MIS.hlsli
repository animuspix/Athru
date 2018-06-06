
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
// Incoming/outgoing vertices contain positions in [0].xyz, interface-IDs
// in [0].w, local normals in [1].xyz, and BXDF-IDs in [1].w 
// (when appropriate)
// [dirs] carrys the path's outgoing direction (i.e. the direction
// towards the previous vertex on [outVt]s subpath) in [0] and
// the lens normal (the direction "faced" by the camera) in
// [1] 
float BidirMISPDF(float2x4 inVt,
                  float2x4 outVt,
                  float2x3 dirs,
                  bool lightPath)
{
    uint inIfaceProps = extractIfaceProps(inVt[0].w);
    uint outIfaceProps = extractIfaceProps(outVt[0].w);
    float pdf = 0;
    switch(inVt[0].w)
    {
        // Camera in/out PDFs are defined over area by default, so no
        // need to convert probabilities in that case
        case INTERFACE_PROPS_LENS:
            if (lightPath)
            {
                return CamAreaPDFOut(outVt[0].xyz - inVt[0].xyz,
                                     dirs[1]);
            }
            else
            {
                return CamAreaPDFIn(outVt[0].xyz - inVt[0].xyz,
                                    dirs[1]);
            }
        case INTERFACE_PROPS_LIGHT:
            pdf = StellarPosPDF();
        case INTERFACE_PROPS_MAT:
            pdf = MatPDF(RaysToAngles(normalize(inVt[0].xyz - outVt[0].xyz),
                                      dirs[0],
                                      lightPath),
                         outVt[1].w);
        default: // Assume interfaces represent materials by default
            pdf = MatPDF(RaysToAngles(normalize(inVt[0].xyz - outVt[0].xyz),
                                      dirs[0],
                                      lightPath),
                         outVt[1].w);
    }

    // Camera PDFs return early, so any values for [pdf] are guaranteed to be defined over solid-angle; convert + return
    // those here
    return pdf * AngleToArea(inVt[0].xyz,
                             outVt[0].xyz,
                             outVt[1].xyz,
                             outVt[1].w == BXDF_ID_MEDIA);
}
