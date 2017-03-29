// Mark [texIn] as a resource bound to the zeroth texture register ([register(t0)])
Texture2D texIn : register(t0);

// Mark [wrapSampler] as a resource bound to the zeroth sampler register ([register(s0)])
SamplerState wrapSampler : register(s0);

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

// Output the result of Oren-Nayar processing for the given roughness, pixel normal,
// normalized average light direction, and lambert term (dot product of the normal and
// the normalized average light direction after clamping between [1] and [0]; didn't
// really want to calculate it twice (first time was to get pre-Oren-Nayar pixel
// exposure) so passing it in here instead)
float4 OrenNayar(float4 pixGrain, float4 pixNorm, float4 avgLightDirectionNorm, float4 lambert)
{
    // Calculate the [A] term for the Oren-Nayar reflectance model
    float grainSqr = pixGrain * pixGrain;
    float orenNayarA = 1 - (0.5 * (grainSqr / (grainSqr + 0.57f)));

    // Calculate the [B] term for the Oren-Nayar reflectance model
    float orenNayarB = 0.45 * (grainSqr / (grainSqr + 0.09f));

    // Generate a vector from the surface to the camera view
    float4 surfaceViewVector = normalize(pixNorm - viewVec);

    // Calculate the amount of light backscattered towards the camera
    float4 cameraBackscatter = surfaceViewVector - (pixNorm * dot(surfaceViewVector, pixNorm));

    // Calculate the amount of light backscatterered towards the overall light
    // source
    float4 sourceBackscatter = avgLightDirectionNorm - (pixNorm * dot(avgLightDirectionNorm, pixNorm));

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

float4 CookTorrance()
{
    // Aggregate all the relevant values into the actual Cook-Torrance
    // function, then return the result to the calling location :)
    return 0;
}

float4 main(Pixel pixIn) : SV_TARGET
{
    // Generate base color...
    float4 pixOut = pixIn.color * texIn.Sample(wrapSampler, pixIn.texCoord);

    // Initialise the average light color
    // with the given ambient tinting
    float4 lightColor = pixIn.ambientLighting;

    // Calculate pixel exposure with the overall light direction
    float lambertTerm = saturate(dot(pixIn.normal, pixIn.avgLightDirection));
    float currPixelExposure = lambertTerm * pixIn.avgLightIntensity;

    // Diffuse lighting

    // Cache the result of the Oren-Nayar function in a specific modifier for easy access
    // when we compute the output lighting equation :)
    float4 orenNayarModifier = OrenNayar(pixIn.grain, pixIn.normal, pixIn.avgLightDirection, lambertTerm);

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
    lightColor += (pixIn.avgLightDiffuse * currPixelExposure) * (currPixelExposure > 0.0f);
    lightColor = saturate(lightColor);

    // Complete the lighting equation by applying the generated diffuse light color to the
    // output pixel defined above

    // Return final color as a (clamped with [saturate]) multiplicative blend
    // between the raw material color ([pixOut]) and the average light color
    // ([lightColor])
    return saturate(pixOut * lightColor);
}