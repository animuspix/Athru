
// Useful shader input data
cbuffer InputStuffs
{
    float4 cameraPos;
    matrix viewMat;
    float4 deltaTime;
    uint4 currTimeSecs;
    uint4 rendPassID;
};

// Very small numbers; can be used to check for ray-marcher convergence
// (since rays never really touch SDF surfaces) and for approximating
// surface gradient (since SDF gradient can be defined in terms of change
// in the field over an infinitesimal area)
#define MARCHER_EPSILON_MAX 0.1f
#define MARCHER_EPSILON_MIN 0.00001f
#define CALC_EPSILON 0.00001f

// Opposite of CALC_EPSILON, a very large number used to remove surfaces from
// [min] unions
#define OMEGA (1.0f / CALC_EPSILON);

// An approximation of pi; stored as a number rather than an inline
// function to avoid repeated divisions and/or trig operations
#define PI 3.14159265359f

// Enum flags for different sorts of distance
// function
#define DF_TYPE_PLANET 0
#define DF_TYPE_STAR 1
#define DF_TYPE_PLANT 2
#define DF_TYPE_CRITTER 3

// Struct representing basic data associated with
// Athru figures
struct Figure
{
    // The location of this figure at any particular time
    float4 pos;

    // The quaternion rotation applied to this figure at
    // any particular time
    float4 rotationQtn;

    // Uniform object scale
    float4 scaleFactor;

    // The distance function used to render [this]
    uint4 dfType;

    // Coefficients of the distance function used to render [this]
    float4 distCoeffs[10];

    // Coefficients of the color function used to tint [this]
    float4 rgbaCoeffs[10];

    // Whether or not [this] has been fully defined on the GPU
    uint4 nonNull;

    // Key marking the original object associated with [this] on
    // the CPU
    uint4 cpuOrigin;
};

// A one-dimensional buffer holding the objects to render
// in each frame + their properties
StructuredBuffer<Figure> figuresReadable : register(t0);
RWStructuredBuffer<Figure> figuresWritable : register(u0);

// The maximum possible number of discrete figures within the
// scene
#define MAX_NUM_FIGURES 10

// A simple read/write allowed unstructed buffer designed
// for use with the 32-bit Xorshift random number generator
// described here:
// https://en.wikipedia.org/wiki/Xorshift
RWBuffer<uint> randBuf : register(u1);

// The length of the state buffer defined above
#define RAND_BUF_LENGTH 16777216

// Linearly scales the rotational part of the given quaternion
// while preserving the axis of rotation
float4 QtnRotationalScale(float scaleBy, float4 qtn)
{
    // CALC_EPSILON is a very small number used to avert
    // divisions by zero without seriously affecting
    // numerical accuracy

    float halfAngleRads = acos(qtn.w);
    float epsilon = 0.0000001f;
    float halfAngleSine = sin(halfAngleRads + CALC_EPSILON);
    float3 vec = float3(qtn.x / halfAngleSine,
                        qtn.y / halfAngleSine,
                        qtn.z / halfAngleSine);

    float freshHalfAngleRads = (halfAngleRads * scaleBy);
    return float4(vec * sin(freshHalfAngleRads), cos(freshHalfAngleRads));
}

// Computes the inverse of a given quaternion
float4 QtnInverse(float4 qtn)
{
    return float4(qtn.xyz * -1.0f, qtn.w);
}

// Compute the product of two quaternions
// Assumes unit quaternion (rotation only)
float4 QtnProduct(float4 qtnA, float4 qtnB)
{
    float3 vecA = qtnA.w * qtnB.xyz;
    float3 vecB = qtnB.w * qtnA.xyz;
    float3 orthoVec = cross(qtnA.xyz, qtnB.xyz);

    return float4(vecA + vecB + orthoVec,
                 (qtnA.w * qtnB.w) - dot(qtnA.xyz, qtnB.xyz));
}

// Perform rotation by applying the given quaternion to
// the given vector
float3 QtnRotate(float3 vec, float4 qtn)
{
    float4 qv = QtnProduct(qtn, float4(vec, 0));
    return QtnProduct(qv, QtnInverse(qtn)).xyz;
}

// A one-dimensional implementation of the Xorshift random
// number generator, where [seed] is an arbitrary index to
// a random pre-defined value within [randBuf]
// Implementation uses the 32-bit algorithm described over
// here: https://en.wikipedia.org/wiki/Xorshift
float rand1D(uint seed)
{
    seed %= RAND_BUF_LENGTH;
    uint base = randBuf[seed];
    base ^= (base << 7); // Trying to avoid bad luck here...
    base ^= (base >> 17);
    base ^= (base << 5);
    randBuf[seed] = base;
    return base;
}

