// Powered by magical raymarched rendering;
// techniques implemented from this tutorial:
// http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions/

// Although that tutorial specifically discusses signed distance functions,
// unsigned functions exist as well; this shader calls every distance
// function [DF] to abstract away the distinction (since it doesn't make a
// significant difference to the rendered output anyways)

// Specific ray-based approaches to light transport mined from
// Scratchapixel:
// https://www.scratchapixel.com/index.php
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-ray-tracing/raytracing-algorithm-in-a-nutshell
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-ray-tracing/implementing-the-raytracing-algorithm
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-ray-tracing/adding-reflection-and-refraction
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/diffuse-lambertian-shading
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/ligth-and-shadows
// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/shading-spherical-light

// Useful shader input data
cbuffer InputStuffs
{
    float4 cameraPos;
    matrix viewMat;
    float4 deltaTime;
    uint4 currTimeSecs;
    uint4 numFigures;
};

// A two-dimensional texture representing the display; used
// as the rasterization target during ray-marching
RWTexture2D<float4> displayTex : register(u0);

// The width and height of the display texture defined above
static const uint DISPLAY_WIDTH = 1024;
static const uint DISPLAY_HEIGHT = 768;

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

    // The quaternion rotationQtn applied to this figure at
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
// in each frame + their properties; will be integrated into
// the scene distance function
StructuredBuffer<Figure> figuresReadable : register(t0);
RWStructuredBuffer<Figure> figuresWritable : register(u1);
// The maximum possible number of discrete figures within the
// scene
// Kept as a #define because strange compiler bugs turn up if
// it's made into a [static] [const] variable
#define MAX_NUM_FIGURES 6

// Struct containing per-point data for any given distance
// function
struct DFData
{
    float dist;
    float4 rgbaColor;
    uint id;
};

// The maximum number of steps allowed for the primary
// ray-marcher
// Kept as a #define because strange compiler bugs turn up if
// it's made into a [static] [const] variable
#define MAX_VIS_MARCHER_STEPS 255

// The maximum number of steps allowed for the shadow
// ray-marcher
// Kept as a #define because strange compiler bugs turn up if
// it's made into a [static] [const] variable
#define MAX_OCC_MARCHER_STEPS 12

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

// Copy of the canonical trigonometric PRNG used in GLSL/HLSL
// ([noise] is deprecated for Shader Model 2 and higher)
// Slightly modified from the version shared by Keijiro on
// Github Gist:
// https://gist.github.com/keijiro/ee7bc388272548396870
//
// This version just works by forcing the seed into a very steep
// sine curve, then restricting output to the fractional part to
// make it look more variable (since the integers are a subset
// of the reals, it makes sense that the reals are going to seem
// much more variable than the integers for a continuous curve
// like [sin(...)]). The original is a bit more complicated, and
// there's some good descriptions of it available over here:
// https://stackoverflow.com/questions/12964279/whats-the-origin-of-this-glsl-rand-one-liner
//
// Note: This is a very chaotic hash, not a "real" random
// number generator. If I'm going to be using GPU random
// numbers a lot I need to set up an actual stateful
// generator using a proven algorithm like e.g. Xorshift
// It's mostly just an example, but there's a description
// of Xorshift available over here:
//  https://en.wikipedia.org/wiki/Xorshift
float rand1D(float seed)
{
    return frac(sin(seed) * 43758.5453);
}

// Specialized (z-parallel) plane DF here
// Returns the distance to the surface of the plane
// parallel to z from the given point and at the
// given y-offset
DFData FloorDF(float3 coord, float planeElev)
{
    planeElev += 0.1f;
    DFData dfData;

    // The floor is always assumed to be static, so no
    // need to perform any physics processing at all...

    dfData.dist = coord.y - planeElev;
    dfData.rgbaColor = float4(1, 1, 1, 1);
    dfData.id = 0;
    return dfData;
}

// Cube DF here
// Returns the distance to the surface of the cube
// (with the given width, height, and depth) from the given point
// Core distance function found within:
// http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
DFData CubeDF(float3 coord,
              Figure fig,
              uint callIndex)
{
    // Transformations (except scale) applied here
    float3 shiftedCoord = coord - fig.pos.xyz;
    float3 orientedCoord = QtnRotate(shiftedCoord, fig.rotationQtn);
    float3 freshCoord = orientedCoord;

    // Scale applied here
    float edgeLength = fig.scaleFactor.x;
    float3 edgeDistVec = abs(freshCoord) - float3(edgeLength,
                                                  edgeLength,
                                                  edgeLength);

    // Function properties generated/extracted and stored here
    DFData dfData;
    dfData.dist = min(max(edgeDistVec.x, max(edgeDistVec.y, edgeDistVec.z)), 0.0f) + length(max(edgeDistVec, float3(0, 0, 0)));
    dfData.rgbaColor = fig.surfRGBA;
    dfData.id = callIndex;
    return dfData;
}

