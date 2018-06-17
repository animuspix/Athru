
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
#define BXDF_ID_NOMAT 4

// Fixed vector indices-of-refraction + extinction
// coefficients for atmosphere/vacuum
// Will likely make atmospheric refraction planet-dependant
// at some point (>> different gasses with different colors/densities)
// but not until I have a solid baseline renderer
// Current scattering properties for atmosphere are based on earthen
// air at sea-level; I found the refraction coefficient here:
// https://refractiveindex.info/?shelf=other&book=air&page=Ciddor
// and the (zero) absorption coefficient here:
// https://www.cfd-online.com/Forums/main/5757-absorption-scattering-co-efficients-radiation.html
// (first post from user [nuray kayakol])
#define SCATTER_THRU_ATMOS float2x3(float3(1.00027632, 1.00027784, 1.00028091), \
                                    0.0f.xxx) // Air absorbs essentially zero energy across all visible
                                              // wavelengths

// Vacuum has no reflective effect on light at any visible
// wavelength, and absorbs no energy across all visible
// wavelengths
#define SCATTER_THRU_VACUUM float2x3(1.0f.xxx,\
                                     0.0f.xxx)

// Returns ambient fresnel coefficients (refraction/extinction) for the given planetary distance;
// points within [EPSILON_MAX] of a planet will begin experiencing atmospheric refraction
// (but not scattering, need a mie/rayleigh system for that :P)
float2x3 AmbFres(float planetDist)
{
    if (planetDist < EPSILON_MAX)
    {
        return SCATTER_THRU_ATMOS;
    }
    else
    {
        return SCATTER_THRU_VACUUM;
    }
}

// Convenience function to convert RGB color to fresnel properties
float2x3 rgbToFres(float3 rgb)
{
    // Not necessary now, but should vary between dielectric and conducting fresnels
    // depending on the material's BXDF ID (i.e. dielectric fresnels for subsurface-scattering
    // materials and conductors for specular surfaces)

    // Spectral refraction/extinction tinted from titanium; should generate baseline
    // refraction/extinction procedurally when possible
    // Procedural generation will need to generate uniform white reflectors
    // with variance limited to wetness/lustre/shininess; that would let
    // us apply material colors with simple tints instead of directly
    // generating fresnel values to match each component in [rgb]
    // Refraction/extinction values from
    // https://refractiveindex.info/?shelf=3d&book=metals&page=titanium
    // after following the Fresnel tutorial in
    // https://shanesimmsart.wordpress.com/2018/03/29/fresnel-reflection/
    return float2x3(float3(2.7407, 2.5418, 2.2370) * rgb,
                    float3(3.8143, 3.4345, 3.0235) * rgb);
}

// x is diffuse weighting, y is specular weighting
// z is subsurface scattering, w is the frequency
// that a given material behaves like the isovolume
// of a participating medium
#define MAT_PROP_BXDF_FREQS 0

// Baseline material colour
#define MAT_PROP_RGB 1

// Material roughness (used for microfacet diffuse/specular
// reflection)
// Described in terms of standard deviation (=> variance) in
// the average reflective angle from the surface's normal
// vector (measured in radians)
#define MAT_PROP_VARI 2

// Constant stellar brightness; might make this a [cbuffer] value
// so I can vary it between systems
#define STELLAR_BRIGHTNESS 8.0f

// Stellar radiance at unit brightness; mostly just defined for
// readability since any star bright enough to illuminate the
// system will wash out to white for human observers
#define STELLAR_RGB 1.0f.xxx

