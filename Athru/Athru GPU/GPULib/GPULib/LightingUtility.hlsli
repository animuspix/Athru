
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
// [figID], and [bxdfID] are ignored for
// vertices on the camera lens/pinhole
struct BidirVert
{
    float4 pos; // Position of [this] in eye-space ([xyz]); contains the local figure-ID in [w]
    float4 atten; // Light throughput/attenuation at [pos] ([xyz]); contains the local BXDF-ID in [w]
    float4 norml; // Surface normal at [pos]; contains the local distance-function type/ID in [w]
    float4 ioSrs; // In/out ray directions, expressed as solid angles
    float4 pdfIO; // Forward/reverse probability densities; ([z] contains local planetary distance
                  // (used for atmospheric refraction), [w] is unused)
};

BidirVert BuildBidirVt(float4 posFigID,
                       float4 attenBXDFID,
                       float4 normlDFType,
                       float4 thetaPhiIO,
                       float3 pdfsPlanetDist)
{
    BidirVert bdVt;
    bdVt.pos = posFigID;
    bdVt.atten = attenBXDFID;
    bdVt.norml = normlDFType;
    bdVt.ioSrs = thetaPhiIO;
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
    BidirVert lightVt;
    BidirVert camVt;
};

// A generic background color; used for contrast against dark surfaces in
// space environments with no ambient environmental light sources besides
// the local star
// Development only, release builds should use a constant [0.0f.xxx] background color
// (no human-visible light in space beyond emittance from the local star)
#define AMB_RGB 0.125f.xxx

// PDF associated with [PRayStellarSurfPos(...)], represents the probability of
// selecting a direction through the section of the stellar surface with
// absolute y-values below [MAX_PLANETARY_RING_HEIGHT]
float PRayStellarPosPDF(float starSize)
{
    // Evaluate stellar circumference at [y = MAX_PLANETARY_RING_HEIGHT]
    // and [y = 0]
    float c = (starSize - MAX_PLANETARY_RING_HEIGHT) * TWO_PI;

    // Evaluate the area of the emissive band as the circumference multiplied
    // by [MAX_PLANETARY_RING_HEIGHT]
    float a = c * MAX_PLANETARY_RING_HEIGHT;

    // Divide out squared radius to get the total solid angle within the
    // emissive area, then return the final PDF as it's reciprocal
    // (emitted rays are assumed to pass through some solid-angle region
    // as they leave the (spherical) stellar surface)
    return (starSize * starSize) / a;
}

// PDF associated with [StellarSurfPos(...), represents the probability of
// selecting any position within the spherical distribution covering the
// entire stellar surface
float StellarPosPDF()
{
    return 1.0f / FOUR_PI;
}

// A function to generate arbitrary positions on the local star;
// slightly less efficient than [PRayStellarSurfPos] for primary ray
// emission but much better for light gathers because it samples
// from a larger domain (=> lower chance for occlusion between scene
// surfaces and generated points on the star)
float3 StellarSurfPos(float starSize,
                      float3 starPos,
                      float3 randUVW)
{
    return (normalize(randUVW) * starSize) + starPos;
}

// Generate an importance-sampled source position for
// primary rays leaving the local star
// [starLinTransf] has star-position in [xyz], star size
// in [w]
float3 PRayStellarSurfPos(float4 starLinTransf,
                          float3 randUVW)
{
    // Scale [v] to the interval [0...MAX_Y]
    float yPos = randUVW.y * MAX_PLANETARY_RING_HEIGHT;

    // Place [u, w] on the circle where y = [yPos]
    float2 xzPos = normalize(randUVW.xz) *
                   (starLinTransf.w - (abs(yPos) - starLinTransf.y));

    // Construct a spherical position from the generated values,
    // then return the result
    return float3(xzPos.x, yPos, xzPos.y);
}

// Generate an importance-sampled outgoing ray direction
// from the local star towards a given point
float3 StellarDir(float3 stellarSurfPos,
                  float3 samplePoint)
{
    // Taking advantage of analytical spherical normals
    // here (n = normalize(p - centroP))
    return normalize(stellarSurfPos - samplePoint);
}

