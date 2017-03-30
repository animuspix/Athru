#define MAX_POINT_LIGHT_COUNT 10
#define MAX_SPOT_LIGHT_COUNT 10

// Mark [texIn] as a resource bound to the zeroth texture register ([register(t0)])
Texture2D texIn : register(t0);

// Mark [wrapSampler] as a resource bound to the zeroth sampler register ([register(s0)])
SamplerState wrapSampler : register(s0);

// Buffer for lighting data (only one directional light per scene atm, though that
// could be extended)
cbuffer LightBuffer
{
    float dirIntensity;
    float3 dirIntensityPadding;

    float4 dirDirection;
    float4 dirDiffuse;
    float4 dirAmbient;
    float4 dirPos;

    float pointIntensity[MAX_POINT_LIGHT_COUNT];
    float3 pointIntensityPadding[MAX_POINT_LIGHT_COUNT];

    float4 pointDiffuse[MAX_POINT_LIGHT_COUNT];
    float4 pointPos[MAX_POINT_LIGHT_COUNT];

    uint numPointLights;
    uint3 numPointLightPadding;

    float spotIntensity[MAX_SPOT_LIGHT_COUNT];
    float spotIntensityPadding[MAX_SPOT_LIGHT_COUNT];

    float4 spotDiffuse[MAX_SPOT_LIGHT_COUNT];
    float4 spotPos[MAX_SPOT_LIGHT_COUNT];
    float4 spotDirection[MAX_SPOT_LIGHT_COUNT];

    float spotCutoffRadians;
    float3 spotCutoffRadiansPadding;

    uint numSpotLights;
    uint3 numSpotLightPadding;
};

struct Pixel
{
    float4 pos : SV_POSITION0;
    float4 color : COLOR0;
    float4 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float grain : COLOR1;
    float reflectFactor : COLOR2;
    float4 surfaceView : NORMAL1;
    float4 vertWorldPos : NORMAL2;
};

// Output the result of Oren-Nayar processing for the given roughness, pixel normal,
// normalized average light direction, and lambert term (dot product of the normal and
// the normalized average light direction after clamping between [1] and [0]; didn't
// really want to calculate it twice (first time was to get pre-Oren-Nayar pixel
// exposure) so passing it in here instead)
float OrenNayar(float pixGrain, float4 pixNorm, float4 surfaceViewVector, float4 surfaceLightVector, float lambert)
{
    // Calculate the [A] term for the Oren-Nayar reflectance model
    float grainSqr = pixGrain * pixGrain;
    float orenNayarA = 1 - (0.5 * (grainSqr / (grainSqr + 0.57f)));

    // Calculate the [B] term for the Oren-Nayar reflectance model
    float orenNayarB = 0.45 * (grainSqr / (grainSqr + 0.09f));

    // Calculate the amount of light backscattered towards the camera
    float4 cameraBackscatter = surfaceViewVector - (pixNorm * dot(surfaceViewVector, pixNorm));

    // Calculate the amount of light backscatterered towards the overall light
    // source
    float4 sourceBackscatter = surfaceLightVector - (pixNorm * dot(surfaceLightVector, pixNorm));

    // Calculate the amount of light radiated directly from the current pixel
    // after backscattering
    float gamma = dot(cameraBackscatter, sourceBackscatter);

    // Cache properties used to calculate the [alpha] and [beta] terms in
    // the Oren-Nayar reflectance model
    float cameraTheta = acos(dot(pixNorm, surfaceViewVector));
    float lightTheta = acos(lambert);

    // Calculate the [alpha] term for the Oren-Nayar reflectance model
    float alpha = max(cameraTheta, lightTheta);

    // Calculate the [beta] term for the Oren-Nayar reflectance model
    float beta = min(cameraTheta, lightTheta);

    // Aggregate all the relevant values into the actual Oren-Nayar
    // function, then return the result to the calling location :)
    return (orenNayarA + orenNayarB * max(0.0f, gamma) * (sin(alpha) * tan(beta)));
}

