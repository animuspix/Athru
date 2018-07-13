
// Small flag to mark whether [this] has already been
// included somewhere in the build process
#define CORE_3D_LINKED

// Useful shader input data
cbuffer InputStuffs
{
    float4 cameraPos;
    matrix viewMat;
    matrix iViewMat;
    float4 timeDispInfo; // Delta-time in [x], current time (seconds) in [y], number of
                         // traceables for the current frame in [z], and number of patches
                         // (groups) along each dispatch axis during path-tracing (w)
    uint4 maxNumBounces;
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

    // Vector of core information associated with [this]; contains
    // a distance-function ID in [x] and the low/high bits of [this]'s
    // CPU address in [yz] ([w] is unused)
    uint4 self;
};

// A one-dimensional buffer holding the objects to render
// in each frame + their properties
StructuredBuffer<Figure> figuresReadable : register(t0);
RWStructuredBuffer<Figure> figuresWritable : register(u0);

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

#include "GenericUtility.hlsli"

// Cube DF here
// Returns the distance to the surface of the cube
// (with the given width, height, and depth) from the given point
// Core distance function found within:
// http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float CubeDF(float3 coord,
             float4 linTransf,
             float4 rotationQtn)
{
    // Transformations (except scale) applied here
    float3 shiftedCoord = coord - linTransf.xyz;
    float3 orientedCoord = QtnRotate(shiftedCoord, rotationQtn);
    float3 freshCoord = orientedCoord;

    // Scale applied here
    float edgeLength = linTransf.w;
    float3 edgeDistVec = abs(freshCoord) - edgeLength.xxx;

    // Function properties generated/extracted and stored here
    return min(max(edgeDistVec.x, max(edgeDistVec.y, edgeDistVec.z)), 0.0f) + length(max(edgeDistVec, float3(0, 0, 0)));
}

// Sphere DF here
// Returns the distance to the surface of the sphere
// (with the given radius) from the given point
// Core distance function modified from the original found within:
// http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions
float SphereDF(float3 coord,
               float4 linTransf)
{
    float3 relCoord = coord - linTransf.xyz;
    return length(relCoord) - linTransf.w;
}

// Cylinder DF here
// Returns the distance to the surface of the cylinder
// (with the given height/radius) from the given point
// Core distance function modified from the original found within:
// http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
float CylinderDF(float3 coord,
                 float2 sizes)
{
    float2 sideDist = abs(float2(length(coord.xz), coord.y)) - sizes;
    return min(max(sideDist.x, sideDist.y), 0.0) + length(max(sideDist, 0.0));
}

// Return distance to the global camera lens; assumes a unit-radius, infinitely thin convex
// lens at the camera's position
// Just used for light/camera intersection tests at the moment; might be re-used when
// (if?) I set-up realistic camera-ray emission
float LensDF(float3 coord,
             float adaptEps)
{
    // Place the incoming ray relative to the camera position/rotation
    coord = mul(float4(coord, 1.0f), iViewMat).xyz;

    // Baseline spherical distance, trimmed at [0.295f] units from the
    // origin
    return max(length(coord) - 1.0f,
               coord.z + 0.295f);
}

#include "Fractals.hlsli"

// Return distance to the given planet (in [x]), also planet position for the
// current time (in [yzw])
float4 PlanetDF(float3 coord,
                Figure planet)
{
    // Calculate a trivial planetary orbit for the current time
    float3 planetPos = float3(cos(timeDispInfo.y * planet.distCoeffs[2].x) * planet.linTransf.x,
                              0.0f,
                              sin(timeDispInfo.y * planet.distCoeffs[2].x) * planet.linTransf.z) +
                       planet.linTransf.xyz;

    // Translate rays to match the planet's orbit
    coord -= planetPos;

    // Concatenate local planetary spin for the current time into the
    // figure's rotation, then orient incoming rays appropriately

    // Synthesize a quaternion representing native planetary spin
    float spinSpeed = planet.distCoeffs[1].w;
    float currAngle = (sin(timeDispInfo.y) * spinSpeed) * 2.0f;
    float4 spinQtn = Qtn(planet.distCoeffs[1].xyz, currAngle);

    // Concatenate the synthesized spin into the figure's in-built offset (if any)
    float4 rotation = QtnProduct(spinQtn, planet.rotationQtn);

    // Orient rays to match the figure rotation
    coord = QtnRotate(coord, planet.rotationQtn); // Working spin, disabled until I get a more powerful testing computer

    // Not really reasonable to "scale" a fractal distance estimator, so transform the
    // incoming coordinate instead (solution courtesy of Jamie Wong's page here:
    // http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions/)
    coord /= planet.linTransf.w;

    // Iterate the Julia distance estimator for the given number of iterations
    JuliaProps julia = Julia(planet.distCoeffs[0],
                             coord,
                             ITERATIONS_JULIA);

    // Catch the iterated length of the initial sample
    // point + the iterated length of the Julia
    // derivative [dz]
    float r = length(julia.z);
    float dr = length(julia.dz);

    // Return the approximate distance to the Julia fractal's surface
    // Ray scaling causes a proportional amount of error buildup in the
    // distance field, so we reverse (multiply rather than divide, with the
    // same actual value) our scaling after evaluating the distance
    // estimator; this ensures that initial distances will be scaled (since
    // we applied the division /before/ sampling the field)
    return float4((0.5 * log(r) * r / dr) * planet.linTransf.w,
                  planetPos); // Orbital planet position

}

