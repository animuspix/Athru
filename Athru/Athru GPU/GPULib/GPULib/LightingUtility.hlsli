
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif
#ifndef MATERIALS_LINKED
    #include "Materials.hlsli"
#endif
#ifndef GEOMETRIC_UTILITIES_LINKED
    #include "GeometricUtility.hlsli"
#endif

// Small flag to mark whether [this] has already been
// included somewhere in the build process
#define LIGHTING_UTILITIES_LINKED

// Maximum allowed number of bounces/path
// (required for BDPT vertex caching so that
// connection strategies can be performed between
// every scatter site in the camera sub-path +
// every scatter site in the light sub-path)
#define MAX_BOUNCES_PER_SUBPATH 5

// Figure-ID associated with the local star in each system
// Also where the local star is stored in the per-frame
// scene-array
#define STELLAR_FIG_ID 0

// Figure-ID associated with the lens/pinhole during BDPT, unrelated
// to any defined elements in the actual scene
// Chosen because even super-complex ecologies shouldn't generate [(2^32)-1] different figures
// in each system
#define LENS_FIG_ID 0xFFFFFFFF

// Structure carrying data cached by BDPT during
// subpath construction; needed for accurate
// light transport during path integration
// (easiest to perform that separately to
// core ray propagation/bouncing)
// [figID] and [bxdfID] are ignored for
// vertices on the camera lens/pinhole
struct PathVt
{
    float4 pos; // Position of [this] in eye-space ([xyz]); contains the local figure-ID in [w]
    float4 dfOri; // Position of the figure associated with [this]; [w] carries local surface variance
                  // (not implemented just yet)
    float4 atten; // Light throughput/attenuation at [pos] ([xyz]); contains the local BXDF-ID in [w]
    float4 norml; // Surface normal at [pos]; contains the local distance-function type/ID in [w]
    float4 iDir; // Incoming ray direction in [xyz], surface redness in [w] (not implemented just yet)
    float4 oDir; // Outgoing ray direction in [xyz], surface greenness in [w] (not implemented just yet)
    float4 pdfIO; // Forward/reverse probability densities; ([z] contains local planetary distance
                  // (used for atmospheric refraction), [w] carries surface blueness (not implemented just yet))
};

PathVt BuildBidirVt(float4 posFigID,
                       float3 figPos,
                       float4 attenBXDFID,
                       float4 normlDFType,
                       float2x3 ioDirs,
                       float3 pdfsPlanetDist)
{
    PathVt bdVt;
    bdVt.pos = posFigID;
    bdVt.dfOri = float4(figPos, 0.0f);
    bdVt.atten = attenBXDFID;
    bdVt.norml = normlDFType;
    bdVt.iDir = float4(ioDirs[0], 0.0f);
    bdVt.oDir = float4(ioDirs[1], 0.0f);
    bdVt.pdfIO = float4(pdfsPlanetDist, 0.0f);
    return bdVt;
}

// Bundle representing a pair of bi-directional vertices; helps to simplify
// path-space representation during scene discovery by allowing [SceneVis.hlsl]
// to read/write from/to the same array of vertex pairs regardless of whether
// it's tracing the light or the camera sub-path
// Also makes debugging easier; only need to think about one set of vertices
// describing a symmetric path rather two halves of the path in two separate
// arrays
struct BidirVtPair
{
    PathVt lightVt;
    PathVt camVt;
};

// A generic background color; used for contrast against dark surfaces in
// space environments with no ambient environmental light sources besides
// the local star
// Development only, release builds should use a constant [0.0f.xxx] background color
// (no human-visible light in space beyond emittance from the local star)
#define AMB_RGB 0.0f.xxx

// PDF associated with [StellarSurfPos(...), represents the probability of
// selecting any position within the (hemispheric) distribution described
// by [StellarSurfPos(...)]
float StellarPosPDF()
{
    return 1.0f / TWO_PI;
}

// A function to generate unpredictable emission/sample positions on
// the local star; positions are importance-sampled by limiting sampling
// to positions in the same hemisphere as the target figure
float3 StellarSurfPos(float4 starInfo, // Position in [xyz], size in [w]
                      uint randVal,
                      float3 targFigPos)
{
    // Generate initial direction (this chooses our sampling hemisphere)
    float3 targVec = (starInfo.xyz - targFigPos);

    // Generate random planar samples in the range [-starInfo.w...starInfo.w]
    float2 randUV = ((float2(iToFloat(xorshiftPermu1D(randVal)),
                             iToFloat(xorshiftPermu1D(randVal))) * 2.0f.xx) - 1.0f.xx) * starInfo.w;

    // System figures will always be coplanar with [XZ], so the sampling hemisphere
    // must be coplanar with [y]; cache appropriate jitter here
    float3 yJitter = float3(0.0f, randUV.y, 0.0f); // [randUV] is assumed signed (range [-1.0...1.0])

    // The local-x direction within the sampling plane (i.e. the direction coplanar with the
    // sampling hemisphere and orthogonal to [y]) must be orthogonal to [targVec] AND [y], so we
    // can construct it with the cross-product; generate appropriate jitter and cache that
    // direction here
    float3 localXJitter = cross(normalize(targVec), float3(0.0f, 1.0f, 0.0f)) * randUV.x; // [randUV] is assumed signed (range [-1.0...1.0])

    // Apply the jittered directions to the original target vector, then normalize, scale back to
    // the star's radius, and offset to the star's position
    return (normalize(targVec + yJitter + localXJitter) * starInfo.w) + starInfo.xyz;
}