float CookTorrance()
{
    // Aggregate all the relevant values into the actual Cook-Torrance
    // function, then return the result to the calling location :)
    return 0;
}

float4 Lighting(Pixel pixIn, float4 lightDiffuse, float lightIntensity, float4 lightDirection)
{
    // Calculate pixel exposure with the overall light direction
    float lambertTerm = saturate(dot(pixIn.normal, lightDirection));
    float currPixelExposure = lambertTerm * lightIntensity;

    // Diffuse lighting

    // Cache the result of the Oren-Nayar function in a specific modifier for easy access
    // when we compute the output lighting equation :)
    float orenNayarModifier = OrenNayar(pixIn.grain, pixIn.normal, pixIn.surfaceView, 
                                        lightDirection, lambertTerm);

    // Specular lighting

    // Cache the result of the Cook-Torrance function in a specific modifier
    // that we can combine with the Oren-Nayar modifier above to generate
    // a mixed diffuse/specular PBR value that we can apply to the raw
    // pixel exposure calculated earlier
    float cookTorranceModifier = CookTorrance();

    // Combine diffuse/specular lighting

    // Sum the Cook-Torrance modifier with the Oren-Nayar modifier, then apply the result
    // to the exposure for the current pixel
    currPixelExposure *= (orenNayarModifier + cookTorranceModifier);

    // Use the generated exposure value to modify the diffuse light color applied to the
    // current [Pixel]
    // ...unless the [Pixel] exposure is less than zero; don't do anything in that case,
    // negative lighting would be weird :P

    // If the current [Pixel] is facing towards the light source, add the
    // multiplicative blend of the diffuse color and the exposure level
    // into the output color; afterward, guarantee that each component of
    // the result is between [0] and [1] with the [saturate] function
    return (lightDiffuse * currPixelExposure) * (currPixelExposure > 0.0f);
}

float4 DirLightColorCumulative(Pixel pixIn, float4 localLightColor)
{
    localLightColor = Lighting(pixIn, dirDiffuse, dirIntensity, dirDirection);
    return saturate(localLightColor);
}

float4 PointLightColorCumulative(Pixel pixIn, float4 localLightColor)
{
    for (uint i = 0; i < numPointLights; i += 1)
    {
        localLightColor += Lighting(pixIn, pointDiffuse[i], pointIntensity[i], normalize(pixIn.vertWorldPos - spotPos[i]));
    }

    return saturate(localLightColor);
}

float4 SpotLightColorCumulative(Pixel pixIn, float4 localLightColor)
{
    for (uint i = 0; i < numSpotLights; i += 1)
    {
        // Skip any spot lights where the angle between [spotDirection] and the
        // light vector itself (defined as acos(dot(normalize(spotDirection), normalize(spotPosition))))
        // is greater than the cuttoff angle [spotRadius]
        float angleVertCos = dot(normalize(spotDirection[i]), normalize(pixIn.vertWorldPos - spotPos[i]));
        float angleToVert = acos(angleVertCos);
        bool isInsideCutoff = (angleToVert < spotCutoffRadians);

        if (isInsideCutoff)
        {
            localLightColor += Lighting(pixIn, pointDiffuse[i], pointIntensity[i], spotPos[i]);
        }
    }

    return saturate(localLightColor);
}

float4 main(Pixel pixIn) : SV_TARGET
{
    // Generate base color
    float4 pixOut = pixIn.color; //* texIn.Sample(wrapSampler, pixIn.texCoord);

    // Initialise the average light color
    // with the given ambient tinting
    float4 lightColor = dirAmbient;

    // Generate mixed diffuse/specular colors from all scene lights, then
    // describe the cumulative effect of those lights on the current pixel by
    // adding the sum of the diffuse/specular colors into [lightColor]
    lightColor += (DirLightColorCumulative(pixIn, lightColor)/* + PointLightColorCumulative(pixIn, lightColor)*/ + SpotLightColorCumulative(pixIn, lightColor));

    // Return final color as a (clamped with [saturate]) multiplicative blend
    // between the raw material color ([pixOut]) and the average light color
    // ([lightColor])
    return saturate(pixOut * lightColor);
}