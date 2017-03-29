#define MAX_POINT_LIGHT_COUNT 10
#define MAX_SPOT_LIGHT_COUNT 10

cbuffer MatBuffer
{
    matrix world;
    matrix view;
    matrix projection;
};

// Buffer for lighting data (only one directional light per scene atm, though that
// could be extended)
cbuffer LightBuffer
{
    float4 dirIntensity;
    float4 dirDirection;
    float4 dirDiffuse;
    float4 dirAmbient;
    float4 dirPos;
    float4 pointIntensity[MAX_POINT_LIGHT_COUNT];
    float4 pointDiffuse[MAX_POINT_LIGHT_COUNT];
    float4 pointPos[MAX_POINT_LIGHT_COUNT];
    int numPointLights;
    float3 pointPadding;
    float4 spotIntensity[MAX_SPOT_LIGHT_COUNT];
    float4 spotDiffuse[MAX_SPOT_LIGHT_COUNT];
    float4 spotPos[MAX_SPOT_LIGHT_COUNT];
    float4 spotDirection[MAX_SPOT_LIGHT_COUNT];
    float spotCutoffRadians;
    int numSpotLights;
    float2 spotPadding;
    float4 viewVec;
};

struct Vertex
{
    float4 pos : POSITION;
    float4 color : COLOR0;
    float4 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float grain : COLOR1;
    float reflectFactor : COLOR2;
};

struct Pixel
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float4 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float grain : COLOR1;
    float reflectFactor : COLOR2;
    float4 avgLightDirection : NORMAL1;
    float4 avgLightIntensity : COLOR3;
    float4 avgLightDiffuse : COLOR3;
    float4 ambientLighting : COLOR4;
};

struct AvgLightInfluence
{
    float4 avgPosition = float4(0, 0, 0, 0);
    float4 avgDiffuse = float4(0, 0, 0, 0);
    float avgIntensity = 0;
};

AvgLightInfluence GetPointInfluence(float4 positions[], float4 diffuseColors[], float intensities[], int numLights, float4 vertWorldPos)
{
    // Return mean values within an [AvgLightInfluence] object
    AvgLightInfluence lightInfluence;

    // Initialise average direction, diffuse color, and
    // intensity to zero
    float4 zeroVec = float4(0, 0, 0, 0);
    float4 avgPosition = zeroVec;
    float4 avgDiffuse = zeroVec;
    float avgIntensity = 0;

    // Store normalized averages of the positions, intensities, and
    // diffuse color of each visible point light
    for (int i = 0; i < numLights; i += 1)
    {
        avgPosition += pointPos[i];

        // Intensity and diffuse color are proximity-weighted so that
        // close lights have more emphasis than far ones
        avgDiffuse += pointDiffuse[i] * length(pointPos[i]);
        avgIntensity += pointIntensity[i][0] * length(pointPos[i]);
    }

    // Retrieve mean from summed positions
    avgPosition /= numPointLights;
    avgPosition = normalize(avgPosition);

    // Retrieve mean from summed diffuse colors
    avgDiffuse /= numPointLights;
    avgDiffuse = normalize(avgDiffuse);

    // Retrieve mean from summed intensities
    avgIntensity /= numPointLights;

    // Store the generated averages within [lightInfluence]
    lightInfluence.avgPosition = avgPosition;
    lightInfluence.avgDiffuse = avgDiffuse;
    lightInfluence.avgIntensity = avgIntensity;

    // Return the generated averages
    return lightInfluence;
}

