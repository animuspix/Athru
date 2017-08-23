
#include "Shared3D.hlsli"

// A two-dimensional texture representing the display; used
// as the rasterization target during ray-marching
RWTexture2D<float4> displayTex : register(u2);

// The width and height of the display texture defined above
#define DISPLAY_WIDTH 1024
#define DISPLAY_HEIGHT 768

// A struct containing points to be path-traced + indices into the corresponding
// pixels in the display texture
struct TracePoint
{
    float4 coord;
    float4 rgbaSrc;
    uint4 figID;
    uint4 isValid;
};

// A structured buffer containing the traceable points above
RWStructuredBuffer<TracePoint> traceables : register(u3);

// Scene distance field here
// Returns the distance to the nearest surface in the
// scene from the given point
DFData SceneField(float3 coord)
{
    // Array serving as a read/write target for the distance
    // functions associated with each object in the scene
    DFData dfArr[MAX_NUM_FIGURES];

    // Extract distances to each figure passed in from the CPU
    const uint sphereDFID = 0;
    const uint cubeDFID = 1;
    uint minIndex = 0;
    for (uint i = 0; i < numFigures.x; i += 1)
    {
        if (figuresReadable[i].dfType.x == sphereDFID)
        {
            dfArr[i] = SphereDF(coord, i);
        }
        else if (figuresReadable[i].dfType.x == cubeDFID)
        {
            dfArr[i] = CubeDF(coord, i);
        }

        if (dfArr[i].dist < dfArr[minIndex].dist)
        {
            minIndex = i;
        }
    }

    // Return the data associated with the closest function
    // to the calling location :)
    DFData sceneDF = dfArr[minIndex];
    return sceneDF;
}

// Extract the normal to a given point from the local scene
// gradient
float3 GetNormal(float3 samplePoint)
{
    float normXA = SceneField(float3(samplePoint.x + rayEpsilon, samplePoint.y, samplePoint.z)).dist;
    float normXB = SceneField(float3(samplePoint.x - rayEpsilon, samplePoint.y, samplePoint.z)).dist;

    float normYA = SceneField(float3(samplePoint.x, samplePoint.y + rayEpsilon, samplePoint.z)).dist;
    float normYB = SceneField(float3(samplePoint.x, samplePoint.y - rayEpsilon, samplePoint.z)).dist;

    float normZA = SceneField(float3(samplePoint.x, samplePoint.y, samplePoint.z + rayEpsilon)).dist;
    float normZB = SceneField(float3(samplePoint.x, samplePoint.y, samplePoint.z - rayEpsilon)).dist;

    return normalize(float3(normXA - normXB,
                            normYA - normYB,
                            normZA - normZB));
}

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
    return saturate(baseRGB * (invSquare * lightRGB * nDotL));
}
