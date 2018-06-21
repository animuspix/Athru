
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif
#ifndef GEOMETRIC_UTILITIES_LINKED
    #include "GeometricUtility.hlsli"
#endif
#ifndef MATERIAL_UTILITIES_LINKED
    #include "MaterialUtility.hlsli"
#endif

// Small flag to mark whether [this] has already been
// included somewhere in the build process
#define MATERIALS_LINKED

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
// directions importance-sample the GGX microfacet distribution
float SpecularPDF(float4 surfGeo, // Angular microfacet normal in [xyz], surface roughness
                                  // in [w]
                  float4 iDirInfo) // The local incoming light direction (xyz) +
                                   // whether or not it was importance sampled (w)
{
    // Perfectly smooth surfaces are only sampleable with directions on equal
    // sides of the surface normal; we're assuming that those directions will
    // only ever appear from importance sampling, so immediately return zero
    // here for perfectly smooth bi-directional connections (+ return [1.0f]
    // for perfectly-smooth importance-sampled surfaces; if there's only one
    // valid outgoing direction for every incoming direction, then that
    // direction must have 100% probability at each interaction)
    if (!iDirInfo.z && (surfGeo.z == 0.0f)) { return 0.0f; }
    else if (iDirInfo.z) { return 1.0f; }

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

// Importance-sampled ray generator for Torrance-Sparrow specular surfaces
float3 SpecularDir(float3 oDir,
                   float3 microNorml,
                   inout uint randVal)
{
    // Will update for refraction sooooooon
    // Spectral reflection is extraordinarily expensive (at least 3x more rays/scene), so
    // just take the average real indices of refraction here instead of tracing separate
    // rays per-wavelength

    // Reflect the outgoing direction about the given microfacet normal, then
    // return
    // Was *sure* I'd need to do all sorts of fancy processing here, but after re-reading
    // it looks like I was wrong about that ^_^
    return reflect(oDir, microNorml);
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
    float d = GGX(h,
                  variSqr);

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

// Volumetric PDF/Dir/BRDF here (stochastic raymaching (i.e. random walks) + phase functions)
// Atmospheric effects (dust, Rayleigh scattering, Mie scattering) are handled discretely inside
// the main lighting functions (i.e. in [Lighting.hlsli])

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
             uint4 surfInfo)
{
    if (coord.w)
    {
        // Swap in/out directions on the light path
        float3 iDir = ioSurfDirs[0];
        ioSurfDirs[0] = ioSurfDirs[1];
        ioSurfDirs[1] = iDir;
    }

    // Evaluate probabilities for the given material + input/ouput
    // directions
    float pdf = 0.0f;
    switch (surfInfo.x)
    {
        case BXDF_ID_DIFFU:
            pdf = DiffusePDF(VecToAngles(ioSurfDirs[1]).x);
            break;
        case BXDF_ID_MIRRO:
            pdf = SpecularPDF(float4(normalize(ioSurfDirs[0] + ioSurfDirs[1]), // Half-angle vector (i.e. microfacet normal)
                                                                               // reconstructed from the given input/output
                                                                               // directions
                                     MatInfo(float2x3(MAT_PROP_VARI,
                                                      surfInfo.yz,
                                                      coord.xyz)).x),
                              float4(ioSurfDirs[0],
                                     surfInfo.w));
            break;
        case BXDF_ID_REFRA:
            // Remap refractions to perfect specular interactions for now...
            pdf = SpecularPDF(float4(normalize(ioSurfDirs[0] + ioSurfDirs[1]), // Half-angle vector (i.e. microfacet normal)
                                                                               // reconstructed from the given input/output
                                                                               // directions
                                     MatInfo(float2x3(MAT_PROP_VARI,
                                                      surfInfo.yz,
                                                      coord.xyz)).x),
                              float4(ioSurfDirs[0],
                                     surfInfo.w));;
            break;
        case BXDF_ID_VOLUM:
            pdf = 0.0f;
            break;
        default:
            pdf = DiffusePDF(ioSurfDirs[1].x); // Assume diffuse surfaces for undefined [BXDFs]
            break;
    }
    return pdf * MatInfo(float2x3(MAT_PROP_BXDF_FREQS,
                                  surfInfo.yz,
                                  coord.xyz))[surfInfo.x];
}

// [surfInfo] gives the reflectance/scattering/transmittance function selected
// by the given material for the current ray in [x] and the distance-field type +
// figure-ID at [coord] in [yz]
float3 MatDir(float3 oDir,
              float3 coord,
              inout uint randVal,
              uint3 surfInfo)
{
    switch (surfInfo.x)
    {
        case BXDF_ID_DIFFU:
            return DiffuseDir(randVal);
        case BXDF_ID_MIRRO:
            // Generate a microfacet normal matching the GGX normal-distribution-function
            return SpecularDir(oDir,
                               GGXMicroNorml(oDir,
                                          sqrt(MatInfo(float2x3(MAT_PROP_VARI,
                                                                surfInfo.yz,
                                                                coord)).x * 2.0f), // Convert to GGX variance here
                                          randVal),
                               randVal);
        case BXDF_ID_REFRA:
            // Remap refractions to perfect specular interactions for now...
            return SpecularDir(oDir,
                               GGXMicroNorml(oDir,
                                          sqrt(MatInfo(float2x3(MAT_PROP_VARI,
                                                                surfInfo.yz,
                                                                coord)).x * 2.0f), // Convert to GGX variance here
                                          randVal),
                               randVal);
        case BXDF_ID_VOLUM:
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
        case BXDF_ID_MIRRO:
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
        case BXDF_ID_REFRA:
            // Remap refractions to perfect specular interactions for now...
            float2 sinCosThetaIRefr;
            sincos(thetaPhiIO.x, sinCosThetaIRefr.x, sinCosThetaIRefr.y);
            return SpecularBRDF(float4(SurfFres(float4x3(prevFres,
														 rgbToFres(MatInfo(float2x3(MAT_PROP_RGB,
																					surfInfo.yz,
																					coord)).rgb)),
												sinCosThetaIRefr),
                                       MatInfo(float2x3(MAT_PROP_VARI,
                                                        surfInfo.yz,
                                                        coord)).x),
                                thetaPhiIO);
        case BXDF_ID_VOLUM:
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