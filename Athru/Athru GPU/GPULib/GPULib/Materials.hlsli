
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

// Short macro to swap zero-valued PDFs with very large numbers
// (effectively minimizing very unlikely contributions)
#define ZERO_PDF_REMAP(x) ((x == 0.0f) ? (1.0f / EPSILON_MIN) : x)

// Importance-sampled ray generator for diffuse surfaces
// Implemented from the cosine-weighted hemisphere sampling
// strategy shown in
// Physically Based Rendering: From Theory to Implementation
// (Pharr, Jakob, Humphreys)
// Diffuse PDF returned in [w] (assumes Oren-Nayar PBR shading)
float DiffusePDF(float3 wi, float3 norml) { return dot(norml, wi) / PI; }
float4 DiffuseDir(float3 wo,
                  float3 norml,
                  float2 uv01)
{
    // Malley's method requires us to send points onto
    // a disc and then offset them along the up-vector,
    // so place [uv] on the unit disc before we do
    // anything else

    // Map generated random values onto a unit square
    // centered at the origin (occupying the domain
    // [-1.0f...1.0f]^2)
    float2 uv = (uv01 * 2.0f) - 1.0f.xx;

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
    float3 wi = normalize(float3(uv.x,
                                 sqrt(1.0f - (uvSqr.x + uvSqr.y)),
                                 uv.y));
    return float4(wi,
                  DiffusePDF(wi, UNIT_Y)); // [wi] is in sampling space and sees the normal parallel to y-up
}

// Diffuse reflectance away from any given surface; uses the Oren-Nayar BRDF
// Surface carries color in [rgb], RMS microfacet roughness/variance in [a]
float3 DiffuseBRDF(float4 surf,
                   float3x3 surfDirs)
{
    // Generate the basis Lambert BRDF
    float3 lambert = surf.rgb / PI;

    // Scale [0...1] roughness out to the range defined for Oren-Nayar
    surf.a *= HALF_PI;

    // Calculate the squared variance (will be needed later)
    float variSqr = surf.a * surf.a;

    // Generate Oren-Nayar parameter values
    float a = 1.0f - (variSqr / (2.0f * (variSqr + 0.33f)));
    float b = (0.45 * variSqr) / (variSqr + 0.09f);
    float2 cThetaIO = float2(dot(surfDirs[1], surfDirs[0]), // Cosines are equal to [n . [l | v]]
                             dot(surfDirs[1], surfDirs[2]));
    float2 sThetaIO = sqrt(max(1.0f.xx - (cThetaIO * cThetaIO), 0.0f.xx)); // Sines are derivable from cosines

    // Sneaky implementation of [cosPhiSection] borrowed from iq:
    // https://www.shadertoy.com/view/ldBGz3
    float cosPhiSection = dot(surfDirs[2] - surfDirs[1] * cThetaIO.y,
                              surfDirs[1] - surfDirs[1] * cThetaIO.x);

    // Evaluate microfacet attenuation for the Oren-Nayar BRDF
    float orenNayar = a + (b * max(0, cosPhiSection)) * // Straight copy from the standard Oren-Nayar definition here...
                      (sThetaIO.x * sThetaIO.y) / // Cleaner this way than defining a separate [alpha] value
                      max(cThetaIO.x, cThetaIO.y); // Tangent is defined as sine/cosine, no reason
                                                   // to conditionally access sine values when both are used anyways

    // Generate the BRDF by applying the microfacet contribution to the basis Lambert reflectance
    // function, then return the result
    return lambert * orenNayar;
}

// Accept a GGX microfacet normal + a roughness + an outgoing direction, then synthesize a matching
// reflective direction (xyz) and an appropriate PDF (w)
// Directions have input in [0], half vector in [1], output in [2], and macrosurface normal in [3]
float MirroPDF(float4x3 dirs,
               float vari)
{
    return GGXSmithPDF(dirs,
                       vari) /
           (4.0f * dot(dirs[2], dirs[1]));
}
float4 MirroDir(float3 wo,
                float3 microNorml,
                float3 norml,
                float vari,
                out float3 hV)
{
    // Reflect the outgoing direction about the given microfacet normal
    float3 wi = reflect(wo, microNorml);
    // Compute the half-vector for specular reflection
    float3 h = normalize(wo + wi);
    // Export the generated half-vector
    hV = h;
    // Reflective rays cannot refract through the surface; return zero probability for those here
    if (!SameHemi(float3x3(wi,
                           wo * -1.0f,
                           norml)))
    { return float4(wi, 0.0f); }
    // Input/output rays lie in the same hemisphere, so this is a valid reflection; return GGX/Smith
    // weighted probability here + the generated input direction
    return float4(wi,
                  MirroPDF(float4x3(wi,
                                    h,
                                    wo,
                                    norml),
                           vari));
}

