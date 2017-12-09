
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

// The number of bounces to calculate per-ray
#define MAX_BOUNCES 4

// A struct that encapsulates surface material properties into
// a single six-float object
struct FigMat
{
    // x is diffuse weighting, y is specular weighting,
    // z is refractive index (ignored for now)
    float3 refl;

    // Baseline material color
    float3 rgb;
};

FigMat FigMaterial(float3 coord,
                   uint figID)
{
    FigMat mat;
    mat.refl = float3(1.0f, 0.0f, 0.0f);
    mat.rgb = float3(1.0f, 1.0f, 1.0f);
    Figure currFig = figuresReadable[figID];
    if (currFig.dfType.x == DF_TYPE_PLANET)
    {
        // Quick/dirty pseudo-procedural material
        // Should replace with a proper physically-driven alternative
        // when possible
        mat.refl = float3(currFig.rgbaCoeffs[0].x,
                          currFig.rgbaCoeffs[0].y,
                          currFig.rgbaCoeffs[0].z);

        // Should replace with a physically-driven color function
        // when possible
        mat.rgb = float3(currFig.rgbaCoeffs[1].x,
                         currFig.rgbaCoeffs[1].y,
                         currFig.rgbaCoeffs[1].z);
        return mat;
    }
    else if (currFig.dfType.x == DF_TYPE_STAR)
    {
        mat.refl = float3(1.0f, 0.0f, 0.0f);
        mat.rgb = float3(currFig.rgbaCoeffs[0].x,
                         currFig.rgbaCoeffs[0].y,
                         currFig.rgbaCoeffs[0].z);
        return mat;
    }
    else
    {
        return mat;
    }
}

// Reflectance function for diffuse surfaces
// Invented by fizzer (http://www.amietia.com/lambertnotangent.html)
// and shared by iq (https://www.shadertoy.com/view/MsdGzl)
float3 DiffuseBRDF(uint rayID,
                   float3 norm)
{
    // Cosine distribution function (diffuse BRDF) invented by fizzer:
    // http://www.amietia.com/lambertnotangent.html
    // and shared by iq:
    // https://www.shadertoy.com/view/MsdGzl
    float a = 6.2831853 * (1.0f / rand1D(rayID));
    float u = 2.0 * (1.0f / rand1D(rayID) * 0.25) - 1.0;
    return normalize(norm + float3(sqrt(1.0 - u * u) * float2(cos(a), sin(a)), u));
}

// Reflectance function for specular surfaces
float3 SpecBRDF(float3 coord,
                float3 norm)
{
    return reflect(coord, norm);
}

// Reflectance function for refractive surfaces
float3 RefracBRDF()
{
    // Does nothing, need more research for refractive/dispersive math
}

float3 PerPointDirectIllum(float3 samplePoint, float3 baseRGB,
                           float3 localNorm)
{
    // Initialise lighting properties
    float3 lightPos = float3(0.0f, 5.0f, -5.0f);
    float3 lightVec = (lightPos - samplePoint);

    // We know the point illuminated by the current ray is lit, so light it
    float nDotL = dot(localNorm, normalize(lightVec));
    float distToLight = FigDF(samplePoint, figuresReadable[0]);
    float albedo = 0.18f;
    float3 invSquare = ((2000 * albedo) / (4 * PI * (distToLight * distToLight)));
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