// Radiance at the given point due to direct illumination from some other
// point in the scene
float3 DirIllumRadiance(float3 traceVec,
                        float3 localNorm,
                        float3 lightRGB,
                        float3 lightPos,
                        float distToLight,
                        float lightPow)
{
    // Extract the local light direction
    // Stars are represented by spheres, and spheres
    // are known to have normals exactly equivalent
    // to points in their local space; that means we
    // can optimize out the call to our explicit
    // gradient function and use simple vector math
    // instead
    float3 lightDir = normalize(lightPos - traceVec);

    // Calculate lighting coefficients
    float nDotL = dot(localNorm, lightDir); // Vector interpretation of [cos(theta)]
    float distToLightSqr = (distToLight * distToLight);
    float3 invSquare = (lightPow / (FOUR_PI * distToLightSqr));

    // Calculate + store directly-illuminated color
    // Will (eventually) remove saturation here and use aperture settings
    // to control exposure levels instead
    return saturate(lightRGB * // RGB light spectrum/color
                   (invSquare * nDotL)); // Attenuation
}

// Return emitted spectra in RGB format for the given light source; can
// be viewed as the special case of [DirIllumRadiance(...)] when the
// irradiated point is perfectly transmittant + sitting on the surface
// of the given light source
float3 Emission(float3 srcRGB,
                float3 srcBrightness)
{
    return srcRGB * srcBrightness;
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
// [rayDest] carries the destination point in [xyz] and the target
// figure-ID in [w]
// [misInfo] carries a BXDF-sampled direction in [xyz] and whether or not
// to to trace a MIS ray in [w]
// [rayOri] carries whether or not to occlude on refraction in [w] and the ray
// origin in [xyz]
#define MAX_OCC_MARCHER_STEPS 256
float3x4 OccTest(float4 rayOri,
                 float4 rayDest,
                 float4 misInfo,
                 out float3x4 misOcc,
                 float adaptEps,
                 inout uint randVal)
{
    float3 rayDir = normalize(rayDest.xyz - rayOri.xyz);
    float2 currRayDist = (adaptEps * 2.0f).xx;
    bool2 occ = true.xx;
    bool2 actiVec = bool2(true.x, misInfo.w); // Whether or not either ray is propagating through the scene
    float2x3 endPos = float2x3(rayDest.xyz, 0.0f.xxx);
    uint2 nearID = 0x11.xx;
    float refrLoss = 0.0f; // Total energy lost from refraction between [rayOri]
                           // and the target figure
    while (any(actiVec))
    {
        float2x3 rayVecs = float2x3((rayDir * currRayDist.x) + rayOri.xyz,
                                    (misInfo.xyz * currRayDist.y) + rayOri.xyz);
        float2x3 sceneField = float2x3(SceneField(rayVecs[0],
                                                  rayOri.xyz,
                                                  true,
                                                  FILLER_SCREEN_ID), // Filler filter ID
                                       SceneField(rayVecs[1],
                                                  rayOri.xyz,
                                                  true,
                                                  FILLER_SCREEN_ID)); // Filler filter ID
        float destDist = length(rayDest.xyz - rayVecs[0]); // Naive one-dimensional distance function for the point at
                                                           // [rayDest] as perceived by the explicit shadow ray
        bool hitDest = (destDist < adaptEps) ||
                       ((sceneField[0] == rayDest.w) && rayOri.w); // Ignore figure matches if [rayOri.w] is [false]
        if ((sceneField[0].x < adaptEps ||
             hitDest) && actiVec[0])
        {
            if (hitDest)
            {
                // Assume rays that pass within [adaptEps] of the destination point or
                // reach the target figure-ID are un-occluded and return [false]
                occ = false;
                endPos = rayDest; // Clip the intersection point into [rayDest]
                actiVec.x = false; // The main shadow ray reached the destination point,
                                   // so start ignoring it here
            }
            else if (rayOri.w)
            {
                // Find the BXDF-ID at the intersection point
                uint bxdfID = MatBXDFID(MatInfo(float2x3(MAT_PROP_BXDF_FREQS,
                                                         sceneField[0].zy,
                                                         rayVecs[0])),
                                        randVal);

                // Pass through refractive surfaces
                // Invoke a transmittance check here (not all rays will have enough energy
                // left to pass through the surface)
                if (bxdfID == BXDF_ID_REFRA ||
                    bxdfID == BXDF_ID_VOLUM)
                {
                    // Update refracted ray direction
                    rayDir = MatDir(rayDir,
                                    rayVecs[0],
                                    randVal,
                                    uint3(bxdfID,
                                          sceneField[1].zy));

                    // Update refracted ray position
                    rayVecs[1] = RefractPos(rayVecs[0],
                                            rayDir,
                                            float3(bxdfID,
                                                   sceneField[1].zy),
                                            adaptEps,
                                            randVal);

                    // Reset MIS ray distance
                    currRayDist.y = 0.0f;
                }
                else
                {
                    // The main shadow ray was occluded by [sceneField[0].z], so return [false]
                    // cache the world-space position of the intersection point, and start
                    // ignoring [rayVecs[0]] here
                    occ = true;
                    endPos = rayVecs[0];
                    actiVec.x = false;
                }
            }

            // Cache the figure-ID associated with intersection point
            nearID.x = (uint)sceneField[0].y;
            actiVec.x == false;
        }
        else if (actiVec.y)
        {
            // Check for occlusion along the implicit shadow ray
            // Implicit shadow rays can be safely refracted (since they aren't assoicated with
            // a defined target point), so handle that case here as well
            if (sceneField[1].x < adaptEps)
            {
                // Find the BXDF-ID at the intersection point
                uint bxdfID = MatBXDFID(MatInfo(float2x3(MAT_PROP_BXDF_FREQS,
                                                         sceneField[1].zy,
                                                         rayVecs[1])),
                                        randVal);
                if (sceneField[1].z == rayDest.w)
                {
                    // The MIS ray reached the target figure, so write out MIS occlusion
                    // information and start ignoring it here
                    occ.y = false;
                    nearID = sceneField[1].z;
                    endPos[1] = rayVecs[1];
                    actiVec.y = false;
                }
                else
                {
                    // Pass through refractive surfaces
                    // Invoke a transmittance check here (not all rays will have enough energy
                    // left to pass through the surface)
                    if (bxdfID == BXDF_ID_REFRA ||
                        bxdfID == BXDF_ID_VOLUM)
                    {
                        // Update refracted ray direction
                        misInfo = MatDir(misInfo.xyz,
                                         rayVecs[1],
                                         randVal,
                                         uint3(bxdfID,
                                               sceneField[1].zy));

                        // Update refracted ray position
                        rayVecs[1] = RefractPos(rayVecs[1],
                                                misInfo.xyz,
                                                float3(bxdfID,
                                                       sceneField[1].zy),
                                                adaptEps,
                                                randVal);

                        // Reset MIS ray distance
                        currRayDist.y = 0.0f;
                    }
                    else
                    {
                        // The MIS ray was occluded by [sceneField[1].z], so write out MIS occlusion
                        // information and start ignoring it here
                        occ.y = true;
                        nearID = sceneField[1].z;
                        endPos[1] = rayVecs[1];
                        actiVec.y = false;
                    }
                }
            }

            // No intersections yet + [rayVec] hasn't passed within [adaptEps] units
            // of [rayDest]; continue marching through the scene
            currRayDist += float2(min(sceneField[0].x, destDist),
                                  sceneField[1].x);
        }
    }

    // Return occlusion information for the given context
    // MIS occlusion is returned in [misOcc], see above
    misOcc = float3x4(misInfo.xyz,
                      currRayDist.y,
                      endPos[1],
                      occ.y,
                      nearID.yyyy);
    return float3x4(rayDir,
                    currRayDist.x,
                    endPos[0],
                    occ.x,
                    nearID.xxxx);
}
