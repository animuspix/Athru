
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
#define MAX_BOUNCES_PER_SUBPATH 7

// A generic background color; used for contrast against dark surfaces in
// space environments with no ambient environmental light sources besides
// the local star
// Development only, release builds should use a constant [0.0f.xxx] background color
// (no human-visible light in space beyond emittance from the local star)
#define AMB_RGB 0.0f.xxx

// Small object to represent path state during/after ray integration
struct Path
{
    float distToPrev; // Distance between the current path-tracing vertex and the most recent
                      // vertex in the scene
    float2x3 attens; // Attenuation at the foremost vertex + the previous vertex
    float4x4 mat; // Local material properties
    float4 rd; // Outgoing ray direction (xyz) and figure-ID (w)
    float3 ro; // Ray origin
    float3 norml; // Local surface normal
    float3 p; // Position for the latest vertex in the path
    float3 srfP; // Origin for the surface incident with the ray at [p]
    float2 ambFrs; // Ambient Fresnel value during path-tracing
};

// PDF associated with [StellarSurfPos(...), represents the probability of
// selecting any position within the (hemispheric) distribution described
// by [StellarSurfPos(...)]
float StellarPosPDF()
{
    return 1.0f / TWO_PI;
}

// A function to generate unpredictable emission/sample positions on
// the local star
float3 StellarSurfPos(float4 starInfo, // Position in [xyz], size in [w]
                      float2 uv01,
                      float3 targFigPos)
{
    // Generate initial direction (this chooses our sampling hemisphere)
    float3 targVec = (starInfo.xyz - targFigPos);

    // Generate random planar samples in the range [-starInfo.w...starInfo.w]
    float2 randUV = ((uv01 * 2.0f.xx) - 1.0f.xx) * starInfo.w;

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

// Probability of targeting any point on a unit sphere;
// matched to the scene-selecting emission function
// [StellarLiDir] (see below)
float StellarLiPDF()
{
    return 1.0f / FOUR_PI;
}

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

// Trace a ray from a given position on a stellar light
// source towards a random area on the scene; much
// greater convergence rate than [DiffuseLiDir], but
// less physically accurate (light can be emitted
// directly back into the source)
//float3 StellarLiDir(float3 uvw01,
//                    float3 srcPos)
//{
//    float4 figLinTransf = figures[1].linTransf;
//    float3 randSphPt = ((uvw01 - 0.5f.xxx) * 2.0f.xxx) * figLinTransf.w;
//    return normalize(srcPos - (figLinTransf.xyz + randSphPt));
//}

// Diffuse emission profile; uses a full hemispheric distribution
// instead of the cosine-weighted distribution chosen for sampling
// diffuse reflection
// Algorithm taken from page 775 of Physically Based Rendering: From
// Theory to Implementation, Third Edition (Pharr, Jakob, Humphreys)
float3 DiffuseLiDir(float2 uv01)
{
    // Generate a scaling factor to keep [x,z] parameters at the hemisphere
    // surface regardless of the value generated for [uv.x] (taken as the
    // y-position/sample altitude later on)
    float2 uv = (uv01 * 2.0f.xx) + 1.0f;
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
            return StellarLiPDF();
        default:
            return DiffuseLiPDF();
    }
}

// Direction generator for light emissions; diffuse directions are
// assumed for all lights atm (might consider more illumination
// profiles later on...)
//float3 LightEmitDir(uint lightID,
//                    float3 liSurfPos,
//                    uint randStrm)
//{
//    switch (lightID)
//    {
//        case LIGHT_ID_STELLAR:
//            return StellarLiDir(randStrm,
//                                liSurfPos); // Stellar light sources are pseudo-hemispheric diffuse emitters
//        default:
//            return DiffuseLiDir(randStrm); // Assume hemispheric diffuse emission by default
//    }
//}

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
                    uint randStrm)
{
    switch (lightID)
    {
        case LIGHT_ID_STELLAR:
            return StellarSurfPos(lightInfo,
                                  randStrm,
                                  targFigPos);
        default:
            return StellarSurfPos(lightInfo,
                                  randStrm,
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
        float dist = PlanetDF(rayVec,
                              figures[(int)rayOri.w],
                              rayDir.w)[0].x;
        inSurf = dist < rayDir.w;
        if (!inSurf) { break; }
        offset += rayDir.w * 250.0f;
    }
    return offset;
}

// Simple occlusion function, traces a shadow ray between two points to test
// for visiblity (needed to validate generic sub-path connections in BDPT)
// [0].xyz contains the tracing direction, [0].w contains the distance
// travelled by the shadow ray, [1].xyz the final position at the head of
// the shadow ray, [1].w contains the shadow ray's occlusion status
// (false => reached [rayDest] without intersecting any earlier geometry;
// true => intersected geometry between [rayOri] and [rayDest]), and
// [2].xyzw contains the figure ID associated with the ray's intersection
// point (if any)
// [rayDir] carries a ray direction in [xyz] and the target figure-ID in
// [w]
// [rayOri] carries the ray origin in [xyz] and the SDF epsilon value for
// the current frame in [w]
// Planetary surface variance is much too high for ordinary ray offsets to work, so
// occlusion rays are traced backwards from the ray destination instead
#define MAX_OCC_MARCHER_STEPS 256
float3x4 OccTest(float4 rayOri,
                 float4 rayDir,
                 float eps)
{
    float currRayDist = 0.0f;
    bool occ = true;
    float3 endPos = rayOri.xyz;
    uint nearID = 0x11;
    // March occlusion rays
    for (int i = 0; i < MAX_OCC_MARCHER_STEPS; i += 1)
    {
        float3 rayVec = rayOri.xyz + (rayDir.xyz * currRayDist);
        float2x3 sceneField = SceneField(rayVec,
                                         rayOri.xyz,
                                         false,
                                         FILLER_SCREEN_ID,
                                         rayOri.w);
        bool hitDest = (sceneField[0].z == rayDir.w); // Check if [rayVec] has intersected the target figure
        if (sceneField[0].x < eps ||
            (hitDest && sceneField[0].x < eps))
        {
            if (hitDest)
            {
                // Assume rays that reach the target figure-ID are un-occluded and return
                // [false]
                occ = false;
                // Record the intersection point
                endPos = rayVec;
            }
            else
            {
                // The shadow ray was occluded by [sceneField[0].z], so return [true] and
                // cache the world-space position of the intersection point
                occ = true;
                endPos = rayVec;
            }
            // Cache the figure-ID associated with intersection point
            nearID = (uint)sceneField[0].z;
            break;
        }

        // Continue marching through the scene
        currRayDist += sceneField[0].x;
    }
    // Return occlusion information for the given context
    return float3x4(rayDir.xyz,
                    currRayDist,
                    endPos,
                    occ,
                    nearID.xxxx);
}
