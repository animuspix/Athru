
// Small flag to mark whether [this] has already been
// included somewhere in the build process
#define CORE_3D_LINKED

// Enum flags for different sorts of distance
// function
#define DF_TYPE_STAR 0
#define DF_TYPE_PLANET 1
#define DF_TYPE_PLANT 2
#define DF_TYPE_CRITTER 3
#define DF_TYPE_LENS 4
#define DF_TYPE_NULL 5

// [Figure] struct + figure-buffer, modularized for sharing between volume rasterization + rendering
#include "FigureBuffer.hlsli"

// Rasterized volume atlas for the current system + plants/animals on the local planet
Buffer<float> rasterAtlas : register(t1);

// The maximum possible number of discrete figures within the
// scene
#define MAX_NUM_FIGURES 10

// Planets will (eventually) be spread evenly through a perfectly circular
// orbit around the local star (heavily simplifies star-driven direct
// illumination, makes for denser + more interesting systems); this value
// describes the radius of that orbit
#define PLANETARY_RING_RADIUS 800.0f

// The planetary ring will have a maximum positive height, and recording it
// makes it easier to focus stellar ray emissions; do that here
#define MAX_PLANETARY_RING_HEIGHT 100.0f

// Angle between planets in the ring is constant, so capture it here
#define RADS_PER_PLANET (PI / 9.0f) // Never more/less than nine planets per system

// Symmetric planetary origin (shared globally because the same SDF is being mirrored
// around the system disc)
#define SYM_PLANET_ORI float3(PLANETARY_RING_RADIUS, 0.0f.xx)

#ifndef UTILITIES_LINKED
    #include "GenericUtility.hlsli"
#endif

// Voxel interface functions, useful for sampling/smoothing volumetric information
// (like e.g. planet/plant/animal distances)
// Commented out for now because I'm not expecting to use voxel surfaces until I start working with animal
// rendering (depends on path-tracing readiness, also sculpting tools + better-defined figure inputs)

// Convert an local position to a voxel UVW value, given a position + scale
// for the nearest volume
//float3 voxUVW(float3 p,
//              float4 linTransf)
//{
//    p -= linTransf.xyz * 2.0f; // First subtraction is view->global, second subtraction is global->local
//    p += linTransf.w * 0.5f; // Convert to corner-relative positions
//    return p / linTransf.w; // Scale into [0...1]
//}
//
//// Find offset for a given position within the nearest voxel
//float3 pOffs(float3 p,
//             float4 linTransf)
//{
//    float3 uvw = voxUVW(p, linTransf) * numTraceables.z; // Generate UVW, scale out to sample width
//    return uvw - floor(uvw); // Return difference between the voxel origin [floor(uvw)] and the sample position [uvw]
//}
//
//// Convert a 3D vector + figure index + figure type + figure transformation to an index within the raster-atlas
//uint vecToNdx(float3 p,
//               uint ndx,
//               uint figType,
//               float4 linTransf)
//{
//    float cellWidth = numTraceables.z;
//    p = floor(voxUVW(p, linTransf) * cellWidth); // Scale out to the rasterization width
//    return (p.x + (p.y * figType) * cellWidth) + (p.z * (ndx + 1u)) * (cellWidth * cellWidth); // Encode/return
//}

// Primitive bounding surfaces
// These are defined by ray-tracing functions rather than distance fields
// They mostly exist to accelerate ray-marching (since we can (almost) freely discard
// bounded fields that were never going to intersect with the ray anyway)

