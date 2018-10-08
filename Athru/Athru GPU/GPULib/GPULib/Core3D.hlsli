
// Small flag to mark whether [this] has already been
// included somewhere in the build process
#define CORE_3D_LINKED

// Useful shader input data
// Considering removing figure position and passing system origin here...
cbuffer InputStuffs
{
    float4 cameraPos;
    matrix viewMat;
    matrix iViewMat;
    float4 timeDispInfo; // Delta-time in [x], current time (seconds) in [y], number of
                         // traceables for the current frame in [z], and number of patches
                         // (groups) along each dispatch axis during path-tracing (w)
    float4 prevNumTraceables; // Previous traceable-element count in [x], [yzw] are empty
    uint4 maxNumBounces;
    uint4 resInfo; // Resolution info carrier; contains app resolution in [xy],
	               // AA sampling rate in [z], and display area in [w]
};

// Enum flags for different sorts of distance
// function
#define DF_TYPE_STAR 0
#define DF_TYPE_PLANET 1
#define DF_TYPE_PLANT 2
#define DF_TYPE_CRITTER 3
#define DF_TYPE_LENS 4
#define DF_TYPE_NULL 5

// Struct representing basic data associated with
// Athru figures
struct Figure
{
    // The location + uniform scale (in [w]) of this figure at
    // any particular time
    float4 linTransf;

    // The quaternion rotation applied to this figure at
    // any particular time
    float4 rotationQtn;

    // Coefficients of the distance function used to render [this]
    float4 distCoeffs[3];
};

// A one-dimensional buffer holding the objects to render
// in each frame + their properties
StructuredBuffer<Figure> figures : register(t0);

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

#include "GenericUtility.hlsli"

// Primitive bounding surfaces
// These are defined by ray-tracing functions rather than distance fields
// They mostly exist to accelerate ray-marching (since we can (almost) freely discard
// bounded fields that were never going to intersect with the ray anyway)

// A traceable bounding sphere; essentially only used to accelerate ray/star queries
// (since stars are the only spherical/elliptical figures in Athru atm)
// Also used for planets until I find a fast way to construct bounding n-gons
// (and/or feel awake enough to implement the point-rasterization approach I wrote
// before)
// [sphGeo] carries sphere position in [xyz], sphere radius in [w]
bool BoundingSphereTrace(float3 pt,
                         float3 rayOri,
                         float4 sphGeo)
{
    // Extract ray direction from [pt]
    // Spheres are the same from every angle, so no need to rotate incoming rays
    float3 rayDir = normalize((pt - rayOri) +
                              EPSILON_MIN.xxx);

    // Position the given ray relative to the bounding sphere
    rayOri -= sphGeo.xyz;

    // First term in the discriminant
    float quadrB = 2.0f * dot(rayDir, rayOri);

    // Second term in the discriminant
    float quadrA = 1.0f; // Squared length (=> sum of component squares) of a unit vector will always be [1.0f]

    // Third term in the discriminant
    float quadrC = dot(rayOri, rayOri) - (sphGeo.w * sphGeo.w);

    // Calculate discriminant
    float quadrD = (quadrB * quadrB) -
                   (4.0f * quadrA * quadrC);

    // Intersections run with positive discriminants, so just return the polarity
    // of the output value
    // We'd take the actual roots if this were a serious ray-tracer, but we just want
    // a nice heuristic value for ray acceleration, and the discriminant is easily
    // powerful enough for that
    return (quadrD > 0.0f);
}

// A traceable plane; not useful by itself, but helpful for constructing n-gonal bounding
// surfaces
bool BoundingPlaneTrace()
{
    return false;
}

// An OBB; used to efficiently enclose non-spherical forms (trees, Athru-style planets, animals)
// Not ideal, but the generalized alternative (n-gons) is _probably_ too inefficient to justify
// Likely to avoid researching these until I have to; want to complete path tracing, take a break,
// then get back to them when they start becoming more significant (i.e. once I have to quickly
// render plants or animals as well as planets)
bool BoundingBoxTrace()
{
    // Nothing for now...want to stay focussed on path tracing until I have diffuse + refractive surfaces
    // together
    // Eventual algorithm will:
    // - Drape point lattices over the implicit from each Cartesian direction (x+/x-, y+/y-, z+/z-)
    // - Take the most distant points from the figure origin in each direction
    // - Treat the normals at these points as normals of a bounding volume
    // - Store the generated data within [distCoeffs] (might need to expand it a bit for this...)
    return BoundingPlaneTrace();
}

