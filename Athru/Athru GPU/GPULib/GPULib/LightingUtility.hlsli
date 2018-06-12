
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif
#include "Materials.hlsli"

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
#define LENS_FIG_ID uFFFFFFFF

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
                       float4 pdfsPlanetDist)
{
    BidirVert bdVt;
    bdVt.pos = posFigID;
    bdVt.atten = attenBXDFID;
    bdVt.norml = normlDFType;
    bdVt.ioSrs = thetaPhiIO;
    bdVt.pdfIO = pdfs;
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

// Converts the given normalized spherical (theta, phi) coordinates to a normalized
// Cartesian vector
float3 AnglesToVec(float2 thetaPhi)
{
    float4 thetaPhiSiCo;
    sincos(thetaPhi.x, thetaPhiSiCo.x, thetaPhiSiCo.y);
    sincos(thetaPhi.y, thetaPhiSiCo.z, thetaPhiSiCo.w);
    return float3(thetaPhiSiCo.x * thetaPhiSiCo.w,
                  thetaPhiSiCo.y,
                  thetaPhiSiCo.x * thetaPhiSiCo.z);
}

// Converts the given Cartesian vector to normalized spherical (theta, phi) coordinates;
// the [r] term is dropped because all the generated values are known to sit on the unit
// sphere
float2 VecToAngles(float3 rayVec)
{
    // Extract spherical coordinates, then return the result
    // Conversion function assumes y-up
    return float2(acos(rayVec.y),
                  atan(rayVec.z / rayVec.x));
}

// Generates input/output spherical angles for
// use with surface BXDFs; swaps ray directions
// lying on light paths so we can avoid calling
// specialized adjoint BSDFs during direct
// gathers + path attenuation
float4 RaysToAngles(float3 inVec,
                    float3 outVec,
                    bool lightPath)
{
    float2x2 thetaPhiIO = float2x2(VecToAngles(inVec), // Evaluate [theta, phi] for the incoming ray
                                   VecToAngles(outVec)); // Evaluate [theta, phi] for the outgoing ray

    // Return the generated angles + swap the incoming/outgoing
    // directions if they lie on a light path
    return float4(thetaPhiIO[lightPath].xy,
                  thetaPhiIO[(lightPath + 1) % 2].xy);
}

// Generates a transformation matrix for the
// local space defined by the given surface
// normal
float3x3 NormalSpace(float3 normal)
{
    // Each possible solution for the [x-basis] assumes some direction is zero and uses that
    // to solve the plane equation exactly orthogonal to the normal direction
    // I.e. given (n.x)x + (n.y)y + (n.z)z = 0 we can resolve the x-basis in three different ways
    // depending on which axis we decide to zero:
    // y = 0 >> (n.x)x + (n.z)z = 0 >> (n.x)x = 0 - (n.z)z >> n.x(x) = -n.z(z) >> (x = n.z, z = -n.x)
    // x = 0 >> (n.y)y + (n.z)z = 0 >> (n.y)y = 0 - (n.z)z >> n.y(y) = -n.z(z) >> (y = n.z, z = -n.y)
    // z = 0 >> (n.x)x + (n.y)y = 0 >> (n.x)x = 0 - (n.y)y >> n.x(x) = -n.y(y) >> (x = n.z, y = -n.x)
    // This assumption becomes less reliable as the direction chosen by any of the solutions gets further
    // and further away from zero; the most effective way to minimize that error is to use the solution
    // corresponding to the smallest direction
    // In other words...
    // When x = min(x, y, z), solve for x = 0
    // When y = min(x, y, z), solve for y = 0
    // When z = min(x, y, z), solve for z = 0
    float3 absNormal = abs(normal);
    float minAxis = min(min(absNormal.x, absNormal.y),
                        absNormal.z);
    // My minimum-selection function
    float3 basisX = 0.0f.xxx;
    //if (minAxis == absNormal.x)
    //{
    //    // Define the x-basis when [x] is the smallest axis
    //    basisX = float3(0.0f, normal.z, normal.y * -1.0f);
    //}
    //else if (minAxis == absNormal.y)
    //{
    //    // Define the x-basis when [y] is the smallest axis
    //    basisX = float3(normal.z, 0.0f, normal.x * -1.0f);
    //}
    //else
    //{
    //    // Define the x-basis when [z] is the smallest axis
    //    basisX = float3(normal.z, normal.x * -1.0f, 0.0f);
    //}

    // Scratchapixel's alternative (only defined for the x <=> y relationship, also
    // used by PBR)
    // Not totally sure why this works...should probably ask CGSE
    if (absNormal.x > absNormal.y)
    {
        basisX = float3(normal.z, 0.0f, normal.x * -1.0f);
    }
    else
    {
        basisX = float3(0.0f, normal.z, normal.y * -1.0f);
    }

    // Build the normal-space from the normal vector and the x-basis we generated before,
    // then return the result
    return float3x3(normalize(basisX),
                    normal,
                    normalize(cross(normal, basisX)));
}

// Physically-correct path tracing is strictly defined in terms of area, but it's often
// more convenient to use integrals over solid angle instead; this function provides a
// quick way to convert solid angle values into their area-based equivalents
// (conveniently unnecessary for ordinary incremental path-tracing (evaluating a given
// solid-angle PDF in terms of differential area means applying the angle-to-area
// transformation to sampled probabilities; dividing samples by that PDF means nullifying
// the original effects of the angle-to-area conversion on the path-tracing integral and
// removing it from the integral's definition), but required for BDPT (deterministic
// visibility rays >> 100% chance of tracing any given path between fixed vertices >>
// no reason to remove stochastic bias by applying a PDF))
float AngleToArea(float3 posA,
                  float3 posB,
                  float3 normal,
                  bool overSurf)
{
    // Projection over differential area is described with
    // (dA * cos(theta)) / d^2
    // for boundary surfaces, and
    // dA / d^2
    // for scattering volumes (grains are assumed small enough and uniform enough to be unaffected
    // by facing ratio)
    // This is derived from the definitions of radiance over area, where it makes more sense;
    // a differentially-sized patch of light will emit energy in the direction [posA - posB] equivalent
    // to the size of the patch ([dA]) scaled by the facing ratio of the patch towards [normal]
    // (if appropriate) and divided by the squared length of [posA - posB] (since radiance spreads out
    // over the surface area of the light volume at any given distance)
    // [dA] isn't passed in as a parameter here, but it doesn't need to be; any solid-angle value scaled
    // against the output of [this] will convert into it's area-based equivalent

    // Define values we'll need for the conversion function
    float4 outVec = float4(posB - posA, 0.0f);
    outVec.w = length(outVec.xyz);

    // Most vertices will interact with surfaces rather than media, so
    // evaluate facing ratio here
    float cosAtten = dot(normal,
                         outVec.xyz / outVec.w);
    cosAtten = (cosAtten * !overSurf) + overSurf; // Lock attenuation to [1.0f] for vertices within scattering volumes
                                                  // (like e.g. media, variable-density glass, vegetation...)
    return cosAtten / (outVec.w * outVec.w); // Scale attenuation by the squared distance between incoming/outgoing vertices, then
                                             // return the generated conversion to the callsite
}

// Specialized version of [AngleToArea(...)] for converting combined forward/reverse PDFs during MIS
float2 AnglePDFsToArea(float3 inPos,
                       float3 basePos,
                       float3 outPos,
                       float2x3 normals,
                       bool2 overSurf)
{
    // Define values we'll need for the conversion function
    float2x3 dirSet = float2x3(outPos - basePos,
                               inPos - basePos);
    float2 distVec = float2(length(dirSet[0]),
                            length(dirSet[1]));
    float2 distSqrVec = distVec * distVec;

    // Most vertices will interact with surfaces rather than media, so
    // evaluate facing ratio here
    float2 cosAtten = float2(dot(normals[0],
                                 dirSet[0] / distVec.x),
                             dot(normals[1],
                                 dirSet[1] / distVec.y));
    cosAtten = (cosAtten * !overSurf) + overSurf; // Lock attenuation to [1.0f] for vertices within scattering volumes
                                                  // (like e.g. media, variable-density glass, vegetation...)
    return cosAtten / distSqrVec; // Scale attenuation by the squared distance between incoming/outgoing vertices, then
                                  // return the generated conversion to the callsite
}

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
    float2 a = c * MAX_PLANETARY_RING_HEIGHT;

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
// [starInfo] has star-position in [xyz], star size
// in [w]
float3 PRayStellarSurfPos(float4 starInfo,
                          float3 randUVW)
{
    // Scale [v] to the interval [0...MAX_Y]
    float yPos = randUVW.y * MAX_PLANETARY_RING_HEIGHT;

    // Place [u, w] on the circle where y = [yPos]
    float2 xzPos = normalize(randUVW.xz) *
                   (starInfo.w - (abs(yPos) - starInfo.y));

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

// Specialized version of [Emission] for stellar light sources

// Simple occlusion function, traces a shadow ray between two points to test
// for visiblity (needed to validate generic sub-path connections in BDPT)
// [0].xyz contains the tracing direction, [0].w clontains the distance
// travelled by the shadow ray, [1].xyz the final position at the head of
// the shadow ray, [1].w contains the shadow ray's occlusion status
// (false => reached [rayDest] without intersecting any earlier geometry;
// true => intersected geometry between [rayOri] and [rayDest]), and
// [2].xyz contains the figure ID associated with the ray's intersection
// point (if any)
// Will need to modify this to return accumulated transmittance from
// subsurface scattering...
#define MAX_OCC_MARCHER_STEPS 256
float3x4 OccTest(float3 rayOri,
                 float3 rayDest,
                 float adaptEps)
{
    float3 rayDir = normalize(rayDest - rayOri);
    float currRayDist = adaptEps * 2.0f;
    bool occ = true;
    float3 endPos = rayDest;
    uint nearID = 11;
    for (uint i = 0; i < MAX_OCC_MARCHER_STEPS; i += 1)
    {
        float3 rayVec = (rayDir * currRayDist) + rayOri;
        float2 sceneField = SceneField(rayVec,
                                       rayOri,
                                       true,
                                       11); // Filler filter ID
        float destDist = length(rayDest - rayVec); // Naive one-dimensional distance function for the point at
                                                   // [rayDest] as perceived by [rayVec]
        bool hitDest = (destDist < adaptEps); // [rayDest] may not lie on a figure surface; handle that case here
        if (sceneField.x < adaptEps ||
            hitDest)
        {
            if (hitDest)
            {
                // Assume rays that pass within [adaptEps] of the destination point
                // are un-occluded and return [false]
                occ = false;
                endPos = rayDest; // Clip the intersection point into [rayDest]
            }
            else
            {
                // Assume intersecting rays more than [adaptEps] from the destination
                // point are occluded; return [true]
                occ = true;
                endPos = rayVec; // Cache the world-space position of the intersection point
            }

            // Cache the figure-ID associated with intersection point
            nearID = (uint)sceneField.y;
            break;
        }
        else
        {
            // No intersections yet + [rayVec] hasn't passed within [adaptEps] units
            // of [rayDest]; continue marching through the scene
            currRayDist += min(sceneField.x, destDist);
        }
    }

    // Rays might sail past the bounds defined by [adaptEps.xxx] without intersecting with
    // the scene; those rays aren't occluded by anything, so return [false] in that case
    return float3x4(rayDir,
                    currRayDist,
                    endPos,
                    occ,
                    nearID.xxxx);
}