// A traceable bounding sphere; essentially only used to accelerate ray/star queries
// [sphGeo] carries sphere position in [xyz], sphere radius in [w]
// Returns minimal distance in [x], maximal distance in [y], intersection status in [z]
// Uses Scratchapixel's intersection function, see:
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
float3 BoundingSphereTrace(float3 pt,
                           float3 ro,
                           float4 sphGeo)
{
    // Extract ray direction from [pt]
    // Spheres are the same from every angle, so no need to rotate incoming rays
    float3 rd = normalize((pt - ro) +
                          EPSILON_MIN.xxx);

    // Position the given ray relative to the bounding sphere
    ro -= sphGeo.xyz;

    // Evaluate the discriminant
    float b = 2.0f * dot(rd, ro);
    float c = dot(ro, ro) - (sphGeo.w * sphGeo.w);
    float dsc = (b * b) -
        		(4.0f * c); // Assumes normalized ray direction

    // Evaluate [t] for the given discriminant, then return
    float invB = -1.0f * b;
    float2 tt = (invB / 2.0f).xx; // Assume [dsc] is zero by default
    if (dsc > 0.0f)
    {
        float dscRt = sqrt(dsc);
        float q = -0.5f * (b + sign(b) * dscRt);
        tt = float2(q, c / q);
    }

    // Order [tt] so minimal distances are in [x] and maximal distances are
    // in [y]
    tt = float2(min(tt.x, tt.y),
                max(tt.x, tt.y));

    // Return min/max distances + intersection status
    return float3(tt, tt.x >= 0.0f && dsc >= 0.0f);
}

// A traceable bounding box, accelerates ray/planet, ray/animal, and ray/plant intersections
// Returns minimal distance in [x], maximal distance in [y], intersection status in [z]
float3 BoundingBoxTrace(float3 pt,
                        float3 ro,
                        float3 scale,
                        float3 boxOri)
{
    // Synthesize ray direction
    // Am using hybrid ray-tracing more heavily now, so might look at using [pt] as the
    // ray origin directly
    float3 rd = normalize(pt - ro);

    // Position [ro] relative to the bounding-box origin
    ro -= boxOri;

    // Synthesize box boundaries from the given origin + scale
    scale *= 0.5f;
    float2x3 bounds = float2x3(boxOri - scale,
                               boxOri + scale);
    // Evaluate per-axis distances to each plane in the box
    float2x3 vecT = float2x3((bounds[0] - ro) / rd,
                      	     (bounds[1] - ro) / rd); // Maybe possible to optimize this down to matrix division...
    // Keep near distances in [0], far distances in [1]
	vecT = float2x3(min(vecT[0], vecT[1]),
                    max(vecT[0], vecT[1]));
    // Evaluate scalar min/max distances for the given ray
    float2 sT = float2(max(max(vecT[0].x, vecT[0].y), vecT[0].z),
                       min(min(vecT[1].x, vecT[1].y), vecT[1].z));
    sT = float2(min(sT.x, sT.y), max(sT.x, sT.y)); // Keep near distance in [x], far distance in [y]
    // Return minimum/maximum distance + intersection status
    bool isect = (vecT[0].x < vecT[1].y && vecT[0].y < vecT[1].x &&
                  vecT[0].z < sT.y && sT.x < vecT[1].z);
    return float3(sT.x, sT.y, isect);
}

// Ray intersection for an arbitrary figure
#define MAX_RAY_DIST 16384.0f // Maximum supported ray distance (points beyond this are assumed to sit in the starfield)
float BoundingSurfTrace(float4 linTransf,
                        float dfType,
                        float3 pt,
                        float3 rayOri)
{
    if (dfType.x == DF_TYPE_STAR)
    {
        return BoundingSphereTrace(pt,
                                   rayOri,
                                   linTransf).z;
    }
    else
    {
        // Lots of empty space in these, but need measured distances for anything more precise
        return BoundingBoxTrace(pt,
                                rayOri,
                                linTransf.w * 2.0f,
                                linTransf.xyz).z;
    }
}

// Sphere DF here
// Returns the distance to the surface of the sphere
// (with the given radius) from the given point
// Core distance function modified from the original found within:
// http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions
float SphereDF(float3 pt,
               float4 linTransf)
{
    float3 relCoord = pt - linTransf.xyz;
    return length(relCoord) - linTransf.w;
}

// Cylinder DF here
// Returns the distance to the surface of the cylinder
// (with the given height/radius) from the given point
// Core distance function modified from the original found within:
// http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
float CylinderDF(float3 pt,
                 float2 sizes)
{
    float2 sideDist = abs(float2(length(pt.xz), pt.y)) - sizes;
    return min(max(sideDist.x, sideDist.y), 0.0) + length(max(sideDist, 0.0));
}

