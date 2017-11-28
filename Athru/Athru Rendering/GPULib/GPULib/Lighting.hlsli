
#include "Core3D.hlsli"

// A two-dimensional texture representing the display; used
// as the rasterization target during ray-marching
RWTexture2D<float4> displayTex : register(u2);

// The width and height of the display texture defined above
#define DISPLAY_WIDTH 1024
#define DISPLAY_HEIGHT 768

// A struct combining path contribution pixel coordinates and
// path contribution colors to allow for top-down progressive
// rendering
struct TracePix
{
    float4 pixCoord;
    float4 pixRGBA;
};

// A buffer containing per-pixel path contributions to allow for easier
// pixel integration during path-tracing
RWStructuredBuffer<TracePix> pixTraces : register(u3);

// A write-only buffer containing parallel primary contributions (not really possible
// to simultaneously increment a single GI vector across multiple parallel threads)
// for each pixel within the display
RWStructuredBuffer<TracePix> giCalcBufWritable : register(u4);

// A read-only buffer containing parallel primary contributions (not really possible
// to simultaneously increment a single GI vector across multiple parallel threads)
// for each pixel within the display
StructuredBuffer<TracePix> giCalcBufReadable : register(t1);

// How many samples to take for each pixel during
// global illumination
#define GI_SAMPLES_PER_RAY 256

// The total number of primary samples processed in each frame
#define GI_SAMPLE_TOTAL ((DISPLAY_WIDTH * 64) * GI_SAMPLES_PER_RAY)

float3 PerPointDirectIllum(float3 samplePoint, float3 baseRGB,
                           float3 localNorm)
{
    // Initialise lighting properties
    float3 lightPos = float3(0.0f, 5.0f, -5.0f);
    float3 reflectVec = (lightPos - samplePoint);

    // We know the point illuminated by the current ray is lit, so light it
    float pi = 3.14159;
    float nDotL = dot(localNorm, normalize(reflectVec));
    float distToLight = length(reflectVec);
    float albedo = 0.18f;
    float3 invSquare = ((2000 * albedo) / (4 * pi * (distToLight * distToLight)));
    float3 lightRGB = float3(1.0f, 1.0f, 1.0f);

    // Formula for directional light:
    // saturate(sampleRGB * ((albedo / pi) * intensity * lightRGB * nDotL));

    // Formula for point light:
    // let invSquare = ((intensity * albedo) / (4 * pi * (distToLight * distToLight)));
    // then light = saturate(sampleRGB * (invSquare * lightRGB * nDotL));

    // Direct illumination calculated here
    //return saturate(baseRGB * ((albedo / pi) * 2000 * lightRGB * nDotL));
    return (baseRGB * (invSquare * lightRGB * nDotL));
}