// Radiance at the given point due to direct illumination from some other
// point in the scene; used for evaluating contributions from light
// gather rays
float3 DirIllumRadiance(float3 traceDir,
                        float3 localNorm,
                        float3 lightRGB,
                        float distToLight,
                        float lightPow)
{
    // Calculate lighting coefficients
    float nDotL = dot(localNorm, traceDir);
    float distToLightSqr = (distToLight * distToLight);
    float3 invSquare = (lightPow / (FOUR_PI * distToLightSqr));

    // Calculate + store directly-illuminated color
    // Will (eventually) remove saturation here and use aperture settings
    // to control exposure levels instead
    return lightRGB // RGB light spectra/color
           * invSquare // Distance attenuation
           * nDotL; // Attenuation for facing ratio
}

// Return emitted spectra in RGB format for the given light source; can
// be viewed as the special case of [DirIllumRadiance(...)] when the
// irradiated point is perfectly transmittant + sitting on the surface
// of the given light source
float3 Emission(float3 srcRGB,
                float3 srcBrightness,
                float distance)
{
    return (srcRGB * srcBrightness) /
           (FOUR_PI * max(distance * distance, 1.0f));
}

// ID's of different light types in Athru
// Only stellar light sources defined for now...
#define LIGHT_ID_STELLAR 0

// Probability of selecting any solid angle within the
// unit hemisphere; equivalent to the constant probability
// density for rays sampled uniformly over hemispheric
// distributions
// Used because light emission directions can't logically
// be thoroughly occluded, so working with a
// cosine-weighted hemisphere doesn't offer the same
// sampling optimization as it does for diffuse surface
// reflection
float DiffuseLiPDF()
{
    return 1.0f / TWO_PI;
}

// Diffuse emission profile; uses a full hemispheric distribution
// instead of the cosine-weighted distribution chosen for sampling
// diffuse reflection
// Algorithm taken from page 775 of Physically Based Rendering: From
// Theory to Implementation, Third Edition (Pharr, Jakob, Humphreys)
float3 DiffuseLiDir(inout uint randVal)
{
    // Generate random sample values
    float2 uv = float2(iToFloat(xorshiftPermu1D(randVal)),
                       iToFloat(xorshiftPermu1D(randVal)));

    // Generate a scaling factor to keep [x,z] parameters at the hemisphere
    // surface regardless of the value generated for [uv.x] (taken as the
    // y-position/sample altitude later on)
    float r = sqrt(1.0f - (uv.x * uv.x));

    // Generate [x,z] directions from the azimuthal angle ([phi]) + the
    // sample value at [uv.y]
    float2 xZ = (2.0f * PI * uv.y).xx;
    sincos(xZ.x, xZ.x, xZ.y);

    // Scale the generated directions out to [r]
    xZ *= r.xx;

    // Generate final direction vector (probably don't need to normalize, but
    // aren't micro-optimizing atm and don't want to think about scaling bugs
    // rn anyway :P)
    return normalize(float3(xZ.y,
                            uv.x,
                            xZ.x));
}

// Probabilities for each illumination profile accessed by [LightEmitDir]; only defined for
// stellar light sources atm
float LightEmitPDF(uint lightID)
{
    switch (lightID)
    {
        case LIGHT_ID_STELLAR:
            return DiffuseLiPDF();
        default:
            return DiffuseLiPDF();
    }
}

// Direction generator for light emissions; diffuse directions are
// assumed for all lights atm (might consider more illumination
// profiles later on...)
float3 LightEmitDir(uint lightID,
                    uint randVal)
{
    switch (lightID)
    {
        case LIGHT_ID_STELLAR:
            return DiffuseLiDir(randVal); // Stellar light sources are hemispheric diffuse emitters
        default:
            return DiffuseLiDir(randVal); // Assume hemispheric diffuse emission by default
    }
}

// Probabilities for each light sampler accessed by [LightSurfPos]; only defined for
// stellar light sources atm
float LightPosPDF(uint lightID)
{
    switch (lightID)
    {
        case LIGHT_ID_STELLAR:
            return StellarPosPDF();
        default:
            return StellarPosPDF();
    }
}

// Standardized light sampler; allows a single interface for arbitrary
// illuminant types (stars, phosphorescent minerals, glowy plants/animals/fungi, etc.)
float3 LightSurfPos(uint lightID,
                    float4 lightInfo, // Light position (in [xyz]) and size (in [w])
                    float3 targFigPos,
                    uint randVal)
{
    switch (lightID)
    {
        case LIGHT_ID_STELLAR:
            return StellarSurfPos(lightInfo,
                                  randVal,
                                  targFigPos);
        default:
            return StellarSurfPos(lightInfo,
                                  randVal,
                                  targFigPos); // Assume stellar light sources by default
    }
}