// Return distance to the global camera lens; assumes a unit-radius, infinitely thin convex
// lens at the camera's position
// Just used for light/camera intersection tests at the moment; might be re-used when
// (if?) I set-up realistic camera-ray emission
float LensDF(float3 pt,
             float eps,
             float4x4 iViewMat)
{
    // Place the incoming ray relative to the camera position/rotation
    pt = mul(float4(pt, 1.0f), iViewMat).xyz;

    // Baseline spherical distance, trimmed at [0.295f] units from the
    // origin
    return max(length(pt) - 1.0f,
               pt.z + 0.295f);
}

// Planetary cube DF here
// Returns the distance to the surface of the bounding cube
// (with the given width, height, and depth) from the given point
// Assumes transformations were already applied in [PlanetDF]
// Core distance function found within:
// http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float BoundCubeDF(float3 pt)
{

    // Function properties generated/extracted and stored here
    float3 edgeDistVec = abs(pt);
    return min(max(edgeDistVec.x, max(edgeDistVec.y, edgeDistVec.z)), 0.0f) + length(max(edgeDistVec, float3(0, 0, 0)));
}

// Short function to symmetrize given coordinates into the system disc
// Returns symmetrized coordinate in [xyz], polar angle within the
// disc in [w]
float4 SysSymmet(float3 pt)
{
    float2 ray2D = pt.xz;
    float theta = atan2(ray2D.y, ray2D.x + EPSILON_MIN);
    float thetaOffs = fmod(theta, RADS_PER_PLANET * 2.0f) - RADS_PER_PLANET;
    float2 scTheta;
    sincos(thetaOffs, scTheta.y, scTheta.x);
    ray2D = scTheta * length(ray2D); //min(length(ray2D), PLANETARY_RING_RADIUS * 1.40f);
    return float4(ray2D.x, pt.y, ray2D.y,
                  theta);
}

// Small convenience function converting system disc angles to planet indices
// (let us access planet-specific rotations, functions, etc.)
uint planetNdx(float theta)
{
    return min(uint(int(floor(theta / (RADS_PER_PLANET * 2.0f))) + 3) + 1u, 8u);
}

#include "Fractals.hlsli"

// Transform the given point to a planet's local space
float3 PtToPlanet(float3 pt,
                  float planetScale)
{
    // Symmetrize the given point into the system disc
    // Apply orbit here...
    pt = SysSymmet(pt).xyz;

    // Position [pt] relative to the symmetric planetary origin
    pt -= SYM_PLANET_ORI;

    // Concatenate local planetary spin for the current time into the
    // figure's rotation, then orient incoming rays appropriately

    // Synthesize a quaternion representing native planetary spin
    //float spinSpeed = planet.distCoeffs[1].w;
    //float currAngle = (sin(timeDispInfo.y) * spinSpeed) * 2.0f;
    //float4 spinQtn = Qtn(planet.distCoeffs[1].xyz, currAngle);
    //
    //// Orient rays to match the figure rotation
    //pt = QtnRotate(pt, spinQtn); // Working spin, disabled until I get a more powerful testing computer

    // Scale [pt] inversely to the given planetary scale,
    // then return
    return pt / planetScale;
}

// Small debug switch, replaces all scene surfaces with a sphere
// near the middle of the default view
//#define PLANET_DEBUG

// Return distance + surface information for the nearest planet
float2x3 PlanetDF(float3 pt,
                  Figure planet,
                  uint figID,
                  float eps)
{
    // Pass [pt] into the given planet's local space
    #ifndef PLANET_DEBUG
        pt = PtToPlanet(pt,
                        planet.scale.x);
    #endif

    // Read out the appropriate distance value
    float jDist = Julia(planet.distCoeffs[0],
                        pt);

    // Return sphere distance for debugging
    #ifdef PLANET_DEBUG
        return float2x3(max(SphereDF(pt, float4(700.0f, 0.0f, 400.0f, 100.0f)), eps * 0.9f), // Surface distance; thresholded to [eps * 0.9f] for near-surface intersections
                        DF_TYPE_PLANET, // Figure distance-field type
                        figID, // Figure ID
                        float3(700.0f, 0.0f, 400.0f)); // Fixed figure origin
    #endif

    // Return the approximate distance to the Julia fractal's surface
    // Ray scaling causes a proportional amount of error buildup in the
    // distance field, so we reverse (multiply rather than divide, with the
    // same actual value) our scaling after evaluating the distance
    // estimator; this ensures that initial distances will be scaled (since
    // we applied the division /before/ sampling the field)
    return float2x3(max(jDist * planet.scale.x, eps * 0.9f), // Scaled surface distance; thresholded to [eps * 0.9f] for near-surface intersections
                    DF_TYPE_PLANET, // Figure distance-field type
                    figID, // Figure ID
                    (float3(cos((float)figID * (RADS_PER_PLANET * 2.0f)),
                            0.0f,
                            sin((float)figID * (RADS_PER_PLANET * 2.0f))) * PLANETARY_RING_RADIUS) + systemOri.xyz) ; // Symmetric figure origin
}