AvgLightInfluence GetSpotInfluence(float4 positions[], float4 diffuseColors[], float intensities[], int numLights, float4 vertWorldPos)
{
    // Return mean values within an [AvgLightInfluence] object
    AvgLightInfluence lightInfluence;

    // Initialise average direction, diffuse color, and
    // intensity to zero
    float4 zeroVec = float4(0, 0, 0, 0);
    float4 avgPosition = zeroVec;
    float4 avgDiffuse = zeroVec;
    float avgIntensity = 0;

    // Vector from a given spot light to the current vertex (in world coordinates)
    // Not really a [spotLight] property, just used to effectively remove spot
    // lights from the shading process if their light-cone excludes the current
    // vertex
    // Not the same as light direction; light direction defines where the light
    // emitter is pointing in general, whereas the light vector defines the path
    // directly from the emitter to a given vertex
    float4 lightVector;

    for (int i = 0; i < numSpotLights; i += 1)
    {
        // Calculate the vector from the current spot light to [vertWorldPos]
        lightVector = normalize(vertWorldPos - spotPos[i]);

        // Skip any spot lights where the angle between [spotDirection] and the
        // light vector itself (defined as acos(dot(normalize(spotDirection), normalize(spotPosition))))
        // is greater than the cuttoff angle [spotRadius]
        float angleVertCos = dot(normalize(spotDirection[i]), normalize(positions[i]));
        float angleToVert = acos(angleToVert);
        bool isInsideCutoff = (angleToVert < spotCutoffRadians);

        avgPosition += spotPos[i] * float4(isInsideCutoff, isInsideCutoff, isInsideCutoff, isInsideCutoff);

        // Intensity and diffuse color are proximity-weighted so that
        // close lights have more emphasis than far ones
        avgIntensity += spotIntensity[i][0] * length(lightVector) * isInsideCutoff;
        avgDiffuse += spotDiffuse[i] * length(lightVector) * float4(isInsideCutoff, isInsideCutoff, isInsideCutoff, isInsideCutoff);
    }

    // Retrieve mean from summed positions
    avgPosition /= numPointLights;
    avgPosition = normalize(avgPosition);

    // Retrieve mean from summed diffuse colors
    avgDiffuse /= numPointLights;
    avgDiffuse = normalize(avgDiffuse);

    // Retrieve mean from summed intensities
    avgIntensity /= numPointLights;

    // Store the generated averages within [lightInfluence]
    lightInfluence.avgPosition = avgPosition;
    lightInfluence.avgDiffuse = avgDiffuse;
    lightInfluence.avgIntensity = avgIntensity;

    // Return the generated averages
    return lightInfluence;
}

Pixel main(Vertex vertIn)
{
    Pixel output;

    // Apply the world, view, and projection
    // matrices to [vertIn]'s position so that
    // it's aligned with it's parent space and
    // sits inside the window's viewport, then
    // store the results within the output
    // [Pixel]'s position property
    output.pos = mul(vertIn.pos, world);
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, projection);

    // Mirror the input vert's color into the
    // output [Pixel]
    output.color = vertIn.color;

    // Update the input vert's normal with the
    // current world matrix, then re-normalize
    // it
    output.normal = mul(vertIn.normal, world);
    output.normal = normalize(output.normal);

    // Mirror the input vert's texture
    // coordinates into the output [Pixel]
    output.texCoord = vertIn.texCoord;

    // Mirror the input vert's grain
    // into the output [Pixel]
    output.grain = vertIn.grain;

    // Mirror the input vert's reflectiveness
    // into the output [Pixel]
    output.reflectFactor = vertIn.reflectFactor;

    // Store a normalized copy of the directional light direction
    float4 normDirDirection = normalize(dirDirection);

    // Calculate the average pixel influence of the visible point lights
    AvgLightInfluence pointLightInfluence = GetPointInfluence(pointPos, pointDiffuse, pointIntensity, numPointLights, mul(vertIn.pos, world));

    // Calculate the average pixel influence of the visible spot lights
    AvgLightInfluence spotLightInfluence = GetSpotInfluence(pointPos, pointDiffuse, pointIntensity, numPointLights, mul(vertIn.pos, world));

    // Calculate the normalized mean of the directional light direction, the average point direction, and the
    // average spot direction
    float4 avgLightDirection = normalize((normDirDirection + pointLightInfluence.avgPosition + spotLightInfluence.avgPosition) / 3);

    // Calculate the mean of the directional light intensity, the average point intensity, and the
    // average spot intensity
    // Weighting intensities by position will have knocked a few components out of the [0...1] range, so fix that by
    // clamping them with [saturate]
    float4 avgLightIntensity = (dirIntensity[0] + saturate(pointLightInfluence.avgIntensity) + saturate(spotLightInfluence.avgIntensity)) / 3;

    // Calculate the clamped mean of the directional light diffuse color, the average point diffuse color, and the
    // average spot diffuse color
    // Weighting colors by position will have knocked a few components out of the [0...1] range, so fix that by
    // clamping them with [saturate]
    // Clamping instead of normalization because we don't want to lose magnitude like we do with direction, but we
    // do want to guarantee that none of our lights will have diffuse components outside the expected [0...1] range
    float4 avgLightDiffuse = saturate((dirDiffuse + saturate(pointLightInfluence.avgDiffuse) + saturate(spotLightInfluence.avgDiffuse)) / 3);

    // Pass generated lighting data into the output [Pixel]
    output.avgLightDirection = avgLightDirection;
    output.avgLightIntensity = avgLightIntensity;
    output.avgLightDiffuse = avgLightDiffuse;
    output.ambientLighting = dirAmbient;

    // Pass the output [Pixel] along to
    // the pixel shader
    return output;
}