// Evaluate GGX/Smith PBR reflection for the given surface
float3 MirroBRDF(float4x3 dirs, // Input direction in [0], half-vector in [1],
               			        // output direction in [2], macrosurface normal in [3]
                 float4 surf, // Reflective surface color in [rgb], squared roughness in [a]
                 float fres) // Fresnel coefficient for the surface (scalar, spectral Fresnel feels too messy for real-time)
{
    // Evaluate cosine terms
    float cosTerm = dot(dirs[3], dirs[0]);
    cosTerm *= dot(dirs[3], dirs[2]);
    if (cosTerm == 0.0f)
    { return 0.0f.xxx; } // Avert divisions by zero

    // Compute the PBR specular BRDF
    return ((GGX(float2x3(dirs[1],
                          dirs[3]),
                 surf.a) * // Distribution term [D]
             Smith(float3x3(dirs[0],
                            dirs[1],
                            dirs[2]),
                   surf.a) * // Masking/shadowing term [G]
             fres) / // Fresnel term [F]
            abs(4.0f * cosTerm)) * // Reflection attenuation
        	surf.rgb; // Surface tint
}

// Accept a GGX/Smith microfacet normal, then synthesize a matching refractive direction (xyz)
// and an appropriate PDF (w)
// Directions have input in [0], half vector in [1], output in [2], and macrosurface normal in [3]
float RefrPDF(float4x3 dirs,
              float etaRatio,
              float vari)
{
    float wiDH = dot(dirs[0], dirs[1]);
    float pdfDenom = dot(dirs[2], dirs[1]) + (etaRatio * wiDH);
    pdfDenom *= pdfDenom;
    etaRatio *= etaRatio;
    return GGXSmithPDF(dirs,
                       vari) * // Evaluate generalized probability density for GGX/Smith microfacet incidence
           abs(etaRatio * dot(dirs[0], dirs[1]) / pdfDenom); // Scale appropriately for microfacet transmission
}
float4 RefrDir(float3 wo,
               float3 microNorml,
               float3 norml,
               float etaRatio,
               float vari,
               out float3 hV)
{
    float3 wi = refract(wo, microNorml, etaRatio);
    float3 h = normalize(wo + (wi * etaRatio));
    hV = h; // Export the generated half-vector (needed for BXDF evaluation)
    if (SameHemi(float3x3(wi,
                          wo * -1.0f,
                          norml)))
    { return float4(wi, 0.0f); } // Refractive rays cannot reflect from the surface; return zero probability for those here
    float pdfDenom = dot(wo, h) + (etaRatio * dot(wi, h));
    pdfDenom *= pdfDenom;
    etaRatio *= etaRatio;
    return float4(wi, RefrPDF(float4x3(wi,
                                   	   h,
                                   	   wo,
                                   	   norml),
                              etaRatio,
                              vari));
}

// Evaluate GGX/Smith PBR transmission for the given surface
// Follows Walter et al., at:
// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
float3 RefrBXDF(float4x3 dirs, // Input direction in [0], half-vector in [1],
                 		       // output direction in [2], macrosurface normal in [3]
                float4 surf, // Surface color in [rgb], roughness in [a]
                float fres, // Fresnel coefficient for the local surface
                float etaRatio) // Ratio of of incident/transmittant indices-of-refraction
{
    // Walter et al.'s method expects [h] above the surface, conditionally invert it here
    if (dirs[1].y < 0.0f) { dirs[1] *= -1.0f; }

    // Cache cosines relative to [h]
    float2 hCosines = float2(dot(dirs[0],
                                 dirs[1]),
                             dot(dirs[2],
                                 dirs[1]));
    // Avert divisions by zero
    if (hCosines.x == 0.0f || hCosines.y == 0.0f) { return 0.0f.xxx; }

    // Evaluate the denominator for the LHS of the transmission function
    float invEtaRatio = 1.0f / etaRatio;
    float denom = ((invEtaRatio * hCosines.y) + (etaRatio * hCosines.x));
    denom *= denom;

    // Compute the LHS
    float lhs = (GGX(float2x3(dirs[1],
                    	      dirs[3]),
                     surf.a) * // Distribution term [D]
        	     Smith(float3x3(dirs[0],
                                dirs[1],
                                dirs[2]),
                       surf.a) * // Masking/shadowing term [G]
                 (1.0f - fres) * // Fresnel term [F]
        	     (etaRatio * etaRatio)) /
        	     denom;

    // Cache cosines relative to the macrosurface normal
    float2 nCosines = float2(dot(dirs[0],
                                 dirs[3]),
                             dot(dirs[2],
                                 dirs[3]));

    // Avert divisions by zero (for macrosurface normals this time)
    if (nCosines.x == 0.0f || nCosines.y == 0.0f) { return 0.0f.xxx; }

    // Compute the RHS
    hCosines = abs(hCosines);
    nCosines = abs(nCosines);
    float rhs = (hCosines.x * hCosines.y) / (nCosines.x * nCosines.y);

    // Evaluate + return the full BTDF
    return abs(lhs * rhs) * // Microfacet attenuation
           surf.rgb; // Transmissive color (T)
}