// Alternative material model, much simpler and data-oriented
// Database-like design, takes a figure type + property request ID and
// generates the requested property on-demand
// Magic nested switch statements to *only* generate the most relevant
// data for each request :)
// No template returns in HLSL, but no ish; all the data we want to
// return this way will fit in float4 anyways :)
// [query] has requested property in [0][x], distance-field type in [0][y]
// figure-ID in [0][z], and position in [1]
float4 MatInfo(float2x3 query)
{
    switch(query[0].x)
    {
        case DF_TYPE_PLANET:
            switch(query[0].x)
            {
                // No ID-dependant properties just yet, maybe later...
                case MAT_PROP_BXDF_FREQS:
                    return float4(1.0f, 0.0f.xxx); // Just pure diffuse for now
                case MAT_PROP_RGB:
                    return 1.0f.xxx; // Ordinary all-white colour
                case MAT_PROP_VARI:
                    return PI.xxxx; // Middling surface roughness for planets
                default:
                    return 0.0f.xxxx; // Unclear query, return a null vector
                                      // to the callsite
            }
        case DF_TYPE_STAR:
            switch(query[0].x)
            {
                // No ID-dependant properties just yet, maybe later...
                case MAT_PROP_BXDF_FREQS:
                    return float4(1.0f, 0.0f.xxx); // Stars are diffuse emitters
                case MAT_PROP_RGB:
                    return STELLAR_BRIGHTNESS.xxxx; // Stars are bright enough to only emit
                                                   // white light from a human perspective
                                                   // (+ no redshifting in Athru)
                case MAT_PROP_VARI:
                    return 0.0f.xxxx; // Stars are assumed to have smooth surfaces
                default:
                    return 0.0f.xxxx; // Unclear query, return a null vector
                                      // to the callsite
            }
        default:
            return 0.0f.xxxx; // No defined materials beyond stars and planets rn
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

// Compute visible microfacet area for GGX interactions
// with the given half-angles
float GGX(float2 thetaPhiH,
          float surfVari)
{
    // Vector carrying trigonometric values needed by GGX (tangent/cosine of [theta])
    // Would love to optimize [cos(h.x)] into a dot-product, but can't easily do that
    // in spherical coordinates (+ I feel passing normalized directions as well as solid angles would
    // add too much complexity to material definitions atm)
    float cosTheta = cos(h.x);
    float2 distroTrig = float2(sqrt(1.0f - cosTheta * cosTheta) / cosTheta,
                               cosTheta);
    if (isinf(distroTrig.x)) { return 0.0f.xxx; } // Tangent values can easily generate singularities; escape here before those lead to NaNs (!!)
    distroTrig *= distroTrig; // Every trig value for GGX is (at least) squared, so handle that here

    // GGX is essentially normalized Beckmann-Spizzichino and re-uses the Beckmann-Spizzichino exponent
    // as a factor inside the denominator; cache the GGX version here for convenience before we
    // evaluate the complete distribution function
    float ggxBeckmannExp = 1.0f + distroTrig.x / variSqr;
    ggxBeckmannExp *= ggxBeckmannExp;

    // Evaluate + return microfacet distribution with GGX
    return 1.0f / (PI * variSqr * (distroTrig.y * distroTrig.y) * ggxBeckmannExp);
}

// Placeholder PSR mollification function
// PSR implementation is after Kaplanyan and Dachsbacher, see:
// http://cg.ivd.kit.edu/english/PSR.php
float3 PSRMollify()
{
    return 0.0f.xxx;
}

// Probability density function (PDF) for directions sampled over specular surfaces; assumes
// directions importance-sample the GGX microfacet distribution
float SpecularPDF(float4 surfGeo, // Angular microfacet normal in [xyz], surface roughness 
                                  // in [w]
                  float4 iDirInfo) // The local incoming light direction (xyz) + 
                                   // whether or not it was importance sampled (w)
{
    // Perfectly smooth surfaces are only sampleable with directions on equal
    // sides of the surface normal; we're assuming that those directions will 
    // only ever appear from importance sampling, so immediately return a 
    // mollified delta-distribution here instead of the GGX probability
    // function
    if (!iDirInfo.z && (surfGeo.z == 0.0f)) { return PSRMollify(); }

    // Return visible microfacet area (mirrorlike reflection will always have
    // the half-vector equal to the local normal, so we can safely propagate
    // the sampled microfacet normal here); our samples come directly from GGX,
    // so visible microfacet area through the given normal will naturally
    // describe the chance of reflecting around that direction (since 
    // "large" regions with lots of visible area would be expected to reflect
    // more light towards the viewing direction than "small" regions with heavy
    // occlusion and much less microsurface exposure) 
    // Also attenuate the generated GGX value with [cos([theta])] since incoming 
    // rays are unlikely to meet microfacets close to parallel with the 
    // macrosurface normal
    // Unsure about this explanation, should maebs check with CGSE
    float2 microNormlSrs = VecToAngles(surfGeo.xyz);
    float ggxPDF = GGX(microNormlSrs,
                       surfGeo.w * 2.0f) * cos(microNormlSrs.x);

    // The GGX distribution is defined for the half-angle vector, but our PDFs are
    // expected to return results defined over incoming/outgoing directions; we can
    // convert betweeen those with the half-angle/incoming-angle sampling ratio
    // derived by PBR, so apply that here before returning to the callsite
    // ("PBR" means 
    // Physically Based Rendering: From
    // Theory to Implementation (Pharr, Jakob, Humphreys),
    // and the derivation is available around page ~812)
    return ggxPDF / (4.0f * dot(iDirInfo.xyz,
                                surfGeo.xyz));
}

// Small function to generate microfacet normals matching the GGX 
// normal-distribution-function
// Implemented from the exact hemisphere sampling strategy described
// by Heitz in
// Eric Heitz.
// A Simpler and Exact Sampling Routine for the GGX Distribution of Visible Normals.
// [Research Report] Unity Technologies. 2017
// Research found on the HAL INRIA open-archives repository at:
// https://hal.archives-ouvertes.fr/hal-01509746
float3 GGXMicroNorml(float3 oDir,
                     float ggxVari,
                     inout uint randVal)
{
    // Heitz' updated method assumes unit roughness (where GGX forms a uniform hemisphere);
	// maintaining microfacet visibility under that assumption means applying the actual
	// surface roughness to displace the outgoing direction within [XZ] (the plane incident
	// with the hemisphere's disc in local space) before renormalizing; this can be seen as
    // rotating the given direction through the GGX distribution until the visible
    // microfacet area is the same in the [alpha = 1] case as when the [alpha = ggxVari]
    // case is seen from the un-rotated output direction
    // Microfacet normals naturally fall back into the [y] axis (equivalent to the local
    // macrosurface normal) for totally smooth surfaces, so we don't need any specific
    // handling for perfect mirrors here ^_^
    float3 ggxDir = normalize(float3(oDir.x * ggxVari, 
                                     oDir.y, 
                                     oDir.z * ggxVari));

    // Generate GGX-specific UV values
    float2 ggxUV = float2(iToFloat(xorshiftPermu1D(randVal)),
						  iToFloat(xorshiftPermu1D(randVal)));

    // Heitz' method splits the GGX sampling space into two half-disc regions projected
    // through the shading hemisphere; one within a plane orthogonal to the outgoing 
    // direction and one incident with the base of the hemisphere itself
    // This is intuitive; visible microfacets at any roughness will mostly lie in
    // the plane normal to the viewing (outgoing) direction
    // Heitz samples from the full disc composed from the projections of either 
    // half-disc, and represents sample positions in polar coordinates [([r], [theta])];
    // the [theta] component is scaled to match the probability of selecting either 
    // half-disc ([pTangent = 1.0f / (1.0f + ggxDir.z)] for the half-disc within the 
    // tangent plane, [1.0f - pTangent] for the half-disc within the base of the shading 
    // hemisphere)
    float pTangPlane = 1.0f / (1.0f + ggxDir.y);
    bool tangPlanePt = !(ggxUV.y < pTangPlane);
    ggxUV.x = sqrt(ggxUV.x); // This might be removable given uniform UV values; maybe test
                             // when I have working specular shading...
    // Could be worth trying to implement these in Cartesian coordinates straight away 
    // instead of starting in polar and converting afterwards...
    if (tangPlanePt) { ggxUV.y = (ggxUV.y / pTangPlane) * PI; } 
    else { ggxUV.y = ((ggxUV.y - pTangPlane) / (1.0f - pTangPlane)) * PI) + PI; }

    // Convert samples to 2D Cartesian coordinates
    float2 sinCosPhi;
    sincos(ggxUV.y, sinCosPhi.x, sinCosPhi.y);
    ggxUV *= sinCosPhi.yx;
    if (tangPlanePt) { ggxUV.y *= ggxDir.z; }
    
    // Construct basis vectors for the space about [oDir]
    float3 oSpaceX = normalize(cross(oDir, float3(0.0f, 1.0f, 0.0f)));
    float3 oSpaceZ = cross(oSpaceX, oDir);

    // Combine the previous values to project [ggxUV] onto the view-orthogonal
    // plane (=> the region with the most visible area relative to [oDir])
    float3 inPt = (ggxUV.x * oSpaceX) + 
                   ((ggxUV.y * oSpaceZ) * ((oDir.y * pTangPlanePt) + !pTangPlanePt));

    // Generate a hemisphere position
    float3 microNorml = inPt + // Use positions on the view-orthogonal disc as a local offset 
                        (sqrt(1.0f - dot(inPt, inPt)) * oDir); // Scale the known disc positions
                                                               // out to the hemisphere; perform
                                                               // scaling along the direction
                                                               // described by [oDir]

    // Transform the generated position to the unit-alpha GGX distribution (this is the same 
    // transformation we applied to the outgoing ray direction before; see above for how/why 
    // we did that) and normalize (giving a roughened GGX-style direction through the 
    // hemisphere), then return
    return normalize(float3(microNorml.x * ggxVari, 
                            microNorml.y, 
                            microNorml * ggxVari));
}

// Importance-sampled ray generator for Torrance-Sparrow specular surfaces
float3 SpecularDir(float3 oDir,
                   float3 microNorml,
                   inout uint randVal)
{
    // Reflect the outgoing direction about the given microfacet normal, then
    // return
    // Was *sure* I'd need to do all sorts of fancy processing here, but after re-reading
    // it looks like I was wrong about that ^_^ 
    return reflect(oDir, microNorml);
}

// Short convenience function for vectorized complex division
// Sourced from:
// http://mathworld.wolfram.com/ComplexDivision.html
float2x3 cplxDiv(float2x3 a,
                 float2x3 b)
{
    float3 denom = b[0] * b[0] + b[1] * b[1];
    return float2x3(a[0] * b[0] + a[0] * b[1] / denom,
                    a[1] * b[0] - a[1] * b[1] / denom);
}

// Short convenience function for vectorized complex multiplication
float2x3 cplxMul(float2x3 a,
                 float2x3 b)
{
    // Given a complex product (a + bi)(c + di),
    // the product's expression should evaluate as
    // ac + adi + bic + bidi
    // e.g. (1 + i)^2 >> (1 + i)(1 + i)
    //      1^2 + i + i + i^2
    //      1 + 2i - 1
    //      0 + 2i
    return float2x3(a[0] * b[0], 0.0f.xxx) +
           float2x3(0.0f.xxx, a[0] * b[1]) +
           float2x3(0.0f.xxx, a[1] * b[0]) -
           float2x3(a[1] * b[1], 0.0f.xxx); // Hardcoded real subtraction since the product of two imaginaries is negative (i^2 >> -1)
}

// Function to evaluate complex square roots for real-valued
// numbers (positive or negative)
float2x3 cplxReRt(float3 a)
{
    float2x3 rt = float2x3(0.0f.xxx, 0.0f.xxx); // Avert singularities at [sqrt(0)] here
    if (a.x > 0.0f) { rt[0].x = sqrt(abs(a.x)); } // Evaluate sqrt(a.x) for the real case
    else if (a.x < 0.0f) { rt[1].x = sqrt(abs(a.x)); } // Evaluate sqrt(a.x) for the imaginary case
    if (a.y > 0.0f) { rt[0].y = sqrt(abs(a.x)); } // Evaluate sqrt(a.y) for the real case
    else if (a.y < 0.0f) { rt[1].y = sqrt(abs(a.x)); } // Evaluate sqrt(a.y) for the imaginary case
    if (a.x > 0.0f) { rt[0].z = sqrt(abs(a.x)); } // Evaluate sqrt(a.z) for the real case
    else if (a.x < 0.0f) { rt[1].z = sqrt(abs(a.x)); } // Evaluate sqrt(a.z) for the imaginary case
    return rt; // Return the generated complex vector for [sqrt(a)]
}

// Evaluate the Fresnel coefficient for the given spectral refraction +
// extinction coefficients + solid-angle ray directions
float3 SurfFres(float4x3 fresInfoIO, // Incoming/outgoing fresnel values (refraction in [0] and [2],
                                     // extinction/absorption in [1] and [3])
                float2 sinCosThetaI)
{
    // Compute the relative index of refraction [[fresInfoO]/[fresInfoI]]
    // Each [fresInfo] element is a complex number pretending to be a real matrix,
    // so we'll need to perform complex division here instead of computing an
    // ordinary real-number ratio
    float2x3 relFres = cplxDiv(float2x3(fresInfoIO[2],
                                        fresInfoIO[3]),
                               float2x3(fresInfoIO[0],
                                        fresInfoIO[1]));

    // Per-component square over [relFres]
    // Different to a scalar complex square like [cplxMul(z,z)]
    float2x3 relFresSqr = float2x3(relFres[0] * relFres[0],
								   relFres[1] * relFres[1]);

    // The generalized Fresnel equation often uses the complex value
    // [a^2 + b^2], so cache that here

    // Generate [(a^2 + b^2)^2] first so we can easily handle imaginary values of
    // [a^2 + b^2] later
    float2 sinCosThetaISqr = sinCosThetaI * sinCosThetaI;
    float3 aSqrBSqrReRt = relFresSqr[0] + relFresSqr[1] - sinCosThetaISqr.x; // Plus because [k] is imaginary, so k^2 is negative real (and
                                                                             // x - -y is equivalent to x + y over the reals)
    float3 aSqrBSqrSqr = float3(aSqrBSqrReRt * aSqrBSqrReRt - // Minus because k^2 (and 4n^2k^2 by extension) will be negative
                                4.0f * relFresSqr[0] * relFresSqr[1]);

    // Ugly forky code to cleanly generate [float2x3]-style complex vectors from unpredictably
    // real/imaginary roots
    float2x3 aSqrBSqr = cplxReRt(aSqrBSqrSqr);

    // Evaluate perpendicular/orthogonal reflectance
    float twoACosTheta = 2.0f * sqrt(aSqrBSqr[0]) * sinCosThetaI.y;
    float2x3 reflOrthoReal = float2x3(twoACosTheta + sinCosThetaISqr.y, 0.0f.xxx);
    float2x3 reflOrtho = cplxDiv(aSqrBSqr - reflOrthoReal,
                                 aSqrBSqr + reflOrthoReal);

    // Evaluate parallel reflectance
    float2x3 reflParallReal = twoACosTheta * sinCosThetaISqr.x + (sinCosThetaISqr.x * sinCosThetaISqr.x);
    float2x3 cosThetaSqrASqrBSqr = float2x3(aSqrBSqr[0] * sinCosThetaISqr.y, // Not sure why scalar/matrix multiplication fails here...
										    aSqrBSqr[1] * sinCosThetaISqr.y);
    float2x3 reflParall = cplxMul(reflOrtho,
                                  cplxDiv((cosThetaSqrASqrBSqr - reflParallReal),
                                          (cosThetaSqrASqrBSqr + reflParallReal)));

    // Combine the generated values into an overall Fresnel reflectance, then return the
    // result
    // 0.5f * (a + bi)^2 + (c + di)^2
    // (4 + 2i)(4 + 2i)
    // 16 + 8i + 8i + 4i^2
    // 16 + 16i - 16
    // 16i
    return 0.5f * cplxMul(reflOrtho, reflOrtho)[1] + cplxMul(reflParall, reflParall)[1];
}

// RGB specular reflectance away from any given surface; uses the Torrance-Sparrow BRDF
// described in Physically Based Rendering: From Theory to Implementation
// (Pharr, Jakob, Humphreys)
// [surf] carries a spectral Fresnel value in [rgb] and GGX microfacet roughness/variance in [a]
// Torrance-Sparrow uses the D/F/G definition for physically-based surfaces, where
// D -> Differential area of microfacets with a given facet-normal (evaluated with GGX here)
// F -> Fresnel attenuation, describes ratio of reflected/absorbed (or transmitted) light
//      for different surfaces
// G -> Geometric attenuation, describes occlusion from nearby microfacets at each
//      sampling location; I'm evaluating this with Smith's masking-shadowing function
//      (see below)
// Unsure about the derivations for Smith and GGX, should read the original papers
// when possible
float3 SpecularBRDF(float4 surf,
                    float4 thetaPhiIO)
{
    // Extract Torrance-Sparrow roughness values from [surf.a] (materials use Oren-Nayar roughness by default)
    float variSqr = surf.a * 2.0f;

    // Zeroed rougness means the surface is a perfect mirror; perfect mirrors are
    // described by specialized BRDFs missed by Torrance-Sparrow, so handle those
    // here and return early instead
    if (variSqr == 0.0f)
    {
        // We're using wavelength-dependant indices of refraction + extinction coefficients
        // already, so no reason to scale our fresnel value against [mat.rgb]
        return surf.rgb / cos(thetaPhiIO.x);
    }

    // Evaluate the GGX microfacet distribution function for the given input/output
    // angles (the "D" term in the PBR D/F/G definition)
    float2 h = (thetaPhiIO.xy + thetaPhiIO.zw) * 0.5f.xx; // We want to simulate highly specular surfaces where
                                                          // ideal reflection lies along the half-angle between [i]
                                                          // and [o], so it makes sense to only evaluate the
                                                          // differential area of microsurfaces that face the same
                                                          // direction (i.e. have normals parallel to [h])
                                                          // Might be using bad values for [h] here, should validate later...
    // Can optimize this by recycling values from the specula BRDF, 
    // but that would be kinda messy...strongly leaning towards a
    // generic material evaluator nows instead
    // (solves + returns the BRDF and PDF for arbitrary materials, chooses
    // between generated and provided bounce directions depending on 
    // context)
    // Would allow for less code, faster exits, etc.
    // Less argument passing, so possibly simpler code as well...
    // Definitely a plausible refactor after implementing PSR
    // (want to finish specular, do that, debug + validate everything,
    // *then* implement transmittance before starting any major
    // refactors/releasing demo videos/looking for work)
    float d = GGX(h);

    // Evaluate Smith masking/shadowing (the "G" term in D/F/G) for the incoming/outgoing directions
    float2 absTanThetas = abs(tan(thetaPhiIO.xz)); // Evaluate the absolute tangent of each direction's angle
                                                   // around [theta]

    // Evaluate ratio of hidden/visible microfacet area (>> "lambda") for GGX along the incoming/outgoing
    // directions
    float2 lam = (sqrt(1.0f + (variSqr.xx * (absTanThetas * absTanThetas))) - 1.0f.xx) * 0.5f.xx;

    // Avert tangent singularities here
    if (isinf(absTanThetas.x)) { lam.x = 0.0f; }
    if (isinf(absTanThetas.y)) { lam.y = 0.0f; }

    // Compute the Smith masking/shadowing function
    float g = 1.0f / (1.0f + lam.x + lam.y);

    // Division-by-zero produces singularities in IEEE 754; escape here before those singularities
    // lead to NaNs
    if (!any(thetaPhiIO.xz == HALF_PI.xx)) { return 0.0f.xxx; }

    // Use the generated D/F/G values to compute + return the Torrance/Sparrow BRDF
    // We're using wavelength-dependant indices of refraction + extinction coefficients
    // already, so no reason to scale our reflectance against [mat.rgb]
    return d * surf.rgb * g / (4.0f * cos(thetaPhiIO.z) * cos(thetaPhiIO.x));
}

// Sub-surface-scattering PDF/Dir/BRDF here (using BSSRDFs)
// Media PDF/Dir/BRDF here (...stochastic raymaching (i.e. random walks)... + phase functions)

// PDF finder for arbitrary reflectance/scattering/transmittance functions
// [surfInfo] gives the reflectance/scattering/transmittance function selected
// by the given material for the current ray in [x], the surface's distance-field 
// type + figure-ID in [yz], and whether or not the incoming light direction was
// importance-sampled for the local BXDF in [w]
// [ioSurfDirs] describes input/output directions in surface coordinates (i.e. with
// y-up); this simplifies physically-based shading (no need to transform microfacet 
// normals) and conceptually neatens the sampling process by allowing everything 
// to occur in the same coordinate space
// [coord] carries global surface position in [xyz] and whether the surface lies on 
// the light subpath in [w]
float MatPDF(float2x3 ioSurfDirs,
             float4 coord,
             float4 surfInfo) 
{
    if (coord.w)
    {
        // Swap in/out directions on the light path
        ioDirs[0] = rayDir; 
        ioDirs[1] = bounceDir;
    }

    // Evaluate probabilities for the given material + input/ouput
    // directions
    float pdf = 0.0f;
    switch (surfInfo.x)
    {
        case BXDF_ID_DIFFU:
            pdf = DiffusePDF(VecToAngles(ioSurfDirs[1]).x);
            break;
        case BXDF_ID_SPECU:
            pdf = SpecularPDF(float4(normalize(ioSurfDirs[0] + ioSurfDirs[1]), // Half-angle vector (i.e. microfacet normal) from
                                                                               // reconstructed from the given input/output 
                                                                               // directions
                                     MatInfo(float2x3(MAT_PROP_VARI,       
                                                      surfInfo.yz,             
                                                      coord))),
                              float4(ioSurfDirs[0], 
                                     surfInfo.w));
            break;
        case BXDF_ID_SSURF:
            pdf = 0.0f;
            break;
        case BXDF_ID_MEDIA:
            pdf = 0.0f;
            break;
        default:
            pdf = DiffusePDF(ioSurfDirs[1].x); // Assume diffuse surfaces for undefined [BXDFs]
            break;
    }
    return pdf * MatInfo(float2x3(MAT_PROPS_BXDF_FREQS,
                                  surfInfo.yz,
                                  coord))[bxdfID];
}

// [surfInfo] gives the reflectance/scattering/transmittance function selected
// by the given material for the current ray in [x] and the distance-field type + 
// figure-ID at [coord] in [yz]
float3 MatDir(float3 oDir,
              float3 coord,
              out float3 microNorml, // Microfacet normal direction for
                                     // GGX (useful for e.g. specular surfaces,
                                     // possibly volumetric materials as well)
              inout uint randVal,
              float3 surfInfo)
{
    switch (surfInfo.x)
    {
        case BXDF_ID_DIFFU:
            return DiffuseDir(randVal);
        case BXDF_ID_SPECU:
            // Generate a microfacet normal matching the GGX normal-distribution-function
            float3 mNorml = GGXMicroNorml(oDir,
                                          sqrt(MatInfo(float2x3(MAT_PROPS_VARI,
                                                                surfInfo.yz,
                                                                coord)).x * 2.0f), // Convert to GGX variance here
                                          randVal);
            microNorml = mNorml;
            return SpecularDir(oDir,
                               mNorml,
                               randVal);
        case BXDF_ID_SSURF:
            return 0.0f.xxx;
        case BXDF_ID_MEDIA:
            return 0.0f.xxx;
        default:
            return DiffuseDir(randVal); // Assume diffuse surfaces for undefined [BXDFs]
    }
}

// BXDF finder for arbitrary materials
float3 MatBXDF(float3 coord,
               float2x3 prevFres, // Refraction/extinction coefficients for the volume
                                  // entered by the previous vertex in the relevant
                                  // subpath
               float4 thetaPhiIO,
               uint3 surfInfo) // BXDF-ID in [x], distance-field type in [y], figure-ID in [z]
{
    switch (surfInfo.x)
    {
        case BXDF_ID_DIFFU:
            return DiffuseBRDF(float4(MatInfo(float2x3(MAT_PROP_RGB,
													   surfInfo.yz,
													   coord)).rgb,
                                      MatInfo(float2x3(MAT_PROP_VARI,
													   surfInfo.yz,
													   coord)).x),
                               thetaPhiIO);
        case BXDF_ID_SPECU:
            float2 sinCosThetaI;
            sincos(thetaPhiIO.x, sinCosThetaI.x, sinCosThetaI.y);
            return SpecularBRDF(float4(SurfFres(float4x3(prevFres,
														 rgbToFres(MatInfo(float2x3(MAT_PROP_RGB,
																					surfInfo.yz,
																					coord)).rgb)),
												sinCosThetaI),
                                       MatInfo(float2x3(MAT_PROP_VARI,
                                                        surfInfo.yz,
                                                        coord)).x),
                                thetaPhiIO);
        case BXDF_ID_SSURF:
            return 0.0f.xxx; // No defined sub-surface scattering BXDF yet
        case BXDF_ID_MEDIA:
            return 0.0f.xxx; // No defined BXDF for participating media yet
        default:
            return 1.0f.xxx / PI; // Assume white Lambert surfaces for undefined [BXDFs]
    }
}

// Small function to generate BXDF ID's for a figure given a figure ID,
// a figure-space position, and a vector of BXDF weights
// Selection function from the accepted answer at:
// https://stackoverflow.com/questions/1761626/weighted-random-numbers
uint MatBXDFID(float4 bxdfFreqs,
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