
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif

// Small flag to mark whether [this] has already been
// included somewhere in the build process
#define MATERIALS_LINKED

// Enum flags for different types of
// reflectance/scattering/transmittance function; one of
// these is semi-randomly selected for each ray based
// on weights given by [FigMat]
#define BXDF_ID_DIFFU 0
#define BXDF_ID_SPECU 1
#define BXDF_ID_SSURF 2
#define BXDF_ID_MEDIA 3

// A small object used to store procedural material
// definitions during path-tracing
struct FigMat
{
    // Generic and diffuse-specific properties

    // x is diffuse weighting, y is specular weighting,
    // z is transmittance, w is volumetric weighting
    float4 bxdfFreqs;

    // Baseline material color
    float3 rgb;

    // Material roughness (used for microfacet diffuse/specular
    // reflection)
    // Described in terms of standard deviation (=> variance) in
    // the average reflective angle from the surface's normal
    // vector (measured in radians)
    float vari;

    // Fresnel/specular properties

    // Refractive index ([0]) and extinction coefficient ([1])
    // (refractive index >> how much light distorts during interaction with [this],
    //  extinction coefficient >> how efficiently [this] absorbs transmitted light)
    float2x3 fres;
};

// Convenience function to convert RGB color to fresnel properties
float2x3 rgbToFres(float3 rgb)
{
    // Spectral refraction/extinction for titanium; should generate these 
    // procedurally when possible
    // Procedural generation will need to generate uniform white reflectors
    // with variance limited to wetness/lustre/shininess; that would let
    // us apply material colors with simple tints instead of directly
    // generating fresnel values to match each components in [rgb]
    // Refraction/extinction values from
    // https://refractiveindex.info/?shelf=3d&book=metals&page=titanium
    // after following the Fresnel tutorial in
    // https://shanesimmsart.wordpress.com/2018/03/29/fresnel-reflection/
    float2x3 tiFres = float2x3(2.7407, 2.5418, 2.2370,
                               3.8143, 3.4345, 3.0235);
    return tiFres * rgb; // Final Fresnel values are naively tinted from the
                         // white-ish baseline fresnel defined above
}