// Volumetric Dir/PDF/BRDF here (stochastic raymaching (i.e. random walks) + phase functions)
// Atmospheric effects (dust, Rayleigh scattering, Mie scattering) are handled discretely inside
// the main lighting functions (i.e. in [Lighting.hlsli])

// Spatial sampler for surfaces; accepts an arbitrary input direction and returns appropriate
// surface radiance
float4 MatSpat(float3x3 dirs, // Input direction in [0], macrosurface normal in [1],
                              // output direction in [2]
               float4 matStats,
               float3 rgb,
               float4 surfInfo)
{
    switch ((uint) surfInfo.x)
    {
        case BXDF_ID_DIFFU:
            return float4(DiffuseBRDF(float4(rgb,
                                             surfInfo.a),
                                      dirs),
                          DiffusePDF(dirs[0],
                                     dirs[1]) / ZERO_PDF_REMAP(matStats.x));
        case BXDF_ID_MIRRO:
            float3 hm = normalize(dirs[0] + dirs[2]);
            return float4(MirroBXDF(float4x3(dirs[0],
                                             hm,
                                             dirs[2],
                                             dirs[1]),
                                    float4(rgb, surfInfo.a),
                                    surfInfo.y),
                          MirroPDF(float4x3(dirs[0],
                                            hm,
                                            dirs[2],
                                            dirs[1]),
                                   surfInfo.w) / ZERO_PDF_REMAP(matStats.y));
        case BXDF_ID_REFRA:
            // Evaluate local radiance given [wi], also apply PDF
            float3 hr = normalize(dirs[2] + (dirs[0] * surfInfo.z));
            return float4(RefrBXDF(float4x3(dirs[0],
                                            hr,
                                            dirs[2],
                                            dirs[1]),
                                 float4(rgb, surfInfo.a),
                                 surfInfo.y,
                                 surfInfo.z),
                          RefrPDF(float4x3(dirs[0],
                                   	       hr,
                                   	       dirs[2],
                                   	       dirs[1]),
                                  surfInfo.z,
                                  surfInfo.a) / ZERO_PDF_REMAP(matStats.z));
        // Add other material types here...
        default:
            return 0.0f.xxxx;
    }
}

// Generic surface sampler; branches once/material instead of once/BXDF + once/PDF + once/direction
// [surfInfo] gives the reflectance/scattering/transmittance function selected
// by the given material for the current ray in [x], the surface's Fresnel coefficient + eta-ratio in [yz],
// and the local surface variance in [w]
float2x4 MatSrf(float3 wo,
                float3 norml,
                float3 muNorml,
                float4 matStats, // BXDF probabilities for the current material
                float3 rgb, // Diffuse surface color
                float4 surfInfo,
                float2 uv01)
{
    switch ((uint)surfInfo.x)
    {
        case BXDF_ID_DIFFU:
            // Generate diffuse direction + evaluate weighted local radiance
            float4 wid = DiffuseDir(wo,
                                    norml,
                                    uv01);
            float3 ld = DiffuseBRDF(float4(rgb, surfInfo.a),
                                    float3x3(wid.xyz,
                                             norml,
                                             wo));
            // Return radiance + the generated ray direction + PDF to the callsite
            return float2x4(wid.xyz, 0.0f.x,
                            ld, (wid.a / ZERO_PDF_REMAP(matStats.x)));
        case BXDF_ID_MIRRO:
            // Generate mirror-reflective direction + matching PDF
            float3 hm = 1.0f.xxx;
            float4 wim = MirroDir(wo,
                                  muNorml,
                                  norml,
                                  surfInfo.w,
                                  hm);
            // Evaluate local radiance given [wi], also apply PDF
            float3 lm = MirroBXDF(float4x3(wim.xyz,
                                           hm,
                                           wo,
                                           norml),
                                  float4(rgb, surfInfo.a),
                                  surfInfo.y);
            // Return radiance + ray direction + PDF
            return float2x4(wim.xyz, 0.0f,
                            lm, (wim.a / ZERO_PDF_REMAP(matStats.y)));
        case BXDF_ID_REFRA:
            // Generate refractive direction + matching PDF
            float3 hr = 1.0f.xxx;
            float4 wir = RefrDir(wo,
                                 muNorml,
                                 norml,
                                 surfInfo.z,
                                 surfInfo.a,
                                 hr);
            // Evaluate local radiance given [wi], also apply PDF
            float3 lr = RefrBXDF(float4x3(wir.xyz,
                                          hr,
                                          wo,
                                          norml),
                                 float4(rgb, surfInfo.a),
                                 surfInfo.y,
                                 surfInfo.z);
            // Return radiance + ray direction
            return float2x4(wir.xyz, 0.0f,
                            lr, (wir.a / ZERO_PDF_REMAP(matStats.z)));
        // Add other material types here...
        default:
            return float2x4(0.0f.xxxx,
                            0.0f.xxxx);
    }
}