// Ray intersection for an arbitrary figure
#define MAX_RAY_DIST 16384.0f // Maximum supported ray distance (points beyond this are assumed to sit in the starfield)
float BoundingSurfTrace(float4 linTransf,
                        float dfType,
                        float3 pt,
                        float3 rayOri)
{
    if (dfType.x == DF_TYPE_PLANET ||
        dfType.x == DF_TYPE_STAR)
    {
        float fitting = 1.0f;
        if (dfType == DF_TYPE_PLANET) { fitting = 1.5f; }
        return BoundingSphereTrace(pt,
                                   rayOri,
                                   float4(linTransf.xyz,
										  linTransf.w * fitting));
    }
    else
    {
        return BoundingBoxTrace();
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
             float adaptEps)
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
    //// Concatenate the synthesized spin into the figure's in-built offset (if any)
    //float4 rotation = QtnProduct(spinQtn, planet.rotationQtn);
    //
    //// Orient rays to match the figure rotation
    //pt = QtnRotate(pt, planet.rotationQtn); // Working spin, disabled until I get a more powerful testing computer

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
                  float adaptEps)
{
    // Pass [pt] into the given planet's local space
    #ifndef PLANET_DEBUG
        pt = PtToPlanet(pt,
                        planet.linTransf.w);
    #endif

    // Read out the appropriate distance value
    float jDist = Julia(planet.distCoeffs[0],
                        pt,
                        ITERATIONS_JULIA,
                        adaptEps);

    // Return sphere distance for debugging
    #ifdef PLANET_DEBUG
        return float2x3(max(SphereDF(pt, float4(700.0f, 0.0f, 400.0f, 100.0f)), adaptEps * 0.9f), // Surface distance; thresholded to [adaptEps * 0.9f] for near-surface intersections
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
    return float2x3(max(jDist * planet.linTransf.w, adaptEps * 0.9f), // Scaled surface distance; thresholded to [adaptEps * 0.9f] for near-surface intersections
                    DF_TYPE_PLANET, // Figure distance-field type
                    figID, // Figure ID
                    float3(cos((float)figID * (RADS_PER_PLANET * 2.0f)),
                           0.0f,
                           sin((float)figID * (RADS_PER_PLANET * 2.0f))) * PLANETARY_RING_RADIUS); // Symmetric figure origin
}

// Return distance + surface information for the local star
float2x3 StarDF(float3 pt,
                float3 rayOri,
                bool useFigBounds)
{
    Figure fig = figures[0];
    if (useFigBounds &&
        !BoundingSphereTrace(pt,
                             rayOri,
                             fig.linTransf))
    {
        return float2x3(MAX_RAY_DIST,
                        DF_TYPE_STAR,
                        0.0f,
                        pt);
    }
    else
    {
        return float2x3(SphereDF(pt,
                                 fig.linTransf),
                        DF_TYPE_STAR,
                        0.0f,
                        fig.linTransf.xyz); // Stars never leave the system origin
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
                     samplePoint,
                     ITERATIONS_JULIA);
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
                    float adaptEps)
{
    // Return the complete union carrying every figure in the scene; also
    // filter out the given figure if appropriate
    float theta = atan2(pt.y, pt.x); // Generate [theta] angle for planetary-figure selection
                                           // (planetary figures are placed with a per-ray symmetric
                                           // transformation)
    uint figID = planetNdx(theta);
    float2x3 sceneDist = PlanetDF(pt, figures[0x1], 0x1, adaptEps);
    float2x3 starDist = StarDF(pt, rayOri, useFigBounds);
    return trackedFigUnion(float4x3(float2x3(sceneDist[0].x + ((screenedFig == 0x1) * MAX_RAY_DIST),
                                             sceneDist[0].yz,
                                             sceneDist[1]),
			   					    float2x3(starDist[0].x + ((screenedFig == 0x0) * MAX_RAY_DIST),
                                             starDist[0].yz,
                                             starDist[1])));
}
