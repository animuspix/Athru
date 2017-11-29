
// Useful shader input data
cbuffer InputStuffs
{
    float4 cameraPos;
    matrix viewMat;
    float4 deltaTime;
    uint4 currTimeSecs;
    uint4 rendPassID;
};

// A very small number; can be used to mitigate floating-point error
// (check size relative to the number instead of trying for zero-equality)
// and for fake infinitesimals during calculus (no love for n/inf in the
// floating-point standard v_v)
#define EPSILON 0.00001f

// Opposite of epsilon, a very large number used to remove surfaces from
// [min] unions
#define OMEGA (1.0F / EPSILON);

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
#define MAX_NUM_FIGURES 11

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
    // Epsilon is a very small number used to avert
    // divisions by zero without seriously affecting
    // numerical accuracy

    float halfAngleRads = acos(qtn.w);
    float epsilon = 0.0000001f;
    float halfAngleSine = sin(halfAngleRads + epsilon);
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
    float3 relCoord = coord - fig.pos.xyz; //mul(fig.rotationQtn, float4(coord, 1)).xyz - fig.pos.xyz;
    return length(relCoord) - fig.scaleFactor.x;
}

// Return distance to the given planet
float PlanetDF(float3 coord,
               Figure planet)
{
    return SphereDF(coord, planet);
}

// Return distance to the given plant
float PlantDF(float3 coord,
              Figure plant)
{
    return CubeDF(coord, plant);
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
    if (!fig.nonNull.x)
    {
        return OMEGA;
    }

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
    float gradXA = FigDF(float3(samplePoint.x + EPSILON, samplePoint.y, samplePoint.z), fig).x;
    float gradXB = FigDF(float3(samplePoint.x - EPSILON, samplePoint.y, samplePoint.z), fig).x;

    float gradYA = FigDF(float3(samplePoint.x, samplePoint.y + EPSILON, samplePoint.z), fig).x;
    float gradYB = FigDF(float3(samplePoint.x, samplePoint.y - EPSILON, samplePoint.z), fig).x;

    float gradZA = FigDF(float3(samplePoint.x, samplePoint.y, samplePoint.z + EPSILON), fig).x;
    float gradZB = FigDF(float3(samplePoint.x, samplePoint.y, samplePoint.z - EPSILON), fig).x;

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
    float gradXA = FigDF(float3(samplePoint.x + EPSILON, samplePoint.y, samplePoint.z), fig).x;
    float gradXB = FigDF(float3(samplePoint.x - EPSILON, samplePoint.y, samplePoint.z), fig).x;

    float gradYA = FigDF(float3(samplePoint.x, samplePoint.y + EPSILON, samplePoint.z), fig).x;
    float gradYB = FigDF(float3(samplePoint.x, samplePoint.y - EPSILON, samplePoint.z), fig).x;

    float gradZA = FigDF(float3(samplePoint.x, samplePoint.y, samplePoint.z + EPSILON), fig).x;
    float gradZB = FigDF(float3(samplePoint.x, samplePoint.y, samplePoint.z - EPSILON), fig).x;

    // May require a constant scaling factor, check with Adam
    return 0.25f / (((gradXA + gradXB + gradYA + gradYB + gradZA + gradZB) - 6.0f * FigDF(samplePoint,
                                                                                          fig)) * EPSILON);
}

// Heterogeneity-preserving [min] function
// Returns the smaller of two numbers, along with
// an ID value associated with the smaller number
float2 trackedMin(float x, float y,
                  uint2 ids)
{
    return float2(min(x, y), ids[x > y]);
}

// Weigh a given distance by the parallelism of it's
// gradient to the angle of the given vector
float vecDist(float3 vec,
              Figure fig)
{
    float3 normVec = normalize(vec);
    return FigDF(vec, fig) * (1.0f - dot(normVec, grad(vec, fig).xyz));
}

// Scene distance field here
// Returns the distance to the nearest surface in the
// scene from the given point
float2 SceneField(float3 coord)
{
    // Fold field distances into a single returnable value

    // Define a super-union of just under a quarter of the figures
    // in the scene
    float2 set0 = trackedMin(vecDist(coord, figuresReadable[0]),
                             vecDist(coord, figuresReadable[1]),
                             uint2(0, 1));

    float2 set1 = trackedMin(vecDist(coord, figuresReadable[2]),
                             vecDist(coord, figuresReadable[3]),
                             uint2(2, 3));

    float2 set0u1 = trackedMin(set0.x, set1.x,
                               uint2(set0.y, set1.y));

    // Define a super-union having another quarter of the figures
    // in the scene
    float2 set2 = trackedMin(vecDist(coord, figuresReadable[4]),
                             vecDist(coord, figuresReadable[5]),
                             uint2(4, 5));

    float2 set3 = trackedMin(vecDist(coord, figuresReadable[6]),
                             vecDist(coord, figuresReadable[7]),
                             uint2(6, 7));

    float2 set2u3 = trackedMin(set2.x, set3.x,
                               uint2(set2.y, set2.y));

    // Define a hyper-union from the super-unions defined above
    float2 majoritySet = trackedMin(set0u1.x, set2u3.x,
                                    uint2(set0u1.y, set2u3.y));

    // Define an ultra-union of the hyper-union defined above +
    // two more figures
    float2 set4 = trackedMin(vecDist(coord, figuresReadable[8]),
                             vecDist(coord, figuresReadable[9]),
                             uint2(8, 9));

    float2 closeSet = trackedMin(majoritySet.x, set4.x,
                                 uint2(majoritySet.y, set4.y));

    // Return a complete union carrying every figure in the scene
    return trackedMin(closeSet.x,
                      FigDF(coord, figuresReadable[10]),
                      uint2(closeSet.y, 10));
}

// Extract the normal to a given point from the local scene
// gradient
float3 GetNormal(float3 samplePoint)
{
    float normXA = SceneField(float3(samplePoint.x + EPSILON, samplePoint.y, samplePoint.z)).x;
    float normXB = SceneField(float3(samplePoint.x - EPSILON, samplePoint.y, samplePoint.z)).x;

    float normYA = SceneField(float3(samplePoint.x, samplePoint.y + EPSILON, samplePoint.z)).x;
    float normYB = SceneField(float3(samplePoint.x, samplePoint.y - EPSILON, samplePoint.z)).x;

    float normZA = SceneField(float3(samplePoint.x, samplePoint.y, samplePoint.z + EPSILON)).x;
    float normZB = SceneField(float3(samplePoint.x, samplePoint.y, samplePoint.z - EPSILON)).x;

    return normalize(float3(normXA - normXB,
                            normYA - normYB,
                            normZA - normZB));
}

void FigUpdates(uint figID)
{
    // Copy the figure stored in the read-only input buffer
    // into a write-allowed temporary variable
    Figure currFig = figuresReadable[figID];

    // Apply angular velocity to rotation
    //currFig.rotationQtn = normalize(QtnProduct(currFig.angularVeloQtn, currFig.rotationQtn));

    // Apply velocity to position
    //currFig.pos += currFig.velocity;

    // Copy the properties defined for the current figure
    // across into the write-allowed output buffer
    figuresWritable[figID] = currFig;
}