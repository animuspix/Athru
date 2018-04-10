
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif

// Enum flags for different types of
// reflectance/scattering/transmittance function; one of
// these is semi-randomly selected for each ray based
// on weights given by [FigMat]
#define BXDF_ID_DIFFUSE 0
#define BXDF_ID_SPECULAR 1
#define BXDF_ID_TRANSMITTANT 2
#define BXDF_ID_VOLU 3

// A small object used to store procedural material
// definitions during path-tracing
struct FigMat
{
    // x is diffuse weighting, y is specular weighting,
    // z is transmittance, w is volumetric weighting
    float4 bxdfWeights;

    // Material refractiveness (ignored for now)
    float refNdx;

    // Baseline material color
    float3 rgb;

    // Material roughness (used for microfacet diffuse/specular
    // reflection)
    // Described in terms of standard deviation (=> variance) in
    // the average reflective angle from the surface's normal
    // vector (measured in radians)
    float vari;
};

FigMat FigMaterial(float3 coord,
                   uint figID)
{
    FigMat mat;
    mat.bxdfWeights = float4(1.0f, 0.0f.xxx);
    mat.refNdx = 0.0f;
    mat.rgb = float3(1.0f, 1.0f, 1.0f);
    mat.vari = 0.125f * PI;
    Figure currFig = figuresReadable[figID];
    if (currFig.dfType.x == DF_TYPE_PLANET)
    {
        // Quick/dirty pseudo-procedural material
        // Should replace with a proper physically-driven alternative
        // when possible
        // No point in setting these directly from [rgbaCoeffs] atm;
        // no specialized per-planet shading data to pass in from the
        // CPU anyways
        // Stick to fully-diffuse materials for initial testing
        mat.bxdfWeights = float4(1.0f, 0.0f.xxx);
        //mat.bxdfWeights = float4(currFig.rgbaCoeffs[0].x,
        //                         currFig.rgbaCoeffs[0].y,
        //                         currFig.rgbaCoeffs[0].z,
        //                         currFig.rgbaCoeffs[0].w);

        // Should replace with a physically-driven color function
        // when possible
        mat.rgb = float3(currFig.rgbaCoeffs[1].x,
                         currFig.rgbaCoeffs[1].y,
                         currFig.rgbaCoeffs[1].z);

        // All planets are assumed to have [pi] variance in reflective
        // angle (translates to medium roughness)
        // Should replace this with a physically driven alternative when
        // possible
        mat.vari = PI;
        return mat;
    }
    else if (currFig.dfType.x == DF_TYPE_STAR)
    {
        mat.bxdfWeights = float4(1.0f, 0.0f.xxx); // All stars are diffuse emitters
        mat.rgb = float3(currFig.rgbaCoeffs[0].x,
                         currFig.rgbaCoeffs[0].y,
                         currFig.rgbaCoeffs[0].z); // Stars may have varying colors
        mat.vari = PI; // Stars are assumed perfe
        return mat;
    }
    else
    {
        return mat;
    }
}

// Probability density function (PDF) for directions sampled over diffuse surfaces
float DiffusePDF(float thetaO)
{
    return cos(thetaO) / PI;
}

// Importance-sampled ray generator for diffuse surfaces
// Implemented from the cosine-weighted hemisphere sampling
// strategy shown in
// Physically Based Rendering: From Theory to Implementation
// (Pharr, Jakob, Humphreys)
// Not completely sure how the disc-mapping algorithm in here
// works, should ask CGSE about that...
float3 DiffuseDir(inout uint randVal)
{
    // Generate random sample values
    float2 uv = float2(iToFloat(xorshiftPermu1D(randVal)),
                       iToFloat(xorshiftPermu1D(randVal)));

    // Malley's method requires us to send points onto
    // a disc and then offset them along the up-vector,
    // so place [uv] on the unit disc before we do
    // anything else

    // Map generated random values onto a unit square
    // centered at the origin (occupying the domain
    // [-1.0f...1.0f]^2)
    uv = (uv * 2.0f) - 1.0f.xx;

    // This is a mapping between incompatible spaces
    // (specifically, a square fold over a disc), so
    // applying the mapping to the centre of the
    // projection will send input points to infinity;
    // avoid that by branching here (assumes the
    // centre of the projection is at [0.0f.xx])
    if (!all(uv == 0.0f.xx))
    {
        // We know the mapping won't send [uv] to infinity, so
        // we can safely map points from [-1.0f...1.0f]^2 onto
        // the disc without generating any artifacts

        // Infer radial distance from the larger value between
        // [uv.x] and [uv.y]
        float r = max(uv.x, uv.y);

        // Construct [theta]; generalization follows the notes below
        bool thetaShifting = (r == uv.y);
        float thetaShift = (PI / -2.0f) * thetaShifting; // Conditional rotation; pairs with [invTheta] and [uvRatio]
                                                         // (see below) to recreate the function
                                                         // [(pi / 2) - (pi / 4) * (uv.x / uv.y)] when [r] represents
                                                         // polar radius
        float uvRatio = uv[uv.x > uv.y] / uv[uv.y > uv.x]; // Take varying UV ratios depending on the value of [r]
        float invTheta = ((-1.0f * thetaShifting) + !thetaShifting); // Invert [theta] if [uv.y] represents the radius
        float theta = (((PI / 4.0f) * uvRatio) + thetaShift) * invTheta; // Modified/generalized theta-function

        // Construct a scaled polar coordinate carrying the
        // generated [u, v] sampling values, then return it
        float2 outVec;
        sincos(theta, outVec.y, outVec.x);
        uv = r * outVec;
    }

    // We've placed [uv] on the unit disc (+ used a square mapping
    // technique to preserve uniformity between samples), so now
    // we can complete Malley's method by setting our output [y]
    // direction to the vertical distance from each initial disc
    // sample to the surface of the unit hemisphere
    float2 uvSqr = uv * uv;
    return normalize(float3(uv.x,
                            sqrt(1.0f - (uvSqr.x + uvSqr.y)),
                            uv.y));
}

