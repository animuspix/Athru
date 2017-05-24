
// Dispatched after initial scene processing; illuminates the
// scene based on starting emissivity values

// Scene globals defined here

// Data-blob containing the six float4s needed for
// each package of voxel normals in [normFieldBuf] (see below)
struct VoxelNormals
{
    float4 left;
    float4 right;
    float4 forward;
    float4 back;
    float4 down;
    float4 up;
};

// Data-blob containing the six floats needed for
// each package of face emissivity values in [emissTexBuf]
// (see below)
struct VoxelFaceEmissivities
{
    float left;
    float right;
    float forward;
    float back;
    float down;
    float up;
};

// Scene color texture buffer/access view
RWStructuredBuffer<float4> colorTexBuf : register(u0);

// Scene normals texture buffer
StructuredBuffer<VoxelNormals> normFieldBuf : register(t0);

// Scene PBR texture buffer
StructuredBuffer<float2> pbrTexBuf : register(t1);

// Scene emissivity texture buffer
StructuredBuffer<VoxelFaceEmissivities> emissTexBuf : register(t2);

// Per-dispatch camera/view data needed for PBR-style
// scene lighting
cbuffer CameraDataBuffer
{
    // The camera's local view vector (normalized)
    float4 cameraViewVec;
};

// Not 100% sure we actually need photoreal lighting;
// very expensive for (potentially) aesthetically
// awkward results