// Cube DF here
// Returns the distance to the surface of the cube
// (with the given width, height, and depth) from the given point
// Core distance function found within:
// http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float CubeDF(float3 coord,
              Figure fig)
{
    // Transformations (except scale) applied here
    float3 shiftedCoord = coord - fig.pos.xyz;
	float3 orientedCoord = QtnRotate(shiftedCoord, /*figuresReadable[figID].rotationQtn);*/float4(float3(0, 1, 0) * sin(shiftedCoord.y / 2), cos(shiftedCoord.x)));
    float3 freshCoord = orientedCoord;

    // Scale applied here
    float edgeLength = fig.scaleFactor.x;
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
               Figure fig)
{
    float3 relCoord = coord - fig.pos.xyz;
    return length(relCoord) - fig.scaleFactor.x;
}

// Cylinder DF here
// Returns the distance to the surface of the cylinder
// (with the given height/radius) from the given point
// Core distance function modified from the original found within:
// http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
float CylinderDF(float3 coord,
                 float2 sizes,
                 Figure fig)
{
    float2 sideDist = abs(float2(length(coord.xz), coord.y)) - sizes;
    return min(max(sideDist.x, sideDist.y), 0.0) + length(max(sideDist, 0.0));
}

#include "Fractals.hlsli"

// Return distance to the given planet
float PlanetDF(float3 coord,
               Figure planet)
{
    // Avoid iterating outside the domain of the planet by checking incoming
    // rays against a bounding sphere
    float sphDist = SphereDF(coord, planet);
    if (sphDist < (planet.scaleFactor.x * 0.5f)) // Padding distance because quaternionic Julias are closer to ellipsoids than spheres
    {
        // Translate rays to match the figure origin
        coord -= planet.pos.xyz;

        // Orient rays to match the figure rotation
        coord = QtnRotate(coord, planet.rotationQtn);

        // Not really reasonable to "scale" a fractal distance estimator, so transform the
        // incoming coordinate instead (solution courtesy of Jamie Wong's page here:
        // http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions/)
        coord /= planet.scaleFactor.x;

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
        return (0.5 * log(r) * r / dr) * planet.scaleFactor.x;
    }
    else
    {
        return sphDist;
    }
}

// Return distance to the given plant
float PlantDF(float3 coord,
              Figure plant)
{
    // Render plants as simple cylinders for now
    // Attempt bump displacement from bark properties
    coord = coord - plant.pos.xyz;
    float2 bumpACrv = plant.distCoeffs[6].xy;
    float bumpA = max(sin(coord.x * bumpACrv.x) * bumpACrv.y, plant.distCoeffs[6].w) * plant.distCoeffs[6].z;

    float2 bumpBCrv = plant.distCoeffs[7].xy;
    float bumpB = max(sin(coord.x * bumpACrv.x) * bumpACrv.y, plant.distCoeffs[7].w) * plant.distCoeffs[7].z;

    float2 bumpCCrv = plant.distCoeffs[8].xy;
    float bumpC = max(sin(coord.x * bumpCCrv.x) * bumpCCrv.y, plant.distCoeffs[8].w) * plant.distCoeffs[8].z;

    float bump = bumpA + bumpB + bumpC;
    return CylinderDF(coord, float2(plant.scaleFactor.x, plant.scaleFactor.x / 2.0f), plant) + bump;
}

// Return distance to the given critter
float CritterDF(float3 coord,
                Figure critter)
{
    return CubeDF(coord, critter);
}

// Return distance to an arbitrary figure
float FigDF(float3 coord,
            Figure fig)
{
    //if (!fig.nonNull.x)
    //{
    //    return OMEGA;
	//}

    if (fig.dfType.x == DF_TYPE_PLANET)
    {
        return PlanetDF(coord,
                        fig);
    }
	else if (fig.dfType.x == DF_TYPE_STAR)
	{
		return SphereDF(coord,
						fig);
	}
    else if (fig.dfType.x == DF_TYPE_PLANT)
    {
        return PlantDF(coord,
                       fig);
    }
    else
    {
        return CritterDF(coord,
                         fig);
    }
}

// Extract the gradient for a given subfield
// (relates to field shape and surface angle)
// Gradient scale is stored in the w component
float4 grad(float3 samplePoint,
			Figure fig)
{
    float gradXA = FigDF(float3(samplePoint.x + CALC_EPSILON, samplePoint.y, samplePoint.z), fig).x;
    float gradXB = FigDF(float3(samplePoint.x - CALC_EPSILON, samplePoint.y, samplePoint.z), fig).x;

    float gradYA = FigDF(float3(samplePoint.x, samplePoint.y + CALC_EPSILON, samplePoint.z), fig).x;
    float gradYB = FigDF(float3(samplePoint.x, samplePoint.y - CALC_EPSILON, samplePoint.z), fig).x;

    float gradZA = FigDF(float3(samplePoint.x, samplePoint.y, samplePoint.z + CALC_EPSILON), fig).x;
    float gradZB = FigDF(float3(samplePoint.x, samplePoint.y, samplePoint.z - CALC_EPSILON), fig).x;

    float3 gradVec = float3(gradXA - gradXB,
                            gradYA - gradYB,
                            gradZA - gradZB);

    float gradMag = length(gradVec);
    return float4(gradVec / gradMag, gradMag);
}

