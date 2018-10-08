
// Small flag to mark whether [this] has already been included
// somewhere in the build process
#define UTILITIES_LINKED

// Bounds on possible epsilon lengths for ray intersection, gradient, etc;
// adaptive epsilons allow for fewer steps (=> better performance) at large
// distances with lots of ray intersections but little surface detail
#define EPSILON_MAX 0.1f
#define EPSILON_MIN 0.00001f

// Approximations/fractions of pi; very useful for anything involving circles, spheres,
// hemispheres, etc.
#define QTR_PI 0.78539816339
#define HALF_PI 1.57079632679
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define FOUR_PI 12.56637061436f

// Standard compute-shader group width (2D) for path-tracing shaders
#define TRACING_GROUP_WIDTH 16

// Unit axes; useful when local directions line up with the larger spaces around them
// (e.g. normals aligned to Y in intersection space, or symmetric sampling spaces coplanar
// with [XZ] during distance estimation)
#define UNIT_X float3(1.0f, 0.0f, 0.0f)
#define UNIT_Y float3(0.0f, 1.0f, 0.0f)
#define UNIT_Z float3(0.0f, 0.0f, 1.0f)

// Identity matrix; useful for conditionally eliding transformations between spaces
#define ID_MATRIX float3x3(1, 0, 0, \
                           0, 1, 0, \
                           0, 0, 1)

// Linearly scales the rotational part of the given quaternion
// while preserving the axis of rotation
float4 QtnRotationalScale(float scaleBy, float4 qtn)
{
    // adaptEps is a very small number used to avert
    // divisions by zero without seriously affecting
    // numerical accuracy

    float halfAngleRads = acos(qtn.w);
    float epsilon = 0.0000001f;
    float halfAngleSine = sin(halfAngleRads + EPSILON_MIN);
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

// Generate a quaternion for the given axis/angle combination
float4 Qtn(float3 axis, float rads)
{
    float2 scRads;
    sincos(rads, scRads.x, scRads.y);
    return float4(axis * scRads.x, scRads.y);
}

// Perform rotation by applying the given quaternion to
// the given vector
float3 QtnRotate(float3 vec, float4 qtn)
{
    float4 qv = QtnProduct(qtn, float4(vec, 0));
    return QtnProduct(qv, QtnInverse(qtn)).xyz;
}

// State buffer for the random number generator (see [irand1D], below)
RWBuffer<uint> randBuf : register(u1);

// Length of the PRNG state buffer (see above)
// Every call to the PRNG repeats the same pattern, and after enough
// samples the pattern starts to become visible; using the largest
// numbers we can get away with reduces the time spent on any one
// part of the state and increases the time until a pattern emerges
// from the "noise" of our PRNG
#define RAND_BUF_LENGTH 88917504

// Hashes the given index into [randBuf] to reduce correlation between
// random values generated with closely spaced raw indices (like values
// in the sequence [0,1,2...])
// Index remixing implemented with the Wang hash found over here:
// http://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
uint xorshiftNdx(uint rawNdx)
{
    // Remix/hash the given index
    rawNdx = (rawNdx ^ 61) ^ (rawNdx >> 16);
    rawNdx *= 9;
    rawNdx = rawNdx ^ rawNdx >> 4;
    rawNdx *= 0x27d4eb2d;
    rawNdx = rawNdx ^ (rawNdx >> 15);

    // Thresholding since we have a finite
    // state buffer :P
    rawNdx %= RAND_BUF_LENGTH;

    // Return the transformed buffer index
    return rawNdx;
}

// Small utility function to convert given integers to
// floating-point values in the range (0...1)
#define UINT_MAX 4294967296.0f
float iToFloat(uint i)
{
    return i * (1.0f / UINT_MAX);
}

// Small utility function to convert given floating-point values
// in the range (0...1) into integers in the range (0...2^32)
uint fToInt(float f)
{
    return f * UINT_MAX;
}

// A one-dimensional implementation of the Xorshift random
// number generator
// Permutes the given value through one Xorshift cycle, but
// avoids storing the result in [randBuf]; shaders are
// expected to process extra Xorshift cycles over a single
// permutation value every time they need more pseudo-random
// numbers, and to only commit the value back to [randBuf]
// when it definitely isn't going to permute any further
// (i.e. in the very last line before [main(...)] ends and
// shader execution stops for the current thread)
// This improves variation (since Xorshift is a "deep" LCG
// that works best over multiple cycles) and slightly
// improves performance (only two texture/buffer accesses per
// thread rather than [n], where [n] is the number of
// random numbers needed by any given shader)
// Side-effects are kinda painful, but heavily simplify
// state management for [permuVal] (would need lots of temp
// variables and nested assignments otherwise...)
uint xorshiftPermu1D(inout uint permuVal)
{
    // Xorshift processing
    permuVal ^= (permuVal << 7); // Trying to avoid bad luck here...
    permuVal ^= (permuVal >> 17);
    permuVal ^= (permuVal << 5);
    return permuVal;
}