// Calculate Oren-Nayar PBR diffuse lighting
float OrenNayar(float voxGrain, float4 voxNorm,
                float4 surfaceViewVector, float4 surfaceLightVector, float lambert)
{
    // Calculate the [A] term for the Oren-Nayar reflectance model
    float grainSqr = voxGrain * voxGrain;
    grainSqr = max(grainSqr, 0.001f);
    float orenNayarA = 1.0f - (0.5f * (grainSqr / (grainSqr + 0.57f)));

    // Calculate the [B] term for the Oren-Nayar reflectance model
    float orenNayarB = 0.45 * (grainSqr / (grainSqr + 0.09f));

    // Calculate the amount of light backscattered towards the camera
    float4 cameraBackscatter = normalize(surfaceViewVector - (voxNorm * dot(voxNorm, surfaceViewVector)));

    // Calculate the amount of light backscatterered towards the overall light
    // source
    float4 sourceBackscatter = normalize(surfaceLightVector - (voxNorm * dot(surfaceLightVector, voxNorm)));

    // Calculate the amount of light radiated directly from the current pixel
    // after backscattering
    float gamma = dot(cameraBackscatter, sourceBackscatter);

    // Cache properties used to calculate the [alpha] and [beta] terms in
    // the Oren-Nayar reflectance model
    float cameraTheta = acos(dot(voxNorm, surfaceViewVector));
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
float CookTorrance(float voxGrain, float voxReflectFactor, float4 voxNorm,
                   float4 surfaceViewVector, float4 surfaceLightVector)
{
    // Calculate different parts of the beckmann distribution
    float grainSqr = voxGrain * voxGrain;
    grainSqr = max(grainSqr, 0.001f);
    float4 halfVec = normalize(surfaceLightVector + surfaceViewVector);
    float normDotHalf = clamp(dot(voxNorm, halfVec), 0, 1);
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
    float normDotView = dot(voxNorm, surfaceViewVector);
    float approxFresLHS = voxReflectFactor;
    float approxFresRHS = (1 - voxReflectFactor) * pow(1 - normDotView, 5);
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
    float halfDotNorm = dot(halfVec, voxNorm);
    float surfaceViewDotNorm = dot(surfaceViewVector, voxNorm);
    float surfaceLightDotNorm = dot(surfaceLightVector, voxNorm);
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
    return (beckmann * approxFres * geoAttenuationFactor) / (pi * surfaceViewDotNorm); //* dot(voxNorm, surfaceLightVector);
}

void LightingInstance(uint3 dispatchThreadID, uint currTargetVox,
                      uint sourceProcessing, float lightEmissFace, float4 lightDir,
                      float2 currTargetVoxPBR, float4 surfaceViewVector )
{
    // If the current face has non-zero emissivity, process it's effect
    // on the target voxel
    float currLightIntensity = lightEmissFace;
    if (currLightIntensity > 0)
    {
        // Take the average lambert of each normal of the current voxel
        // against the current normal of the given light source
        float4 currLightDir = lightDir;
        float lambertA = dot(normFieldBuf[currTargetVox].left, currLightDir);
        float lambertB = dot(normFieldBuf[currTargetVox].right, currLightDir);
        float lambertC = dot(normFieldBuf[currTargetVox].forward, currLightDir);
        float lambertD = dot(normFieldBuf[currTargetVox].back, currLightDir);
        float lambertE = dot(normFieldBuf[currTargetVox].down, currLightDir);
        float lambertF = dot(normFieldBuf[currTargetVox].up, currLightDir);
        float avgLambert = (lambertA + lambertB + lambertC + lambertD + lambertE + lambertF) / 6;

        // Calculate pixel exposure with the overall light direction
        float lambertTerm = saturate(avgLambert);
        float currPixelExposure = lambertTerm * currLightIntensity;

        // Diffuse lighting

        // Cache the result of the Oren-Nayar function in a specific modifier for easy access
        // when we compute the output lighting equation :)
        float orenNayarModifier = OrenNayar(currTargetVoxPBR.x, currTargetVoxPBR.y, surfaceViewVector,
                                            currLightDir, lambertTerm);

        // Specular lighting

        // Cache the result of the Cook-Torrance function in a specific modifier
        // that we can combine with the Oren-Nayar modifier above to generate
        // a mixed diffuse/specular PBR value that we can apply to the rawq
        // pixel exposure calculated earlier
        float cookTorranceModifier = CookTorrance(currTargetVoxPBR.x, currTargetVoxPBR.y, normFieldBuf[currTargetVox].left, surfaceViewVector,
                                                  currLightDir);

        // Combine diffuse/specular lighting

        // Sum the Cook-Torrance modifier with the Oren-Nayar modifier, then apply the result
        // to the exposure for the current pixel
        currPixelExposure *= (orenNayarModifier + cookTorranceModifier);

        // Use the generated exposure value to modify the diffuse light color applied to the
        // current [Pixel]
        // ...unless the [Pixel] exposure is less than zero; don't do anything in that case,
        // negative lighting would be weird :P

        // Safely apply the generated lighting data to the target voxel
        if (currPixelExposure > 0.0f)
        {
            colorTexBuf[currTargetVox] *= currPixelExposure;
        }
    }
}

// Approximate lighting at the current UVW coordinate
void Lighting(uint3 dispatchThreadID, uint currTargetVox, uint sourceProcessing)
{
    // Cache the target voxel's roughness/reflectance properties
    float2 currTargetVoxPBR = pbrTexBuf[currTargetVox];

    // Cache the surface-to-view vector for the current target voxel
    float4 surfaceViewVector = (cameraViewVec - float4(dispatchThreadID, 0));

    // Process lighting separately for each face of the current light source

    LightingInstance(dispatchThreadID, currTargetVox, sourceProcessing,
                     emissTexBuf[sourceProcessing].left, normFieldBuf[sourceProcessing].left,
                     currTargetVoxPBR, surfaceViewVector);

    LightingInstance(dispatchThreadID, currTargetVox, sourceProcessing,
                     emissTexBuf[sourceProcessing].right, normFieldBuf[sourceProcessing].right,
                     currTargetVoxPBR, surfaceViewVector);

    LightingInstance(dispatchThreadID, currTargetVox, sourceProcessing,
                     emissTexBuf[sourceProcessing].forward, normFieldBuf[sourceProcessing].forward,
                     currTargetVoxPBR, surfaceViewVector);

    LightingInstance(dispatchThreadID, currTargetVox, sourceProcessing,
                     emissTexBuf[sourceProcessing].back, normFieldBuf[sourceProcessing].back,
                     currTargetVoxPBR, surfaceViewVector);

    LightingInstance(dispatchThreadID, currTargetVox, sourceProcessing,
                     emissTexBuf[sourceProcessing].down, normFieldBuf[sourceProcessing].down,
                     currTargetVoxPBR, surfaceViewVector);

    LightingInstance(dispatchThreadID, currTargetVox, sourceProcessing,
                     emissTexBuf[sourceProcessing].up, normFieldBuf[sourceProcessing].up,
                     currTargetVoxPBR, surfaceViewVector);
}

[numthreads(1, 1, 1)]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
    // Cache the current voxel index so we can use it later
    uint currVox = (dispatchThreadID.x + dispatchThreadID.y + dispatchThreadID.z);

    // Cache the alpha value of the current voxel so we can re-set it later
    float currVoxAlpha = colorTexBuf[currVox].a;

    // Calculate the lighting emitted onto the current voxel from
    // each voxel in the surrounding area

    // Calculate influences from above the current voxel
    uint upperForwardLightSource = dispatchThreadID.x + (dispatchThreadID.y + 1) + dispatchThreadID.z + 1;
    uint upperLeftForwardLightSource = (dispatchThreadID.x - 1) + (dispatchThreadID.y + 1) + dispatchThreadID.z + 1;
    uint upperRightForwardLightSource = (dispatchThreadID.x + 1) + (dispatchThreadID.y + 1) + dispatchThreadID.z + 1;
    uint upperLeftLightSource = (dispatchThreadID.x - 1) + (dispatchThreadID.y + 1) + dispatchThreadID.z + 1;
    uint upperRightLightSource = (dispatchThreadID.x + 1) + (dispatchThreadID.y + 1) + dispatchThreadID.z + 1;
    uint upperBackwardLightSource = dispatchThreadID.x + (dispatchThreadID.y + 1) + dispatchThreadID.z - 1;
    uint upperLeftBackwardLightSource = (dispatchThreadID.x - 1) + (dispatchThreadID.y + 1) + dispatchThreadID.z - 1;
    uint upperRightBackwardLightSource = (dispatchThreadID.x + 1) + (dispatchThreadID.y + 1) + dispatchThreadID.z - 1;

    Lighting(dispatchThreadID, currVox, upperForwardLightSource);
    Lighting(dispatchThreadID, currVox, upperLeftForwardLightSource);
    Lighting(dispatchThreadID, currVox, upperRightForwardLightSource);
    Lighting(dispatchThreadID, currVox, upperLeftLightSource);
    Lighting(dispatchThreadID, currVox, upperRightLightSource);
    Lighting(dispatchThreadID, currVox, upperBackwardLightSource);
    Lighting(dispatchThreadID, currVox, upperLeftBackwardLightSource);
    Lighting(dispatchThreadID, currVox, upperRightBackwardLightSource);

    // Calculate influences from below the current voxel
    uint lowerForwardLightSource = dispatchThreadID.x + (dispatchThreadID.y - 1) + dispatchThreadID.z + 1;
    uint lowerLeftForwardLightSource = (dispatchThreadID.x - 1) + (dispatchThreadID.y - 1) + dispatchThreadID.z + 1;
    uint lowerRightForwardLightSource = (dispatchThreadID.x + 1) + (dispatchThreadID.y - 1) + dispatchThreadID.z + 1;
    uint lowerLeftLightSource = (dispatchThreadID.x - 1) + (dispatchThreadID.y - 1) + dispatchThreadID.z + 1;
    uint lowerRightLightSource = (dispatchThreadID.x + 1) + (dispatchThreadID.y - 1) + dispatchThreadID.z + 1;
    uint lowerBackwardLightSource = dispatchThreadID.x + (dispatchThreadID.y - 1) + dispatchThreadID.z - 1;
    uint lowerLeftBackwardLightSource = (dispatchThreadID.x - 1) + (dispatchThreadID.y - 1) + dispatchThreadID.z - 1;
    uint lowerRightBackwardLightSource = (dispatchThreadID.x + 1) + (dispatchThreadID.y - 1) + dispatchThreadID.z - 1;

    Lighting(dispatchThreadID, currVox, lowerForwardLightSource);
    Lighting(dispatchThreadID, currVox, lowerLeftForwardLightSource);
    Lighting(dispatchThreadID, currVox, lowerRightForwardLightSource);
    Lighting(dispatchThreadID, currVox, lowerLeftLightSource);
    Lighting(dispatchThreadID, currVox, lowerRightLightSource);
    Lighting(dispatchThreadID, currVox, lowerBackwardLightSource);
    Lighting(dispatchThreadID, currVox, lowerLeftBackwardLightSource);
    Lighting(dispatchThreadID, currVox, lowerRightBackwardLightSource);

    // The alpha channel might have gotten knocked around somewhere in all those lighting
    // calculations, so re-set it here
    colorTexBuf[currVox].a = currVoxAlpha;
}
