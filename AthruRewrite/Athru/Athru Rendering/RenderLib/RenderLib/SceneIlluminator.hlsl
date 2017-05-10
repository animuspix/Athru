
// Dispatched after initial scene processing; illuminates the
// scene based on starting emissivity values

// Scene globals defined here

// Scene color texture buffer/access view
RWStructuredBuffer<float4> colorTexBuf : register(u0);

// Scene normals texture buffer
StructuredBuffer<float4> normFieldBuf : register(u1);

// Scene PBR texture buffer
StructuredBuffer<float2> pbrTexBuf : register(u2);

// Scene emissivity texture buffer
StructuredBuffer<float> emissTexBuf : register(u3);

// The camera's local view vector
// Not 100% sure we actually need photoreal lighting;
// very expensive for (potentially) aesthetically
// awkward results
StructuredBuffer<float4> cameraViewVec : register(u4);

// Calculate Oren-Nayar PBR diffuse lighting
float OrenNayar(float pixGrain, float4 pixNorm, float4 surfaceViewVector, float4 surfaceLightVector, float lambert)
{
    // Calculate the [A] term for the Oren-Nayar reflectance model
    float grainSqr = pixGrain * pixGrain;
    grainSqr = max(grainSqr, 0.001f);
    float orenNayarA = 1.0f - (0.5f * (grainSqr / (grainSqr + 0.57f)));

    // Calculate the [B] term for the Oren-Nayar reflectance model
    float orenNayarB = 0.45 * (grainSqr / (grainSqr + 0.09f));

    // Calculate the amount of light backscattered towards the camera
    float4 cameraBackscatter = normalize(surfaceViewVector - (pixNorm * dot(pixNorm, surfaceViewVector)));

    // Calculate the amount of light backscatterered towards the overall light
    // source
    float4 sourceBackscatter = normalize(surfaceLightVector - (pixNorm * dot(surfaceLightVector, pixNorm)));

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
    return (orenNayarA + (orenNayarB * max(0.0f, gamma) * (sin(alpha) * tan(beta))));
}

// Calculate Cook-Torrance PBR reflectance
float CookTorrance(float pixGrain, float pixReflectFactor, float4 pixNorm,
                   float4 surfaceViewVector, float4 surfaceLightVector)
{
    // Calculate different parts of the beckmann distribution
    float grainSqr = pixGrain * pixGrain;
    grainSqr = max(grainSqr, 0.001f);
    float4 halfVec = normalize(surfaceLightVector + surfaceViewVector);
    float normDotHalf = clamp(dot(pixNorm, halfVec), 0, 1);
    float normDotHalfSqr = normDotHalf * normDotHalf;
    float normDotHalfCube = normDotHalfSqr * normDotHalfSqr;

    // Calculate the alpha term + store the constant [e] for easy reference
    float alpha = (-1 * (1 - normDotHalfSqr)) / ((normDotHalfSqr) * grainSqr);
    float e = 2.71828182845904523536f;

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
    float approxFresLHS = pixReflectFactor;
    float approxFresRHS = (1 - pixReflectFactor) * pow(1 - normDotView, 5);
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
    float pi = 3.14159265358979323846f;
    return (beckmann * approxFres * geoAttenuationFactor) / (pi * surfaceViewDotNorm); //* dot(pixNorm, surfaceLightVector);
}

// Calculate lighting at the current UVW coordinate
float4 Lighting()
{
    // Extract local face normals from the given vertex normal
    float4 localXNorm = normalize(mul(float4(pixIn.normal.x, 0, 0, 1), worldMat));
    float4 localYNorm = normalize(mul(float4(0, pixIn.normal.y, 0, 1), worldMat));
    float4 localZNorm = normalize(mul(float4(0, 0, pixIn.normal.z, 1), worldMat));

    float lambertX = dot(localXNorm, lightDirection);
    float lambertY = dot(localYNorm, lightDirection);
    float lambertZ = dot(localZNorm, lightDirection);
    float avgLambert = (lambertX + lambertY + lambertZ) / 3;

    // Calculate pixel exposure with the overall light direction
    float lambertTerm = saturate(avgLambert);
    float currPixelExposure = lambertTerm * lightIntensity;

    // Diffuse lighting

    // Cache the result of the Oren-Nayar function in a specific modifier for easy access
    // when we compute the output lighting equation :)
    float orenNayarModifier = OrenNayar(pixIn.grain, pixIn.normal, pixIn.surfaceView,
                                        lightDirection, lambertTerm);

    // Specular lighting

    // Cache the result of the Cook-Torrance function in a specific modifier
    // that we can combine with the Oren-Nayar modifier above to generate
    // a mixed diffuse/specular PBR value that we can apply to the rawq
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

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    Lighting();
}