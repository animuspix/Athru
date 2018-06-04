
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

//float MIS