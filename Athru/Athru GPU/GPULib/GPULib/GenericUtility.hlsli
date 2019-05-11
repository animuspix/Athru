
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

// 3D identity matrix; useful for conditionally eliding transformations between spaces
#define ID_MATRIX_3D float3x3(1, 0, 0, \
							  0, 1, 0, \
							  0, 0, 1)

// Figure-ID associated with the local star in each system
#define STELLAR_FIG_ID 0

// Useful shader input data
struct GPUInput
{
    float4 tInfo; // Delta-time in [x], current time (seconds) in [y], frame count in [z],
                  // nothing in [w]
    float4 systemOri; // Origin for the current star-system; useful for predicting figure
                      // positions during ray-marching/tracing (in [xyz], [w] is unused)
};

// Linearly scales the rotational part of the given quaternion
// while preserving the axis of rotation
float4 QtnRotationalScale(float scaleBy, float4 qtn)
{
    // eps is a very small number used to avert
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

// A Philox stream representation
// Allows us to carry counters/keys together in each element of the
// random state buffer, and avoid passing empty axes along to
// [philoxPermu]
struct PhiloStrm
{
    uint4 ctr;
    uint2 key;
};

// Small utility function to convert given integers to
// floating-point values in the range (0...1)
#define UINT_MAX 4294967296.0f
float iToFloat(uint i)
{
    return i * (1.0f / UINT_MAX);
}

// [iToFloat] expanded for vector integers
float4 iToFloatV(uint4 i)
{
    return i * (1.0f / UINT_MAX);
}

// Small utility function to convert given floating-point values
// in the range (0...1) into integers in the range (0...2^32)
uint fToInt(float f)
{
    return f * UINT_MAX;
}

// 32-bit unsigned multiplication, returns 64-bit result
// No intrinsic support in HLSL SM5, so implemented in software instead
// Modified from the implementation shown in:
// https://www.shadertoy.com/view/XslBR4
// originally from the _mulhilo_c99_tpl macro function (philox.h) within
// the Random123 source code (see below)
uint2 mulhilo(uint a, uint b)
{
    // Naively compute low multiply
    uint2 lohi = uint2(a * b, 0u);

    // Cache bitfields for high multiply
    const uint WHALF = 16u;
    const uint LOMASK = 0xFFFF;
    uint ahi = a >> WHALF;
    uint alo = a & LOMASK;
    uint bhi = b >> WHALF;
    uint blo = b & LOMASK;
    uint ahbl = ahi * blo;
    uint albh = alo * bhi;

    // Evaluate high-32 bits for result, perform carries from lower bits, return
    uint ahbl_albh = ((ahbl & LOMASK) + (albh & LOMASK));
    lohi.y = ahi * bhi + (ahbl >> WHALF) + (albh >> WHALF);
    lohi.y += ahbl_albh >> WHALF; /* carry from the sum of lo(ahbl) + lo(albh) ) */
    /* carry from the sum with alo*blo */
    lohi.y += ((lohi.x >> WHALF) < (ahbl_albh & LOMASK)) ? 1u : 0u;
    return lohi;
}

// A modified (8-round, not 7 or 10) implementation of the Philox CBPRNG,
// from the description in:
// Parallel Random Numbers: As Easy as 1, 2, 3
// available here:
// http://www.thesalmons.org/john/random123/papers/random123sc11.pdf
// and the provided source, available here:
// http://www.deshawresearch.com/downloads/download_random123.cgi/
// A useful reference implementation is available on Shadertoy at:
// https://www.shadertoy.com/view/XslBR4
// Permutes the given value through one Philox cycle, but
// avoids storing the result in [randBuf]; shaders are
// expected to process extra cycles over a single
// permutation value every time they need more pseudo-random
// numbers, and to only commit values back to [randBuf] when
// they definitely aren't going to permute any further (i.e. in
// the very last line before [main(...)] ends and shader
// execution stops for the current thread)
// This improves variation (since Philox is a "deep" RNG
// that works best over multiple cycles) and slightly
// improves performance (only two texture/buffer accesses per
// thread rather than [n], where [n] is the number of
// random numbers needed by any given shader)
uint4 philoxPermu(inout PhiloStrm strm)
{
    // Iterate the stream
    uint4 ctr = strm.ctr;
    uint2 key = strm.key;
    const uint m0 = 0xCD9E8D57; // Multiplier from the Random123 paper, page 7
    const uint m1 = 0xD2511F53; // Multiplier from the Random123 paper, page 7
    for (uint i = 0; i < 8; i += 1)
    {
        // Update state
        uint2 mul0 = mulhilo(m0, ctr.x);
        uint2 mul1 = mulhilo(m1, ctr.z);
        ctr = uint4(mul1.y ^ ctr.y ^ key.x, mul1.x,
                    mul0.y ^ ctr.z ^ key.y, mul0.x);

        // Update key
        key += uint2(0x9E3779B9, // LO-bit bump, truncated to 32 bits
                     0xBB67AE85); // HI-bit bump, truncated to 32 bits
    }
    strm.ctr = ctr;
    strm.key = key;
    // Return iterated state to the callsite
    return ctr;
}
