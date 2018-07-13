
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif

// Small flag to mark whether [this] has already been
// included somewhere in the build process
#define MATERIAL_UTILITIES_LINKED

// Enum flags for different types of
// reflectance/scattering/transmittance function; one of
// these is semi-randomly selected for each ray based
// on weights given by [FigMat]
#define BXDF_ID_DIFFU 0
#define BXDF_ID_MIRRO 1
#define BXDF_ID_REFRA 2
#define BXDF_ID_VOLUM 3
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
#define STELLAR_BRIGHTNESS 80000000.0f

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
    switch (query[0].y)
    {
        case DF_TYPE_PLANET:
            switch (query[0].x)
            {
                // No ID-dependant properties just yet, maybe later...
                case MAT_PROP_BXDF_FREQS:
                    return float4(1.0f, 0.0f.xxx); // Just pure diffuse for now
                case MAT_PROP_RGB:
                    return 1.0f.xxxx; // Ordinary all-white colour
                case MAT_PROP_VARI:
                    return PI.xxxx; // Middling surface roughness for planets
                default:
                    return 0.0f.xxxx; // Unclear query, return a null vector
                                      // to the callsite
            }
        case DF_TYPE_STAR:
            switch (query[0].x)
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

// Return whether or not the given BXDF-ID describes a delta distribution
// Only valid for reflective/refractive specular materials atm; I'll update for
// volumetric materials once I've implemented them + decided how to preserve
// diffuse/specular particle choice within [bxdfID]
bool isDeltaBXDF(uint bxdfID)
{
    return bxdfID == BXDF_ID_MIRRO || bxdfID == BXDF_ID_REFRA;
}

// Compute visible microfacet area for GGX interactions
// with the given half-angles
float GGX(float2 thetaPhiH,
          float variSqr)
{
    // Vector carrying trigonometric values needed by GGX (tangent/cosine of [theta])
    // Would love to optimize [cos(h.x)] into a dot-product, but can't easily do that
    // in spherical coordinates (+ I feel passing normalized directions as well as solid angles would
    // add too much complexity to material definitions atm)
    float cosTheta = cos(thetaPhiH.x);
    float2 distroTrig = float2(sqrt(1.0f - cosTheta * cosTheta) / cosTheta,
                               cosTheta);
    if (isinf(distroTrig.x))
    {
        return 0.0f;
    } // Tangent values can easily generate singularities; escape here before those lead to NaNs (!!)
    distroTrig *= distroTrig; // Every trig value for GGX is (at least) squared, so handle that here

    // GGX is essentially normalized Beckmann-Spizzichino and re-uses the Beckmann-Spizzichino exponent
    // as a factor inside the denominator; cache the GGX version here for convenience before we
    // evaluate the complete distribution function
    float ggxBeckmannExp = 1.0f + distroTrig.x / (variSqr + EPSILON_MIN);
    ggxBeckmannExp *= ggxBeckmannExp;

    // Evaluate + return microfacet distribution with GGX
    return 1.0f / ((PI * variSqr * (distroTrig.y * distroTrig.y) * ggxBeckmannExp) + EPSILON_MIN);
}

// PSR mollification function
// PSR implementation is after Kaplanyan and Dachsbacher, see:
// http://cg.ivd.kit.edu/english/PSR.php
// Small changes needed here to support MLT + refraction; see
// the original paper + supplement for details
#define basePSRRadius PLANETARY_RING_RADIUS * 0.01
#define psrLam -0.16666666667f // Approximately ((1.0f / 6.0f) * -1.0f)
float PSRMollify(float radius,
                 float2x3 ioDirs,
                 float dist)
{
    radius = radius / dist; // Scale mollification radius to match the distance between the
                            // outgoing/incoming vertices
    float mollifAngle = atan(radius); // Evaluate the mollfication angle for the given radius
    float cosIOAngle = dot(ioDirs[0], ioDirs[1]); // Evaluate cosine of the angle between the incoming/outgoing directions for
                                                  // the mollified vertex
    if (acos(cosIOAngle) < mollifAngle)
    {
        return (1.0f / (TWO_PI * (1.0f - cos(mollifAngle)))) / cosIOAngle; // Return the mollified PDF :D
    }
    else
    {
        return 0.0f; // Zero probabilities for bounces passing outside the mollification cone
    }
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
                                     oDir.z * ggxVari) + EPSILON_MIN.xxx);

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
    if (tangPlanePt)
    {
        ggxUV.y = (ggxUV.y / pTangPlane) * PI;
    }
    else
    {
        ggxUV.y = (((ggxUV.y - pTangPlane) / (1.0f - pTangPlane)) * PI) + PI;
    }

    // Convert samples to 2D Cartesian coordinates
    float2 sinCosPhi;
    sincos(ggxUV.y, sinCosPhi.x, sinCosPhi.y);
    ggxUV *= sinCosPhi.yx;
    if (tangPlanePt)
    {
        ggxUV.y *= ggxDir.z;
    }

    // Construct basis vectors for the space about [oDir]
    float3 oSpaceX = normalize(cross(oDir, float3(0.0f, 1.0f, 0.0f)) + EPSILON_MIN.xxx);
    float3 oSpaceZ = cross(oSpaceX, oDir);

    // Combine the previous values to project [ggxUV] onto the view-orthogonal
    // plane (=> the region with the most visible area relative to [oDir])
    float3 inPt = (ggxUV.x * oSpaceX) +
                   ((ggxUV.y * oSpaceZ) * ((oDir.y * tangPlanePt) + !tangPlanePt));

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
                            microNorml.z * ggxVari));
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
    float3 twoACosTheta = 2.0f * sqrt(aSqrBSqr[0]) * sinCosThetaI.y;
    float2x3 reflOrthoReal = float2x3(twoACosTheta + sinCosThetaISqr.y, 0.0f.xxx);
    float2x3 reflOrtho = cplxDiv(aSqrBSqr - reflOrthoReal,
                                 aSqrBSqr + reflOrthoReal);

    // Evaluate parallel reflectance
    float2x3 reflParallReal = float2x3(twoACosTheta * sinCosThetaISqr.x + (sinCosThetaISqr.x * sinCosThetaISqr.x), 0.0f.xxx);
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

// Find the exit position for refracted rays
// [startInfo] has an initial position in [xyz]
// and a figure-ID in [w]
// [surfInfo] has a BXDF-ID in [x], a figure-ID
// in [y], and a distance-function type in [z]
float3 RefractPos(float3 startPos,
                  float3 refrDir,
                  float3 surfInfo,
                  float adaptEps,
                  inout uint randVal)
{
    float currRayDist = adaptEps * 2.0f;
    float3 rayVec = startPos;
    if (surfInfo.x == BXDF_ID_REFRA)
    {
        while (currRayDist > adaptEps)
        {
            rayVec = startPos + (refrDir * currRayDist);
            currRayDist += FigDF(rayVec,
                                 startPos,
                                 false,
                                 figuresReadable[surfInfo.y])[0].x * -1.0f; // We're moving *inside* scene figures, so make sure to
                                                                       // trace inverse PDFs here
        }
        return rayVec + refrDir * (adaptEps * 2.0f); // Translate the generated position just outside the SDF of the
                                                     // refracted figure (to prevent immediate occlusion during ray
                                                     // propagation through the scene)
    }
    else // Volumetric scattering implied here, refraction through non-transmissive/volumetric materials is invalid
    {
        // Return the input position for now, will implement for multiple
        // scattering events later on...
        return startPos;
    }
}