FigMat FigMaterial(float3 coord,
                   uint figID)
{
    FigMat mat;
    mat.bxdfFreqs = float4(1.0f, 0.0f.xxx);
    mat.refNdx = 0.0f;
    mat.rgb = float3(1.0f, 1.0f, 1.0f);
    mat.vari = 0.125f * PI;
    Figure currFig = figuresReadable[figID];
    if (currFig.dfType.x == DF_TYPE_PLANET)
    {
        // No point in setting these directly from [rgbaCoeffs] atm;
        // no specialized per-planet shading data to pass in from the
        // CPU anyways
        // Stick to fully-diffuse materials for initial testing
        mat.bxdfFreqs = float4(1.0f, 0.0f.xxx);

        // Should replace with a procedural color function
        // when possible
        mat.rgb = float3(currFig.rgbaCoeffs[1].x,
                         currFig.rgbaCoeffs[1].y,
                         currFig.rgbaCoeffs[1].z);

        // Generate spectral indices of refraction + extinction coefficients
        // for the color associated with [this]
        mat.fres = rgbToFres(mat.rgb);

        // All planets are assumed to have [pi] variance in reflective
        // angle (translates to medium roughness)
        // Should replace this with a procedural alternative when
        // possible
        mat.vari = PI;
        return mat;
    }
    else if (currFig.dfType.x == DF_TYPE_STAR)
    {
        mat.bxdfFreqs = float4(1.0f, 0.0f.xxx); // All stars are diffuse emitters
        mat.rgb = float3(currFig.rgbaCoeffs[0].x,
                         currFig.rgbaCoeffs[0].y,
                         currFig.rgbaCoeffs[0].z); // Stars may have varying colors
        mat.vari = 0; // Stars are assumed smooth surfaces
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

        // Some changes from the PBR algorithm, mostly to
        // minimize branching...

        // Infer radial distance from the larger absolute value between
        // [uv.x] and [uv.y], then re-apply the sign of the original
        // axis
        float2 absUV = abs(uv);
        float r = max(absUV.x, absUV.y) *
                  sign(uv[absUV.x < absUV.y]);

        // Construct [theta]
        float uvRatio = uv[absUV.x > absUV.y] / uv[absUV.y > absUV.x]; // Take varying UV ratios depending on the value of [r]
        float3 coeffs = float3(-1.0f, 1.0f, (r == uv.y)); // Generic coefficients, needed to support a [theta] definition without [if]s
        float theta = (((PI / 2.0f) * coeffs.z) - ((PI / 4.0f) * uvRatio)) * coeffs[coeffs.z]; // Generalized [theta] definition

        // Construct a scaled polar coordinate carrying the
        // generated [u, v] sampling values, then return it
        float2 outVec;
        sincos(theta, outVec.y, outVec.x);
        uv = r.xx * outVec;
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
// Surface carries color in [rgb], RMS microfacet roughness/variance in [a]
float3 DiffuseBRDF(float4 surf,
                   float4 thetaPhiIO)
{
    // Generate the basis Lambert BRDF
    float3 lambert = surf.rgb / PI;

    // Calculate the squared variance (will be needed later)
    float variSqr = surf.a * surf.a;

    // Generate Oren-Nayar parameter values
    float a = 1.0f - (variSqr / (2.0f * (variSqr + 0.33f)));
    float b = (0.45 * variSqr) / (variSqr + 0.09f);
    float alpha = max(thetaPhiIO.x, thetaPhiIO.z);
    float beta = min(thetaPhiIO.x, thetaPhiIO.z);
    float cosPhiSection = cos(thetaPhiIO.y - thetaPhiIO.w);

    // Evaluate the Oren-Nayar microfacet contribution
    float orenNayar = a + (b * max(0, cosPhiSection)) * sin(alpha) * tan(beta);

    // Generate the BRDF by applying the microfacet contribution to the basis Lambert reflectance
    // function, then return the result
    return lambert * orenNayar;
}

// Probability density function (PDF) for directions sampled over specular surfaces; assumes
// directions importance-sample the GGX/Smith BRDF
float SpecularPDF()
{
    return 0.0f;
}

// Importance-sampled ray generator for GGX specular surfaces
// Implemented from the unbiased hemisphere sampling strategy described
// by Heitz in 
// Eric Heitz. 
// A Simpler and Exact Sampling Routine for the GGX Distribution of Visible Normals. 
// [Research Report] Unity Technologies. 2017
// Research found on the HAL INRIA open-archives repository at:
// https://hal.archives-ouvertes.fr/hal-01509746
float3 SpecularDir()
{
    return 0.0f.xxx;
}

// Evaluate the Fresnel coefficient for the given spectral refraction + 
// extinction coefficients
float3 SurfFres(float3x3 fresInfo,
                float4 thetaPhiIO)
{
    return 1.0f.xxx;
}

// RGB specular reflectance away from any given surface; uses the Torrance-Sparrow BRDF
// described in Physically Based Rendering: From Theory to Implementation
// (Pharr, Jakob, Humphreys)
// [surf] carries a spectral Fresnel value in [rgb] and RMS microfacet roughness/variance in [a]
// Torrance-Sparrow uses the D/F/G definition for physically-based surfaces, where
// D -> Differential area of microfacets with a given facet-normal (evaluated with GGX here)
// F -> Fresnel attenuation, describes ratio of reflected/absorbed (or transmitted) light
//      for different surfaces 
// G -> Geometric attenuation, describes occlusion from nearby microfacets at each
//      sampling location; I'm evaluating this with Smith's masking-shadowing function
//      (see below)
float3 SpecularBRDF(float4 surf,
                    float4 thetaPhiIO)
{
    // Evaluate the GGX microfacet distribution function for the given input/output
    // angles (the "D" term in the PBR D/F/G definition)
    float variSqr = surf.a * 2.0f; // Double variance here so we can re-use Oren-Nayar roughness :)
    float2 facetAngles = VecToAngles(...); // Example microfacet angles (decomposed from a given microfacet normal [wh])
    float2 distroTrig; // Vector carrying trigonometric values needed by GGX (tangent/cosine of [theta] in [xy],
                       // sine/cosine of [phi] in [zw])
    distroTrig.xy = float2(tan(facetAngles.x), cos(facetAngles.x)); // Evaluate cosine/tangent values for [theta]
    if (isinf(distroTrig.x)) { return 0.0f.xxx; } // Tangent values can easily generate singularities; escape here before those lead to NaNs (!!)
    distroTrig *= distroTrig; // Every trig value for GGX is (at least) squared, so handle that here
    
    // GGX is essentially normalized Beckmann-Spizzichino and re-uses the Beckmann-Spizzichino exponent 
    // as a factor inside the denominator; cache the GGX version here for convenience before we 
    // evaluate the complete distribution function
    float ggxBeckmannExp = 1 + distroTrig.x / variSqr; 
    ggxBeckmannExp *= ggxBeckmannExp;

    // Compute microfacet distribution with GGX
    float d = 1.0f / (PI * variSqr * (distroTrig.y * distroTrig.y) * ggxBeckmannExp); 

    // Evaluate Smith shadowing/masking (the "G" term in D/F/G)
    float g;

    // Use the generated d/f/g values to compute + return the Torrance/Sparrow BRDF
    return d * surf.rgb * g / (4.0f * cos(thetaPhiIO.z) * cos(thetaPhiIO.x));
}

// Sub-surface-scattering PDF/Dir/BRDF here (using BSSRDFs)
// Media PDF/Dir/BRDF here (...stochastic raymaching (i.e. random walks)... + phase functions)

// PDF finder for arbitrary reflectance/scattering/transmittance functions
// [bxdfInfo] gives the reflectance/scattering/transmittance function selected
// by the given material for the current ray in [x] and the probability of selecting
// that material in [y]
float MatPDF(float4 thetaPhiIO,
             float2 bxdfInfo)
{
    float pdf = 0.0f;
    switch (bxdfID)
    {
        case BXDF_ID_DIFFU:
            pdf = DiffusePDF(thetaPhiIO.z);
        case BXDF_ID_SPECU:
            pdf = SpecularPDF();
        case BXDF_ID_SSURF:
            pdf = 0.0f;
        case BXDF_ID_MEDIA:
            pdf = 0.0f;
        default:
            pdf = DiffusePDF(thetaPhiIO.z); // Assume diffuse surfaces for undefined [BXDFs]
    }
    return pdf * bxdfInfo.y;
}

float3 MatDir(inout uint randVal,
              uint bxdfID)
{
    switch (bxdfID)
    {
        case BXDF_ID_DIFFU:
            return DiffuseDir(randVal);
        case BXDF_ID_SPECU:
            return SpecularDir(randVal);
        case BXDF_ID_SSURF:
            return 0.0f.xxx;
        case BXDF_ID_MEDIA:
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
    switch (bxdfID)
    {
        case BXDF_ID_DIFFU:
            return DiffuseBRDF(float4(mat.rgb, mat.vari),
                               thetaPhiIO);
        case BXDF_ID_SPECU:
            return SpecularBRDF(float4(SurfFres(mat.fres,
                                                thetaPhiIO), 
                                       mat.vari),
                                thetaPhiIO);
        case BXDF_ID_SSURF:
            return 0.0f.xxx;
        case BXDF_ID_MEDIA:
            return 0.0f.xxx;
        default:
            return DiffuseBRDF(float4(mat.rgb, mat.vari),
                               thetaPhiIO); // Assume diffuse surfaces for undefined [BXDFs]
    }
}

// Small function to generate BXDF ID's for a figure given a figure ID,
// a figure-space position, and a vector of BXDF weights
// Selection function from the accepted answer at:
// https://stackoverflow.com/questions/1761626/weighted-random-numbers
uint MatBXDFID(uint figID,
               float3 localPos,
               float4 bxdfFreqs,
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
        if (bxdfSel < bxdfFreqs[i])
        {
            selID = i;
            break;
        }

        // If we think of each weight as a discrete probability "bin", this
        // step exists to move the selection value between bins whenever
        // it falls outside the probability range given by one of the weights
        // in the set
        bxdfSel -= bxdfFreqs[i];
    }

    // Return the selected BXDF ID value
    return selID;
}