// Small function to generate surface-agnostic offsets for occlusion tests +
// BXDF ray bounces
float RayOffset(float4 rayOri, // Ray origin in [xyz], figure-ID in [w]
                float4 rayDir) // Ray direction in [xyz], adaptive epsilon in [w]
{
    float offset = rayDir.w;
    bool inSurf = true;
    while (inSurf)
    {
        float3 rayVec = (offset * rayDir.xyz) + rayOri.xyz;
        float dist = FigDF(rayVec,
                           rayOri.xyz,
                           false,
                           figuresReadable[(int)rayOri.w])[0].x;
        inSurf = dist < rayDir.w;
        if (!inSurf) { break; }
        offset += rayDir.w / 8.0f;
    }
    return offset;
}

// Simple occlusion function, traces a shadow ray between two points to test
// for visiblity (needed to validate generic sub-path connections in BDPT)
// [0].xyz contains the tracing direction, [0].w clontains the distance
// travelled by the shadow ray, [1].xyz the final position at the head of
// the shadow ray, [1].w contains the shadow ray's occlusion status
// (false => reached [rayDest] without intersecting any earlier geometry;
// true => intersected geometry between [rayOri] and [rayDest]), and
// [2].xyzw contains the figure ID associated with the ray's intersection
// point (if any)
// [rayDir] carries a ray direction in [xyz] and the target figure-ID in
// [w]
// [rayOri] carries whether or not to occlude on refraction in [w] and the ray
// origin in [xyz]
// [rayDest] carries a specific ray destination in [xyz] (required for bi-directional
// connection) and whether to validate against ray destinations instead of figure-IDs
// in [w]
#define MAX_OCC_MARCHER_STEPS 256
float3x4 OccTest(float4 rayOri,
                 float4 rayDir,
                 float4 rayDest,
                 float adaptEps,
                 inout uint randVal)
{
    float currRayDist = RayOffset(float4(rayOri.xyz, rayDir.w),
                                  float4(rayDir.xyz, adaptEps));
    bool occ = true;
    float3 endPos = rayOri.xyz;
    uint nearID = 0x11;
    float refrLoss = 0.0f; // Total energy lost from refraction between [rayOri]
                           // and the target figure
    for (int i = 0; i < MAX_OCC_MARCHER_STEPS; i += 1)
    {
        float3 rayVec = float3((rayDir.xyz * currRayDist) + rayOri.xyz);
        float2x3 sceneField = SceneField(rayVec,
                                         rayOri.xyz,
                                         true,
                                         FILLER_SCREEN_ID); // Filler filter ID
        float destPtDist = length(rayDest.xyz - rayVec);
        bool hitDest = (sceneField[0].y == rayDir.w); // Check if incident figures/points match the destination figure/point
        if (rayDest.w) { hitDest = (destPtDist < adaptEps); } // Uncomfortable with this layout, considering revising
        if (sceneField[0].x < adaptEps ||
            destPtDist < adaptEps)
        {
            if (hitDest)
            {
                // Assume rays that reach the target figure-ID are un-occluded and return
                // [false]
                occ = false;
                // Clip the intersection point into [rayDest] if validating against ray destinations; otherwise
                // set it to [rayVec]
                if (rayDest.w) { endPos = rayDest.xyz; }
                else { endPos = rayVec; }
            }
            else
            {
                // Find the BXDF-ID at the intersection point
                uint bxdfID = MatBXDFID(MatInfo(float2x3(MAT_PROP_BXDF_FREQS,
                                                         sceneField[0].zy,
                                                         rayVec)),
                                        randVal);

                // Pass through refractive surfaces
                // Invoke a transmittance check here (not all rays will have enough energy
                // left to pass through the surface)
                if ((bxdfID == BXDF_ID_REFRA ||
                     bxdfID == BXDF_ID_VOLUM) &&
                    !rayOri.w)
                {
                    // Update refracted ray direction
                    rayDir.xyz = MatDir(rayDir.xyz,
                                        rayVec,
                                        randVal,
                                        uint3(bxdfID,
                                              sceneField[0].zy));

                    // Update refracted ray position
                    rayVec = RefractPos(rayVec,
                                        rayDir.xyz,
                                        float3(bxdfID,
                                               sceneField[0].zy),
                                        adaptEps,
                                        randVal);
                }
                else
                {
                    // The shadow ray was occluded by [sceneField[0].z], so return [true] and
                    // cache the world-space position of the intersection point
                    occ = true;
                    endPos = rayVec;
                }
            }

            // Cache the figure-ID associated with intersection point
            nearID = (uint)sceneField[0].y;
            break;
        }

        // No intersections yet + [rayVec] hasn't passed within [adaptEps] units
        // of [rayDest]; continue marching through the scene
        if (!rayDest.w) { currRayDist += sceneField[0].x; }
        else { currRayDist += min(sceneField[0].x, destPtDist); }
    }

    // Return occlusion information for the given context
    return float3x4(rayDir.xyz,
                    currRayDist,
                    endPos,
                    occ,
                    nearID.xxxx);
}
