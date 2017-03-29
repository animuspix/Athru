cbuffer MatBuffer
{
    matrix world;
    matrix view;
    matrix projection;
};

struct Vertex
{
    float4 pos : POSITION0;
    float4 color : COLOR0;
    float4 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float grain : COLOR1;
    float reflectFactor : COLOR2;
};

struct Pixel
{
    float4 pos : SV_POSITION0;
    float4 color : COLOR0;
    float4 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float grain : COLOR1;
    float reflectFactor : COLOR2;
};

struct AvgLightInfluence
{
    float4 avgPosition;
    float4 avgDiffuse;
    float avgIntensity;
};

AvgLightInfluence GetPointInfluence(float4 positions[MAX_POINT_LIGHT_COUNT], float4 diffuseColors[MAX_POINT_LIGHT_COUNT],
                                    float intensities[MAX_POINT_LIGHT_COUNT], uint numLights,
                                    float4 vertWorldPos, float4 surfaceViewVec)
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
    for (uint i = 0; i < numLights; i += 1)
    {
        // Light properties are proximity-weighted so that
        // close lights have more emphasis than far ones
        avgPosition += pointPos[i] * (1 / length(surfaceViewVec) * 10);
        avgDiffuse += pointDiffuse[i] * (1 / length(surfaceViewVec) * 10);
        avgIntensity += pointIntensity[i] * (1 / length(surfaceViewVec) * 10);
    }

    // Retrieve mean from summed positions
    avgPosition /= numLights;

    // Retrieve mean from summed diffuse colors
    avgDiffuse /= numLights;

    // Retrieve mean from summed intensities
    avgIntensity /= numLights;

    // Store the generated averages within [lightInfluence]
    lightInfluence.avgPosition = avgPosition;
    lightInfluence.avgDiffuse = avgDiffuse;
    lightInfluence.avgIntensity = avgIntensity;

    // Return the generated averages
    return lightInfluence;
}

AvgLightInfluence GetSpotInfluence(float4 positions[MAX_SPOT_LIGHT_COUNT], float4 diffuseColors[MAX_SPOT_LIGHT_COUNT],
                                   float intensities[MAX_SPOT_LIGHT_COUNT], uint numLights, float4 vertWorldPos,
                                   float4 surfaceViewVec)
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

    for (uint i = 0; i < numLights; i += 1)
    {
        // Calculate the vector from the current spot light to [vertWorldPos]
        lightVector = normalize(vertWorldPos - positions[i]);

        // Skip any spot lights where the angle between [spotDirection] and the
        // light vector itself (defined as acos(dot(normalize(spotDirection), normalize(spotPosition))))
        // is greater than the cuttoff angle [spotRadius]
        float angleVertCos = dot(normalize(spotDirection[i]), lightVector);
        float angleToVert = acos(angleVertCos);
        bool isInsideCutoff = (angleToVert < spotCutoffRadians);

        // Light properties proximity-weighted so that
        // close lights have more emphasis than far ones
        avgPosition += spotPos[i] * (1 / length(surfaceViewVec) * 10) * isInsideCutoff; //float4(isInsideCutoff, isInsideCutoff, isInsideCutoff, isInsideCutoff);
        avgIntensity += spotIntensity[i] * (1 / length(surfaceViewVec) * 10) * isInsideCutoff;
        avgDiffuse += spotDiffuse[i] * (1 / length(surfaceViewVec) * 10) * isInsideCutoff;
    }

    // Retrieve mean from summed positions
    avgPosition /= numLights;

    // Retrieve mean from summed diffuse colors
    avgDiffuse /= numLights;

    // Retrieve mean from summed intensities
    avgIntensity /= numLights;

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

    // Generate a vector from the surface to the camera view
    float4 surfaceToView = output.normal - viewVec;

    // Store a normalized copy of the directional light direction
    float4 normDirDirection = normalize(dirDirection);

    // Calculate the average pixel influence of the visible point lights
    AvgLightInfluence pointLightInfluence = GetPointInfluence(pointPos, pointDiffuse, pointIntensity, numPointLights,
                                                              mul(vertIn.pos, world), surfaceToView);

    // Calculate the average pixel influence of the visible spot lights
    AvgLightInfluence spotLightInfluence = GetSpotInfluence(pointPos, pointDiffuse, pointIntensity, numSpotLights,
                                                            mul(vertIn.pos, world), surfaceToView);

    // Calculate the normalized mean of the directional light direction, the average point direction, and the
    // average spot direction
    float4 avgLightDirection = normalize((normDirDirection/* + pointLightInfluence.avgPosition*/ + spotLightInfluence.avgPosition) / 2);

    // Calculate the mean of the directional light intensity, the average point intensity, and the
    // average spot intensity
    // Weighting intensities by position will have knocked a few components out of the [0...1] range, so fix that by
    // clamping them with [saturate]
    float avgLightIntensity = (dirIntensity /*+ saturate(pointLightInfluence.avgIntensity)*/ + saturate(spotLightInfluence.avgIntensity)) / 2;

    // Calculate the clamped mean of the directional light diffuse color, the average point diffuse color, and the
    // average spot diffuse color
    // Weighting colors by position will have knocked a few components out of the [0...1] range, so fix that by
    // clamping them with [saturate]
    // Clamping instead of normalization because we don't want to lose magnitude like we do with direction, but we
    // do want to guarantee that none of our lights will have diffuse components outside the expected [0...1] range
    float4 avgLightDiffuse = saturate((dirDiffuse /*+ saturate(pointLightInfluence.avgDiffuse)*/ + saturate(spotLightInfluence.avgDiffuse)) / 2);

    // Pass generated lighting data into the output [Pixel]
    output.avgLightDirection = avgLightDirection;
    output.avgLightIntensity = 1.0f; //float4(avgLightIntensity, avgLightIntensity, avgLightIntensity, avgLightIntensity);
    output.avgLightDiffuse = avgLightDiffuse;
    output.ambientLighting = dirAmbient;
    output.surfaceToView = normalize(surfaceToView);

    // Pass the output [Pixel] along to
    // the pixel shader
    return output;
}