// RGB diffuse reflectance away from any given surface; uses the Oren-Nayar BRDF
float3 DiffuseBRDF(FigMat mat,
                   float4 thetaPhiIO)
{
    // Generate the basis Lambert BRDF
    float3 lambert = mat.rgb / PI;
    return lambert;

    // Calculate the squared variance (will be needed later)
    float variSqr = mat.vari * mat.vari;

    // Generate Oren-Nayar parameter values
    float a = 1 - (variSqr / (2.0f * (variSqr + 0.33f)));
    float b = 0.45 * (variSqr / (variSqr + 0.09f));
    float alpha = max(thetaPhiIO.x, thetaPhiIO.z);
    float beta = min(thetaPhiIO.x, thetaPhiIO.z);
    float cosPhiSection = cos(thetaPhiIO.y - thetaPhiIO.w);

    // Evaluate the Oren-Nayar microfacet contribution
    float orenNayar = a + (b * max(0, cosPhiSection)) * sin(alpha) * tan(beta);

    // Generate the BRDF by applying the microfacet contribution to the basis Lambert reflectance
    // function, then return the result
    return lambert * orenNayar;
}

// PDF finder for arbitrary reflectance/scattering/transmittance functions
// [bxdfID] gives the reflectance/scattering/transmittance function selected
// by the given material for the current ray
// Still very unsure about material definitions...
float MatPDF(FigMat mat,
             float4 thetaPhiIO,
             uint bxdfID)
{
    // No PDF functions for non-diffuse surfaces, so just return diffuse
    // for now...
    switch (bxdfID)
    {
        case BXDF_ID_DIFFUSE:
            return DiffusePDF(thetaPhiIO.z);
        case BXDF_ID_SPECULAR:
            return 0.0f;
        case BXDF_ID_TRANSMITTANT:
            return 0.0f;
        case BXDF_ID_VOLU:
            return 0.0f;
        default:
            return DiffusePDF(thetaPhiIO.z); // Assume diffuse surfaces for undefined [BXDFs]
    }
}

float3 MatDir(inout uint randVal,
              uint bxdfID)
{
    // No sampling strategy for non-diffuse surfaces, so just return diffuse
    // for now...
    switch (bxdfID)
    {
        case BXDF_ID_DIFFUSE:
            return DiffuseDir(randVal);
        case BXDF_ID_SPECULAR:
            return 0.0f.xxx;
        case BXDF_ID_TRANSMITTANT:
            return 0.0f.xxx;
        case BXDF_ID_VOLU:
            return 0.0f.xxx;
        default:
            return DiffuseDir(randVal); // Assume diffuse surfaces for undefined [BXDFs]
    }
}

// BXDF finder for arbitrary materials
// Still very unsure about material definitions...
float3 MatBXDF(FigMat mat,
              float4 thetaPhiIO,
              uint bxdfID)
{
    // No BXDFs for non-diffuse surfaces, so just return diffuse
    // for now...
    switch (bxdfID)
    {
        case BXDF_ID_DIFFUSE:
            return DiffuseBRDF(mat,
                               thetaPhiIO);
        case BXDF_ID_SPECULAR:
            return 0.0f.xxx;
        case BXDF_ID_TRANSMITTANT:
            return 0.0f.xxx;
        case BXDF_ID_VOLU:
            return 0.0f.xxx;
        default:
            return DiffuseBRDF(mat,
                               thetaPhiIO); // Assume diffuse surfaces for undefined [BXDFs]
    }
}

// Small function to generate BXDF ID's for a figure given a figure ID,
// a figure-space position, and a vector of BXDF weights
// Selection function from the accepted answer at:
// https://stackoverflow.com/questions/1761626/weighted-random-numbers
uint MatBXDFID(uint figID,
               float3 localPos,
               float4 bxdfWeights,
               inout uint randVal)
{
    // Generate selection value
    float bxdfSel = iToFloat(xorshiftPermu1D(randVal));

    // Scan through available material weights and perform a weighted
    // random selection on each pass
    // Sadly no way (...that I know of...) to cleanly spread arbitrary
    // probabilities through a set without using a loop :(
    uint selID = 0;
    for (int i = 0; i < 4; i += 1)
    {
        // Test the selection value against the current material weight;
        // if the selection passes, set [selID] to the loop's counter
        // value (known to be equal to the weight index since we're
        // testing each weight exactly once) and break out of the
        // selection process
        if (bxdfSel < bxdfWeights[i])
        {
            selID = i;
            break;
        }

        // If we think of each weight as a discrete probability "bin", this
        // step exists to move the selection value between bins whenever
        // it falls outside the probability range given by one of the weights
        // in the set
        bxdfSel -= bxdfWeights[i];
    }

    // Return the selected BXDF ID value
    return selID;
}