// Return distance to the given plant (in [x]) and plant position
// (in [yzw])
float4 PlantDF(float3 coord,
               float4 linTransf)
{
    // Render plants as simple cylinders for now
    return float4(CylinderDF(coord, float2(linTransf.w, linTransf.w / 2.0f)),
                  linTransf.xyz); // Non-displaced figure position (no transformations for now)
}

// Return distance to the given critter (in [x]) and critter position (in [yzw])
float4 CritterDF(float3 coord,
                 float4 linTransf)
{
    return float4(CubeDF(coord,
                         linTransf,
                         Qtn(float3(0.0f, 1.0f, 0.0f), sin(timeDispInfo.y))),
                    linTransf.xyz); // Non-displaced figure position (no transformations for now)
}

float CubeDFProcedural(float3 coord,
                       float3 pos,
                       float3 scaleVec,
                       float4 rotationQtn)
{
    // Transformations (except scale) applied here
    float3 shiftedCoord = coord - pos;
    float3 orientedCoord = QtnRotate(shiftedCoord, rotationQtn);
    float3 freshCoord = orientedCoord;

    // Scale applied here
    float3 edgeDistVec = abs(freshCoord) - scaleVec;

    // Function properties generated/extracted and stored here
    return min(max(edgeDistVec.x, max(edgeDistVec.y, edgeDistVec.z)), 0.0f) + length(max(edgeDistVec, float3(0, 0, 0)));
}

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
bool BoundingSphereTrace(float3 coord,
                         float3 rayOri,
                         float4 sphGeo)
{
    // Extract ray direction from [coord]
    // Spheres are the same from every angle, so no need to rotate incoming rays
    float3 rayDir = normalize((coord - rayOri) +
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
                        float3 coord,
                        float3 rayOri)
{
    if (dfType.x == DF_TYPE_PLANET ||
        dfType.x == DF_TYPE_STAR)
    {
        float fitting = 1.0f;
        if (dfType == DF_TYPE_PLANET) { fitting = 1.5f; }
        return BoundingSphereTrace(coord,
                                   rayOri,
                                   float4(linTransf.xyz,
										  linTransf.w * fitting));
    }
    else
    {
        return BoundingBoxTrace();
    }
}

// Return distance to an arbitrary figure ([0].x) + the
// distance function associated with the given
// figure ([0].y) ([z] is unused)
// [1] carries the world-space origin for
// each figure (known ahead of time for stars, but
// not for procedurally-displaced figures like planets,
// critters, plants, etc.)
float2x3 FigDF(float3 coord,
               float3 rayOri,
               bool useFigBounds,
               Figure fig)
{
    if (useFigBounds &&
        !BoundingSurfTrace(fig.linTransf,
                           fig.self.x,
                           coord,
                           rayOri))
    {
        return float2x3(MAX_RAY_DIST,
                        fig.self.x,
                        0.0f,
                        coord);
    }
    else
    {
        switch(fig.self.x)
        {
            case DF_TYPE_PLANET:
                return float2x3(SphereDF(coord,
                                         fig.linTransf),
                                fig.self.x,
                                0.0f, // No info for [z] atm...
                                fig.linTransf.xyz); // No planetary displacement during debugging
            case DF_TYPE_STAR:
                return float2x3(SphereDF(coord,
                                       fig.linTransf),
                                fig.self.x,
                                0.0f,
                                fig.linTransf.xyz); // Stars never leave the system origin
            case DF_TYPE_PLANT:
                float4 plantDF = PlantDF(coord,
                                         fig.linTransf); // Locally-cached DF for easy reordering at return
                return float2x3(plantDF.x,
                                fig.self.x,
                                0.0f,
                                plantDF.yzw);
            default:
                float4 critterDF = CritterDF(coord,
                                             fig.linTransf); // Locally-cached DF for easy reordering at return
                return float2x3(critterDF.x,
                                fig.self.x,
                                0.0f,
                                critterDF.yzw);
        }
    }
}

// Extract the tetrahedral gradient for a given
// subfield; marginally faster than the six-point
// approximation used by the main [grad] function,
// but also less accurate
// Sourced from shadertoy user nimitz: https://www.shadertoy.com/view/Xts3WM
// (see five-tap normal/curvature function)
float4 tetGrad(float3 samplePoint,
               float adaptEps,
               Figure fig)
{
    float2 e = float2(-1.0f, 1.0f) * adaptEps;
    float t1 = FigDF(samplePoint + e.yxx, 0.0f.xxx, false, fig)[0].x;
    float t2 = FigDF(samplePoint + e.xxy, 0.0f.xxx, false, fig)[0].x;
    float t3 = FigDF(samplePoint + e.xyx, 0.0f.xxx, false, fig)[0].x;
    float t4 = FigDF(samplePoint + e.yyy, 0.0f.xxx, false, fig)[0].x;

    float3 gradVec = t1 * e.yxx +
                     t2 * e.xxy +
                     t3 * e.xyx +
                     t4 * e.yyy;

    float gradMag = length(gradVec);
    return float4(gradVec / gradMag, gradMag);
}

// Extract the gradient for a given subfield
// (relates to field shape and surface angle)
// Gradient scale is stored in the w component
float4 grad(float3 samplePoint,
            float adaptEps,
			Figure fig)
{
    float gradXA = FigDF(float3(samplePoint.x + adaptEps, samplePoint.y, samplePoint.z), 0.0f.xxx, false, fig)[0].x;
    float gradXB = FigDF(float3(samplePoint.x - adaptEps, samplePoint.y, samplePoint.z), 0.0f.xxx, false, fig)[0].x;
    float gradYA = FigDF(float3(samplePoint.x, samplePoint.y + adaptEps, samplePoint.z), 0.0f.xxx, false, fig)[0].x;
    float gradYB = FigDF(float3(samplePoint.x, samplePoint.y - adaptEps, samplePoint.z), 0.0f.xxx, false, fig)[0].x;
    float gradZA = FigDF(float3(samplePoint.x, samplePoint.y, samplePoint.z + adaptEps), 0.0f.xxx, false, fig)[0].x;
    float gradZB = FigDF(float3(samplePoint.x, samplePoint.y, samplePoint.z - adaptEps), 0.0f.xxx, false, fig)[0].x;

    float3 gradVec = float3(gradXA - gradXB,
                            gradYA - gradYB,
                            gradZA - gradZB);

    float gradMag = length(gradVec);
    return float4(gradVec / gradMag, gradMag);
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
// scene from the given point, also figure-IDs (in [y])
// and distance-field types (in [z])
// Outputs figure position in
#define FILLER_SCREEN_ID 0xFFFFFFFE
float2x3 SceneField(float3 coord,
                    float3 rayOri,
                    bool useFigBounds,
                    uint screenedFig)
{
    // Fold field distances into a single returnable value
    // Sparse matrix used to insert ID values into the data
    // returned by each distance function
    const int2x3 idMat = float2x3(0.0f.xx, 1.0f,
                                  0.0f.xxx);

    // Define a union over the first eight figures in the scene
    float2x3 setA = trackedFigUnion(float4x3(trackedFigUnion(float4x3(trackedFigUnion(float4x3(FigDF(coord, rayOri, useFigBounds, figuresReadable[0]) + (idMat * 0),
									                                                           FigDF(coord, rayOri, useFigBounds, figuresReadable[1]) + (idMat * 1))),
                                                                      trackedFigUnion(float4x3(FigDF(coord, rayOri, useFigBounds, figuresReadable[2]) + (idMat * 2),
	                                                                  					       FigDF(coord, rayOri, useFigBounds, figuresReadable[3]) + (idMat * 3))))),
                                             trackedFigUnion(float4x3(trackedFigUnion(float4x3(FigDF(coord, rayOri, useFigBounds, figuresReadable[4]) + (idMat * 4),
									                                                           FigDF(coord, rayOri, useFigBounds, figuresReadable[5]) + (idMat * 5))),
                                                                      trackedFigUnion(float4x3(FigDF(coord, rayOri, useFigBounds, figuresReadable[6]) + (idMat * 6),
									                                                           FigDF(coord, rayOri, useFigBounds, figuresReadable[7]) + (idMat * 7)))))));
    // Define another union from the last two scene figures
    float2x3 setB = trackedFigUnion(float4x3(FigDF(coord, rayOri, useFigBounds, figuresReadable[8]),
			   					             FigDF(coord, rayOri, useFigBounds, figuresReadable[9])));

    // Return the complete union carrying every figure in the scene; also
    // filter out the given figure if appropriate
    float setAScreen = (((int)screenedFig == setA[0].y) * MAX_RAY_DIST);
    float setBScreen = (((int)screenedFig == setB[0].y) * MAX_RAY_DIST);
    return trackedFigUnion(float4x3(float2x3(setA[0].x + setAScreen, setA[0].yz,
                                             setA[1]),
                                    float2x3(setB[0].x + setBScreen, setB[0].yz,
                                             setB[1])));
}