// Return distance + surface information for the local star
float2x3 StarDF(float3 pt,
                float3 rayOri,
                bool useFigBounds)
{
    Figure fig = figures[0];
    float4 linTransf = float4(systemOri.xyz, fig.scale.x);
    float3 isect = BoundingSphereTrace(pt,
                                       rayOri,
                                       linTransf);
    float sphereDF = SphereDF(pt,
                              linTransf);
    if (!useFigBounds)
    {
        return float2x3(sphereDF,
                        DF_TYPE_STAR,
                        0.0f,
                        linTransf.xyz); // Stars never leave the system origin
    }
    else
    {
        return float2x3(sphereDF + (!isect.z * MAX_RAY_DIST),
                        DF_TYPE_STAR,
                        0.0f,
                        linTransf.xyz); // Stars never leave the system origin
    }
}

// Planet-specific gradient; just outputs analytical Julia gradient for now,
// will update for physical displacement later
float3 PlanetGrad(float3 samplePoint,
                  Figure fig)
{
    #ifdef PLANET_DEBUG
        return normalize(samplePoint - float3(700.0f, 0.0f, 400.0f));
    #endif
    return JuliaGrad(fig.distCoeffs[0],
                     samplePoint);
}

// Heterogeneity-preserving figure union function
// Returns the smaller of each distance ([[0].x]), along with
// an ID value ([[0].y]), a distance-field type ([[0].z]), and a
// position associated with the closest figure ([[1].xyz])
// Each half of [figInfo] (so ([0],[1]) and ([2],[3])) has the
// same layout as the return data to allow for faux recursion
// (=> nested unions)
float2x3 trackedFigUnion(float4x3 figInfo)
{
    bool distSel = figInfo[0].x > figInfo[2].x;
    return float2x3(min(figInfo[0].x, figInfo[2].x),
                    figInfo._12_32[distSel],
                    figInfo._13_33[distSel],
                    float2x3(figInfo[2],
                             figInfo[3])[distSel]);
}

// Scene distance field here
// Returns the distance to the nearest surface in the
// scene from the given point, also figure-IDs (in [z])
// and distance-field types (in [y])
// Outputs figure position in [1]
#define FILLER_SCREEN_ID 0xFFFFFFFE
float2x3 SceneField(float3 pt,
                    float3 rayOri,
                    bool useFigBounds,
                    uint screenedFig,
                    float eps)
{
    // Return the complete union carrying every figure in the scene; also
    // filter out the given figure if appropriate
    float theta = atan2(pt.y, pt.x); // Generate [theta] angle for planetary-figure selection
                                           // (planetary figures are placed with a per-ray symmetric
                                           // transformation)
    uint figID = planetNdx(theta);
    float2x3 sceneDist = PlanetDF(pt, figures[0x1], 0x1, eps);
    float2x3 starDist = StarDF(pt, rayOri, useFigBounds);
    return trackedFigUnion(float4x3(float2x3(sceneDist[0].x + ((screenedFig == 0x1) * MAX_RAY_DIST),
                                             sceneDist[0].yz,
                                             sceneDist[1]),
			   					    float2x3(starDist[0].x + ((screenedFig == 0x0) * MAX_RAY_DIST),
                                             starDist[0].yz,
                                             starDist[1])));
}