// Sphere DF here
// Returns the distance to the surface of the sphere
// (with the given radius) from the given point
// Core distance function modified from the original found within:
// http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions
DFData SphereDF(float3 coord,
                Figure fig,
                uint callIndex)
{
    float3 relCoord = coord - fig.pos.xyz; //mul(fig.rotationQtn, float4(coord, 1)).xyz - fig.pos.xyz;
    DFData dfData;
    dfData.dist = length(relCoord) - fig.scaleFactor.x;
    dfData.rgbaColor = fig.surfRGBA;
    dfData.id = callIndex;
    return dfData;
}

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
        Figure currFig = figuresReadable[i];
        if (currFig.dfType.x == sphereDFID)
        {
            dfArr[i] = SphereDF(coord, currFig, i);
        }
        else if (currFig.dfType.x == cubeDFID)
        {
            dfArr[i] = CubeDF(coord, currFig, i);
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

// Utility function designed to match rays emitted by the camera
// to the shape of the view frustum (stolen from the shader written
// by the tute author as a code guide)
float3 RayDir(float fovRads, float2 viewSizes, uint2 pixID)
{
    float2 xy = pixID - (viewSizes / 2.0);
    float z = viewSizes.y / tan(fovRads / 2.0);
    return normalize(float3(xy, z));
}

// Extract the normal to a given point from the local scene
// gradient
float3 GetNormal(float3 samplePoint, float rayEpsilon)
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

float3 LightingAtAPoint(float3 samplePoint, float rayEpsilon, float3 sampleRGB,
                        float shadowStrength)
{
    // Initialise lighting properties
    float3 lightPos = float3(0.0f, 5.0f, -5.0f);
    float3 outputColor = sampleRGB;

    // Fetch the local scene normal
    float3 norm = GetNormal(samplePoint, rayEpsilon);

    // We know the point illuminated by the current ray is lit, so light it
    float pi = 3.14159;
    float3 reflectVec = (lightPos - samplePoint);

    // Correct sunken rays by boosting every given ray slightly past the surface
    // of the scene with regards to the local gradient
    // (see: http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#Result___Shadow_acne
    // for why)
    float nDotL = dot(norm, normalize(reflectVec));
    float bias = clamp(0.005 * tan(acos(nDotL)), 0, 0.01);
    samplePoint = samplePoint + (norm * bias);

    float distToLight = length(reflectVec);
    float albedo = 0.18f;
    float3 invSquare = ((400 * albedo) / (4 * pi * (distToLight * distToLight)));
    float3 lightRGB = float3(1.0f, 1.0f, 1.0f);

    // Formula for directional light:
    // saturate(sampleRGB * ((albedo / pi) * intensity * lightRGB * nDotL));

    // Formula for point light:
    // let invSquare = ((intensity * albedo) / (4 * pi * (distToLight * distToLight)));
    // then light = saturate(sampleRGB * (invSquare * lightRGB * nDotL));
    outputColor = saturate(sampleRGB * ((albedo / pi) * 10 * lightRGB * nDotL)); //saturate(sampleRGB * (invSquare * lightRGB * nDotL));

    // March the current ray back to the light source; if it intersects with any
    // geometry, darken the current color appropriately
    // Shadow-tracing algorithm is from Inigo Quilez, i.e. the master of ray-marched DFs;
    // you can find it over here: http://www.iquilezles.org/www/articles/rmshadows/rmshadows.htm
    float3 rayDir = normalize(lightPos - samplePoint);
    float currRayDist = rayEpsilon;

    float brightness = 1.0;
    for (uint i = 0; i < MAX_OCC_MARCHER_STEPS; i += 1)
    {
        float3 pathVec = samplePoint + (rayDir * currRayDist);
        float distToScene = SceneField(pathVec).dist;
        if (distToScene < rayEpsilon)
        {
            brightness = 0.0f;
            break;
        }
        else
        {
            brightness = min(brightness, shadowStrength * distToScene / currRayDist);
            currRayDist += distToScene;
        }
    }

    return outputColor * brightness;
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

[numthreads(1, 1, 1)]
void main(uint3 groupID : SV_GroupID,
          uint3 threadID : SV_GroupThreadID)
{
    // The pixel ID (x/y coordinates) associated with the current
    // thread group
    uint2 pixID = groupID.xy;

    // A magical error term, used because close rays might intersect
    // each other during shadowing/reflection/refraction or "skip" through
    // the scene surface because of floating-point error; this allows us to
    // specify a range of valid values between +rayEpsilon and -rayEpsilon
    // for those situations :)
    float rayEpsilon = 0.00001f;

    float maxRayDist = 64.0f;
    float currRayDist = 0.0f;

    float3 eyePos = cameraPos.xyz;
    float3 rayDir = RayDir(3.14f / 2.0f, float2(DISPLAY_WIDTH, DISPLAY_HEIGHT), pixID);
    for (uint i = 0; i < MAX_VIS_MARCHER_STEPS; i += 1)
    {
        float3 rayVec = mul(float4(currRayDist * rayDir, 1), viewMat).xyz;
        DFData sceneField = SceneField(eyePos + rayVec);
        if (sceneField.dist < rayEpsilon)
        {
            // Use path tracing to illuminate the current figure
            float3 localRGB = LightingAtAPoint(eyePos + rayVec, rayEpsilon,
                                               sceneField.rgbaColor.rgb, 64.0f);

            // Write the illuminated color to the display texture
            displayTex[pixID] = float4(localRGB, sceneField.rgbaColor.a);
            break;
        }

        currRayDist += sceneField.dist;

        if (currRayDist > (maxRayDist - rayEpsilon))
        {
            // Assume the ray has passed through the scene without touching anything;
            // write a transparent color to the scene texture before breaking out
            displayTex[pixID] = float4(sin(groupID.x), tan(groupID.y), 0.0f, 1.0f);
            break;
        }
    }

    if (groupID.x < numFigures.x &&
        groupID.y == 0 &&
        groupID.z == 0)
    {
        FigUpdates(groupID.x);
    }
}