#define MAX_POINT_LIGHT_COUNT 16
#define MAX_SPOT_LIGHT_COUNT 16

// NOTE
// As far as I know, the lighting calculations below are valid (touch wood) for
// directional, point, and spot lights using PBR for diffuse (Oren-Nayar) and
// specular (Cook-Torrance) shading; however, all the input light data below
// [dirPos] is lost when the [cbuffer] is initialized (no, I don't understand
// it either), which means that the functions evaluating point and spot
// lighting never actually run. I'll continue working on this, and I'll update
// you + demonstrate the post-patch code if I manage to find a solution :)

// Mark [texIn] as a resource bound to the zeroth texture register ([register(t0)])
Texture2D texIn : register(t0);

// Mark [wrapSampler] as a resource bound to the zeroth sampler register ([register(s0)])
SamplerState wrapSampler : register(s0);

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
    uint4 numPointLights;

    float4 spotIntensity[MAX_SPOT_LIGHT_COUNT];
    float4 spotDiffuse[MAX_SPOT_LIGHT_COUNT];
    float4 spotPos[MAX_SPOT_LIGHT_COUNT];
    float4 spotDirection[MAX_SPOT_LIGHT_COUNT];
    float4 spotCutoffRadians;
    uint4 numSpotLights;

    matrix worldMat;
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

float CookTorrance(float pixGrain, float pixReflectFactor, float4 pixNorm,
                   float4 surfaceViewVector, float4 surfaceLightVector)
{
    // Calculate different parts of the beckmann distribution
    float grainSqr = pixGrain * pixGrain;
    float4 halfVec = normalize(surfaceLightVector + surfaceViewVector);
    float normDotHalf = clamp(dot(pixNorm, halfVec), 0, 1);
    float normDotHalfSqr = normDotHalf * normDotHalf;
    float normDotHalfCube = normDotHalfSqr * normDotHalfSqr;

    // Calculate the alpha term + store the constant [e] for easy reference
    float alpha = (-1 * (1 - normDotHalfSqr)) / ((normDotHalfSqr) * grainSqr);
    float e = 2.71828182845904523536;

    // Express the distribution in functional form and store it's value for
    // the given roughness, surface-to-view vector, and surface-to-light vector
    float beckmann = pow(e, alpha) / (grainSqr * normDotHalfCube);

    // Calculate an approximate fresnel term with the given reflection factor
    // Fresnel terms are functions describing the behaviour of light at
    // various angles as it interacts with various materials
    // We fake it via Schlick's Approximation, since most materials (for now)
    // are opaque and we can get away with a generic reflection factor rather
    // than computing reflection + refraction
    // To /really/ get Schlick's Approximation you'd need to thoroughly
    // understand specular reflection, and there isn't really enough space to
    // explain that here; that said, there's quite a nice summary of the math
    // + physics behind it available over on Wikipedia:
    // https://en.wikipedia.org/wiki/Specular_reflection
    float normDotView = dot(pixNorm, surfaceViewVector);
    float approxFresLHS = pixReflectFactor + (1 - pixReflectFactor);
    float approxFresRHS = pow(1 - normDotView, 5);
    float approxFres = approxFresLHS * approxFresRHS;

    // Calculate the geometric attenuation factor
    // The GAF is used to simulate shadowing between the microfacets on the
    // surface being lit, which ultimately reduces the amount of light
    // reflected back to the viewer
    // Microfacets are very relevant to specular lighting as well as
    // diffuse + specular PBR lighting techniques; you can read more
    // about them over here:
    // https://en.wikipedia.org/wiki/Specular_highlight#Microfacets
    //
    // And there's a definition of the GAF in maths notation available over
    // here:
    // http://groups.csail.mit.edu/graphics/classes/6.837/F00/Lecture17/Slide11.html

    // Calculate relevant dot-products
    float halfDotNorm = dot(halfVec, pixNorm);
    float surfaceViewDotNorm = dot(surfaceViewVector, pixNorm);
    float surfaceLightDotNorm = dot(surfaceLightVector, pixNorm);
    float surfaceViewDotHalf = dot(surfaceViewVector, halfVec);

    // Calculate approximate values for the two fractions used to define the GAF
    float gafFracA = (2 * halfDotNorm * surfaceViewDotNorm) / surfaceViewDotHalf;
    float gafFracB = (2 * halfDotNorm * surfaceLightDotNorm) / surfaceViewDotHalf;

    // Express the GAF in functional form and store it's value for the given
    // surface-to-light vector, surface-to-view vector, and surface normal
    // Nested [min(...)] calls because three-float [min(...)] is unsupported
    // in HLSL
    float geoAttenuationFactor = min(1, min(gafFracA, gafFracB));

    // Aggregate all the relevant values into the actual Cook-Torrance
    // function, then return the result to the calling location :)
    float pi = 3.14159265358979323846;
    return (beckmann * approxFres * geoAttenuationFactor) / (pi * surfaceViewDotNorm);
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
    float cookTorranceModifier = CookTorrance(pixIn.grain, pixIn.reflectFactor, pixIn.normal, pixIn.surfaceView,
                                              lightDirection);

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

float4 DirLightColorCumulative(Pixel pixIn)
{
    float4 internalLightColor = float4(0, 0, 0, 0);
    internalLightColor = Lighting(pixIn, dirDiffuse, dirIntensity[0], dirDirection);
    return saturate(internalLightColor);
}

float4 PointLightColorCumulative(Pixel pixIn)
{
    float4 internalLightColor = float4(0, 0, 0, 0);

    for (uint i = 0; i != numPointLights[0]; i += 1)
    {
        internalLightColor += Lighting(pixIn, pointDiffuse[i], pointIntensity[i][0], (mul(pointPos[i], worldMat) - pixIn.vertWorldPos));
    }

    return saturate(internalLightColor);
}

float4 SpotLightColorCumulative(Pixel pixIn)
{
    float4 internalLightColor = float4(0, 0, 0, 0);

    for (uint j = 0; j != numSpotLights[0]; j += 1)
    {
        // Skip any spot lights where the angle between [spotDirection] and the
        // light vector itself (defined as acos(dot(normalize(spotDirection), normalize(spotPosition))))
        // is greater than the cuttoff angle [spotRadius]
        float angleVertCos = dot(normalize(spotDirection[j]), pixIn.vertWorldPos - mul(spotPos[j], worldMat));
        float angleToVert = acos(angleVertCos);
        bool isInsideCutoff = (angleToVert < spotCutoffRadians[0]);

        if (isInsideCutoff)
        {
            internalLightColor += Lighting(pixIn, pointDiffuse[j], pointIntensity[j][0], (mul(pointPos[j], worldMat) - pixIn.vertWorldPos));
        }
    }

    return saturate(internalLightColor);
}

float4 main(Pixel pixIn) : SV_TARGET
{
    // Generate base color
    float4 pixOut = pixIn.color * texIn.Sample(wrapSampler, pixIn.texCoord);

    // Initialise the average light color
    // with the given ambient tinting
    float4 lightColor = dirAmbient;

    // Generate mixed diffuse/specular colors from all scene lights, then
    // describe the cumulative effect of those lights on the current pixel by
    // adding the sum of the diffuse/specular colors into [lightColor]
    lightColor += (DirLightColorCumulative(pixIn) + PointLightColorCumulative(pixIn) + SpotLightColorCumulative(pixIn));

    // Return final color as a (clamped with [saturate]) multiplicative blend
    // between the raw material color ([pixOut]) and the average light color
    // ([lightColor])
    return saturate(pixOut * lightColor);
}