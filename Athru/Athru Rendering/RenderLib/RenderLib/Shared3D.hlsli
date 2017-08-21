
// Useful shader input data
cbuffer InputStuffs
{
    float4 cameraPos;
    matrix viewMat;
    float4 deltaTime;
    uint4 currTimeSecs;
    uint4 numFigures;
    uint4 rendPassID;
};

// Struct representing renderable objects passed in from the
// CPU
struct Figure
{
    // Where this figure is going + how quickly it's going
    // there
    float4 velocity;

    // The location of this figure at any particular time
    float4 pos;

    // How this figure is spinning, defined as radians/second
    // about an implicit angle
    float4 angularVeloQtn;

    // The quaternion rotation applied to this figure at
    // any particular time
    float4 rotationQtn;

    // Uniform object scale
    float4 scaleFactor;

    // The distance function used to render [this]
    uint4 dfType;

    // Array of coefficients associated with the distance function
    // used to render [this]
    //float coeffs[10];

    // Fractal palette vector (bitmasked against function
    // properties to produce procedural colors)
    float4 surfRGBA;

    // Key marking the original object associated with [this] on
    // the CPU
    uint4 origin;
};

// A one-dimensional buffer holding the objects to render
// in each frame + their properties
StructuredBuffer<Figure> figuresReadable : register(t0);
RWStructuredBuffer<Figure> figuresWritable : register(u0);

// The maximum possible number of discrete figures within the
// scene
#define MAX_NUM_FIGURES 6

// Struct containing per-point data for any given distance
// function
struct DFData
{
    float dist;
    float4 rgbaColor;
    uint id;
};

// A simple read/write allowed unstructed buffer designed
// for use with the 32-bit Xorshift random number generator
// described here:
// https://en.wikipedia.org/wiki/Xorshift
RWBuffer<uint> randBuf : register(u1);

// The length of the state buffer defined above
#define RAND_BUF_LENGTH 1024

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
DFData CubeDF(float3 coord,
              uint figID)
{
    // Transformations (except scale) applied here
    float3 shiftedCoord = coord - figuresReadable[figID].pos.xyz;
	float3 orientedCoord = QtnRotate(shiftedCoord, /*figuresReadable[figID].rotationQtn);*/float4(float3(0, 1, 0) * sin(coord.y / 2), cos(coord.y / 2)));
    float3 freshCoord = orientedCoord;

    // Scale applied here
    float edgeLength = figuresReadable[figID].scaleFactor.x;
    float3 edgeDistVec = abs(freshCoord) - edgeLength.xxx;

    // Function properties generated/extracted and stored here
    DFData dfData;
    dfData.dist = min(max(edgeDistVec.x, max(edgeDistVec.y, edgeDistVec.z)), 0.0f) + length(max(edgeDistVec, float3(0, 0, 0)));
    dfData.rgbaColor = figuresReadable[figID].surfRGBA;
    dfData.rgbaColor.rgb = 1.0f.xxx; //float3(sinh(abs(freshCoord.y + freshCoord.x)), cosh(freshCoord.z), 1.0f) * (freshCoord.y + 1.0f);
    dfData.id = figID;
    return dfData;
}

// Sphere DF here
// Returns the distance to the surface of the sphere
// (with the given radius) from the given point
// Core distance function modified from the original found within:
// http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions
DFData SphereDF(float3 coord,
                uint figID)
{
    float3 relCoord = coord - figuresReadable[figID].pos.xyz; //mul(fig.rotationQtn, float4(coord, 1)).xyz - fig.pos.xyz;
    DFData dfData;
    dfData.dist = length(relCoord) - figuresReadable[figID].scaleFactor.x;
    dfData.rgbaColor = figuresReadable[figID].surfRGBA;
    dfData.id = figID;
    return dfData;
}

void FigUpdates(uint figID)
{
    // Copy the figure stored in the read-only input buffer
    // into a write-allowed temporary variable
    Figure currFig = figuresReadable[figID];

    // Apply angular velocity to rotation
    currFig.rotationQtn = normalize(QtnProduct(currFig.angularVeloQtn, currFig.rotationQtn));

    // Apply velocity to position
    currFig.pos += currFig.velocity;

    // Copy the properties defined for the current figure
    // across into the write-allowed output buffer
    figuresWritable[figID] = currFig;
}

// A magical error term, used because close rays might intersect
// each other during shadowing/reflection/refraction or "skip" through
// the scene surface because of floating-point error; this allows us to
// specify a range of valid values between +rayEpsilon and -rayEpsilon
// for those situations :)
#define rayEpsilon 0.00001f
