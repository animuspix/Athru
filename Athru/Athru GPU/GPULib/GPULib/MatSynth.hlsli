
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif

#ifndef UTILITIES_LINKED
    #include "GenericUtility.hlsli"
#endif

// Small #define to avert multiple inclusions
#define MAT_SYNTH_LINKED

// Enum flags for different types of
// reflectance/scattering/transmittance function; one of
// these is semi-randomly selected for each ray based
// on weights in surface materials
// Atmospheric scattering is computed in-line with ordinary
// ray-marching; all other scattering models are computed
// in specialty sampling functions at intersection
#define BXDF_ID_DIFFU 0
#define BXDF_ID_MIRRO 1
#define BXDF_ID_REFRA 2
#define BXDF_ID_CLOUD 3
#define BXDF_ID_ATMOS 4
#define BXDF_ID_SNOWW 5
#define BXDF_ID_SSURF 6
#define BXDF_ID_FURRY 7
#define BXDF_ID_NOMAT 8

// Evaluate the Fresnel coefficient for the given incoming/outgoing spectral refraction/extinction
// indices + outgoing angle-of-incidence
// Would like to use the direct maths implementation from PBR, but it's very expensive and
// easy to fluff up; this approximation was found in
// http://www.codinglabs.net/article_physically_based_rendering_cook_torrance.aspx
float FresMirr(float2 etaKap, // Surface indices of refraction (x) and extinction (y)
               float2x3 dirs) // Output direction in [0], local normal in [1]
{
    float cTheta = dot(dirs[1], dirs[0]);
    float etaMin1 = etaKap.x - 1.0f;
    etaMin1 *= etaMin1;
    float etaPlu1 = etaKap.x + 1.0f;
    etaPlu1 *= etaPlu1;
    etaKap *= float2(1.0f, etaKap.y);
    return (etaMin1 + ((4.0f * etaKap.x) * pow(1.0f - cTheta, 5.0f)) + etaKap.y) / (etaPlu1 + etaKap.y);
}

// Evaluate the Fresnel coefficient for the given incoming/outgoing spectral IORs
// + incoming angle-of-incidence
// Uses the dielectric Fresnel function from
// Physically Based Rendering (Pharr, Jakob, Humphreys), page 518
// Spectral Fresnel coefficient returned in [rgb], incoming/outgoing [eta] ratio
// returned in [a]
float2 FresDiel(float2 etaIO, // Incoming/outgoing indices of refraction
               			      // Vectorized indices are possible, but cause significant
               			      // dispersion that I'd rather avoid here
                float2x3 dirs) // Output direction in [0], local normal in [1]
{
    // Evaluate cosines of the incoming/outgoing ray directions
    // Incoming cosine is derivable from Snell's law,
    // https://en.wikipedia.org/wiki/Snell's_law
    float cosI = abs(dot(dirs[0], dirs[1])); // Outgoing cosine is a trivial dot-product
    float sinI = sqrt(max(1.0f - (cosI * cosI), 0.0f));
    float etaRatio = etaIO.x / etaIO.y; // Ratio of sines is equivalent to ratios of [eta] (Snell's Law)
    float sinT = etaRatio * sinI;
    float cosT = sqrt(max(1.0f - (sinT * sinT), 0.0f));

    // Compute reflectance for parallel polarized light
    float rhoPar = ((etaIO.y * cosI) - (etaIO.x * cosT)) /
        		   ((etaIO.y * cosI) + (etaIO.x * cosT));

    // Compute reflectance for orthogonal polarized light
    float rhoOrt = ((etaIO.x * cosI) - (etaIO.y * cosT)) /
        		   ((etaIO.x * cosI) + (etaIO.y * cosT));
    return float2(((rhoPar * rhoPar) +
            	  (rhoOrt * rhoOrt)) * 0.5f,
                  etaRatio);
}