// Extract the (approximate) laplacian of a given subfield
// (relates to field curvature)
// Sourced from: https://www.shadertoy.com/view/Xts3WM (see 7-tap curvature-only function)
float3 lap(float3 samplePoint,
           Figure fig)
{
    float gradXA = FigDF(float3(samplePoint.x + CALC_EPSILON, samplePoint.y, samplePoint.z), fig).x;
    float gradXB = FigDF(float3(samplePoint.x - CALC_EPSILON, samplePoint.y, samplePoint.z), fig).x;

    float gradYA = FigDF(float3(samplePoint.x, samplePoint.y + CALC_EPSILON, samplePoint.z), fig).x;
    float gradYB = FigDF(float3(samplePoint.x, samplePoint.y - CALC_EPSILON, samplePoint.z), fig).x;

    float gradZA = FigDF(float3(samplePoint.x, samplePoint.y, samplePoint.z + CALC_EPSILON), fig).x;
    float gradZB = FigDF(float3(samplePoint.x, samplePoint.y, samplePoint.z - CALC_EPSILON), fig).x;

    // May require a constant scaling factor, check with Adam
    return 0.25f / (((gradXA + gradXB + gradYA + gradYB + gradZA + gradZB) - 6.0f * FigDF(samplePoint,
                                                                                          fig)) * CALC_EPSILON);
}

// Heterogeneity-preserving [min] function
// Returns the smaller of two numbers, along with
// an ID value associated with the smaller number
float2 trackedMin(float x, float y,
                  uint2 ids)
{
    return float2(min(x, y), ids[x > y]);
}

// Scene distance field here
// Returns the distance to the nearest surface in the
// scene from the given point
float2 SceneField(float3 coord)
{
    //return FigDF(coord, figuresReadable[1]);
    // Fold field distances into a single returnable value

    // Define a super-union of just under a quarter of the figures
    // in the scene
    float2 set0 = trackedMin(FigDF(coord, figuresReadable[0]),
                             FigDF(coord, figuresReadable[1]),
                             uint2(0, 1));

    float2 set1 = trackedMin(FigDF(coord, figuresReadable[2]),
                             FigDF(coord, figuresReadable[3]),
                             uint2(2, 3));

    float2 set0u1 = trackedMin(set0.x, set1.x,
                               uint2(set0.y, set1.y));

    // Define a super-union having another quarter of the figures
    // in the scene
    float2 set2 = trackedMin(FigDF(coord, figuresReadable[4]),
                             FigDF(coord, figuresReadable[5]),
                             uint2(4, 5));

    float2 set3 = trackedMin(FigDF(coord, figuresReadable[6]),
                             FigDF(coord, figuresReadable[7]),
                             uint2(6, 7));

    float2 set2u3 = trackedMin(set2.x, set3.x,
                               uint2(set2.y, set2.y));

    // Define a hyper-union from the super-unions defined above
    float2 majoritySet = trackedMin(set0u1.x, set2u3.x,
                                    uint2(set0u1.y, set2u3.y));

    // Define an ultra-union of the hyper-union defined above +
    // two more figures
    float2 set4 = trackedMin(FigDF(coord, figuresReadable[8]),
                             FigDF(coord, figuresReadable[9]),
                             uint2(8, 9));

    // Return a complete union carrying every figure in the scene
    return trackedMin(majoritySet.x, set4.x,
                      uint2(majoritySet.y, set4.y));
}

// Extract the normal to a given point from the local scene
// gradient
float3 GetNormal(float3 samplePoint)
{
    float normXA = SceneField(float3(samplePoint.x + CALC_EPSILON, samplePoint.y, samplePoint.z)).x;
    float normXB = SceneField(float3(samplePoint.x - CALC_EPSILON, samplePoint.y, samplePoint.z)).x;

    float normYA = SceneField(float3(samplePoint.x, samplePoint.y + CALC_EPSILON, samplePoint.z)).x;
    float normYB = SceneField(float3(samplePoint.x, samplePoint.y - CALC_EPSILON, samplePoint.z)).x;

    float normZA = SceneField(float3(samplePoint.x, samplePoint.y, samplePoint.z + CALC_EPSILON)).x;
    float normZB = SceneField(float3(samplePoint.x, samplePoint.y, samplePoint.z - CALC_EPSILON)).x;

    return normalize(float3(normXA - normXB,
                            normYA - normYB,
                            normZA - normZB));
}