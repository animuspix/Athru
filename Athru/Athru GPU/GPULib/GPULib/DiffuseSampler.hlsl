#include "Materials.hlsli"

// Intersection object; carries local normal, material properties, and position
struct Isect
{
    float4 norml; // Surface normal in [xyz], source pixel ID in [w]
    float4 surf; // Surface color (rgb) and roughness (a)
    float4 spath; // Subpath ID (x), ambient real IOR (y) ([zw] are unused)
    float2x4 wiwo; // Local ray directions ([0]/[1].xyz), complex IOR (spread across 
                   // [0].a/[1].a)
}

// Diffuse intersection buffer
ConsumeStructuredBuffer<Isect> DiffBuf : register(t1);

// Path object, represents path state at a vertex within the scene
// Only carries final-vertex data because (afaict) full bi-directional path-tracing
// is too expensive for real-time
struct PxPath
{
    PathVt lastVt; // Full definition for the final vertex in the path (includes path attenuation)
    float4 pixel; // Carries filter coefficient in [x], pespective projection depth
                  // (useful for reprojection on light paths) in [y], bits from [randVal] in [z],
                  // [randVal] seed index in [w]
}

// Fullscreen camera path buffer
RWStructuredBuffer<PxPath> CPathBuf : register(u2);

// Fullscreen light path buffer
RWStructuredBuffer<PxPath> LiPathBuf : register(u3);

// 128 threads/pixel
[numthreads(8, 8, 4)]
void main()
{
    // Choose an intersection to process
    Isect isect = DiffBuf.Consume();

    // Select integration buffer
    // for the chosen intersection
    RWStructuredBuffer<PxPath> pathBuf;
    uint spathID = uint(isect.pos.a); // Unpack, cast subpath ID here
    if (spathID == CAMERA_SUBPATH) { pathBuf = CPathBuf; }
    else { spathID = LiPathBuf; }

    // Cache per-thread Xorshift value before
    // sampling the current surface
    uint randVal = PathBuf[isect.norml.w].pixel.z;

    // Generate forward/reverse PDFs
    float2 pdfs = float2(DiffusePDF(isect.wiwo[0].xyz,
                                    isect.norml.xyz),
                         DiffusePDF(isect.wiwo[1].xyz,
                                    isect.norml.xyz));

    // Evaluate diffuse BRDF
    float3 brdf = DiffuseBRDF(isect.surf,
                              float3x3(wi,
                                       isect.wiwo[1].xyz,
                                       isect.norml.xyz));

    // Update path properties
    // Properties not modified here are processed in the ray-propagation shader
    // Assumes attenuation is scaled by [1 / diffChance] during ray propagation
    pathBuf[isect.norml.w].lastVt.atten *= (brdf / (pdfs.x * PathBuf)) * abs(dot(norml, wi));
    pathBuf[isect.norml.w].lastVt.wiwo._1424 *= pdfs;

    // Commit mutations on [randVal]
    pathBuf[isect.norml.w].pixel.z = (float)randVal;
}