// GGX sampling function from
// https://agraphicsguy.wordpress.com/2015/11/01/sampling-microfacet-brdf/
// + minor optimizations
float3 GGXMicroNorml(float vari,
                     float2 uv01)
{
    float2 uv = uv01 * TWO_PI; // [phi] can be sampled uniformly
    uv.x = sqrt((1.0f - uv.x) /
                (uv.x * (vari - 1.0f) + 1.0f)); // Avoid [acos] here since we'll need the cosine when we change coordinates
    // Change to Cartesian coordinates, then return
    float sTheta = sqrt(1.0f - (uv.x * uv.x));
    float cPhi = cos(uv.y);
    float sPhi = sin(uv.y);
    return normalize(float3(sTheta * cPhi,
                		    uv.x,
                		    sTheta * sPhi));
}

// Small function to generate BXDF ID's for a figure given a figure ID,
// a figure-space position, and a vector of BXDF weights
// Selection function from the accepted answer at:
// https://stackoverflow.com/questions/1761626/weighted-random-numbers
uint MatBXDFID(float4 bxdfFreqs,
               float u01)
{
    // Scan through available material weights and perform a weighted
    // random selection on each pass
    // Sadly no way (...that I know of...) to cleanly spread arbitrary
    // probabilities through a set without using a loop :(
    for (uint i = 0; i < 4; i += 1)
    {
        // Test the selection value against the current material weight;
        // return the current index to the callsite if the selection
        // passes
        if (u01 < bxdfFreqs[i])
        {
            return i;
        }

        // If we think of each weight as a discrete probability "bin", this
        // step exists to move the selection value between bins whenever
        // it falls outside the probability range given by one of the weights
        // in the set
        u01 -= bxdfFreqs[i];
    }
    return 0u; // Default to diffuse surfaces
}

// Per-vertex material synthesizer
// Light materials are implict in [DirIllumRadiance]/[Emission]
// so there's no reason to generate them here
float4x4 MatGen(float3 coord,
                float3 wo,
                float3 norml,
                float2 ambFres,
                uint dfType,
                inout PhiloStrm randStrm)
{
    // Just a basic generalized planetary/stellar function now, will create
    // plant/critter variants when neccessary
    switch (dfType)
    {
        // Cases for plants & animals will go here
        default:
            // Cache material probabilities
            // Will eventually update [matStats] for volumetric materials (fur, subsurface-scattering volumes, cloudy media)
            float4 matStats = float4(0.98f, 0.02f, 0.0f.xx);

            // Cache the current surface/volume type
            float4 rand = philoxPermu(randStrm);
            uint bxdfID = MatBXDFID(matStats,
                                    rand.x);

            // Cache surface roughness
            // Prefer rough values for diffuse surfaces and smooth values for specular
            float alpha = (bxdfID == BXDF_ID_DIFFU) ? 1.0f : 0.4f;

            // Generate a GGX microsurface normal for specular reflection/refraction
            float3 muNorml = GGXMicroNorml(alpha,
                                           rand.yz);

            // Generate conductor/dielectric probabilities
            float fresP1 = abs(sin(coord.y));
            float fresP2 = (1.0f - fresP1);

            // Cache (fixed atm) indices of refraction/absorption
            float2 etaKap = float2(1.33f, 1.1f);

            // Evaluate + cache Fresnel values appropriately
            bool metal = true; //rand.w > fresP1;
            float2 fres = 1.0f.xx;
            if (metal)
            {
                fres.x = FresMirr(etaKap,
                                  float2x3(wo,
                                           muNorml));
                // Metals won't refract light at the macroscale, so merge refractive probability
                // into the chance for mirror reflection (matStats.y) before zeroing the original
                // value (matStats.z)
                matStats.y += matStats.z;
                matStats.z = 0.0f;
            }
            else
            {
                fres = FresDiel(float2(etaKap.x, ambFres.x),
                                float2x3(wo,
                                         muNorml));
                matStats.y *= fres.x; // Scale mirror probability for dielectrics
                matStats.z *= (1.0f - fres.x); // Scale refractive probability for dielectrics
            }
            float3 rgb = abs(sin(norml));
            return float4x4(matStats,
                            rgb, alpha, // Procedural color, fixed surface roughness
                            fres, bxdfID, 0.0f, // Fresnel values in [xy], surface/volume type in [z], [w] is unused
                            muNorml, 0.0f.x); // Microsurface normal in [xyz], [w] is unused
    }
}
