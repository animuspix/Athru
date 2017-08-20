//#define HORI_RES 1024
//#define VERT_RES 768

// Mark [texIn] as a resource bound to the zeroth texture register ([register(t0)])
Texture2D texIn : register(t0);

// Mark [wrapSampler] as a resource bound to the zeroth sampler register ([register(s0)])
SamplerState wrapSampler : register(s0);

//cbuffer EffectStatusBuffer
//{
//    int4 blurActive;
//    int4 drugsActive;
//    int4 brightenActive;
//    //int4 basicAAActive;
//};

struct Pixel
{
    float4 pos : SV_POSITION0;
    float2 texCoord : TEXCOORD0;
};

//float4 Blur(float4 basePix, int blurRad, float2 flippedTexCoord)
//{
//    float texelHoriSize = 1.0f / HORI_RES;
//    float texelVertSize = 1.0f / VERT_RES;
//
//    [unroll]
//    for (int i = 1; i < (blurRad + 1); i += 1)
//    {
//        basePix += texIn.Sample(wrapSampler, float2(flippedTexCoord.x - (texelHoriSize * i), flippedTexCoord.y));
//        basePix += texIn.Sample(wrapSampler, float2(flippedTexCoord.x + (texelHoriSize * i), flippedTexCoord.y));
//        basePix += texIn.Sample(wrapSampler, float2(flippedTexCoord.x, flippedTexCoord.y + (texelVertSize * i)));
//        basePix += texIn.Sample(wrapSampler, float2(flippedTexCoord.x, flippedTexCoord.y - (texelVertSize * i)));
//    }
//
//    return basePix /= blurRad;
//}
//
//float4 Drugs(float4 basePix, float uvValue)
//{
//    return basePix % sin(uvValue);
//}
//
//float4 Brighten(float4 basePix)
//{
//    return basePix * 4.0f;
//}

//float4 BasicAA(float4 basePix, float2 basePixUV)
//{
//    // Cache the smallest possible steps it's possible
//    // to take along the u/v axes of the screen
//    // texture
//    float texelHoriSize = 1.0f / HORI_RES;
//    float texelVertSize = 1.0f / VERT_RES;
//
//    // Construct local kernel (8 surrounding pixels + the current pixel)
//    float4 colorArr[9];
//    colorArr[0] = texIn.Sample(wrapSampler, float2(basePixUV.x - texelHoriSize, basePixUV.y - texelVertSize));
//    colorArr[1] = texIn.Sample(wrapSampler, float2(basePixUV.x, basePixUV.y - texelVertSize));
//    colorArr[2] = texIn.Sample(wrapSampler, float2(basePixUV.x + texelHoriSize, basePixUV.y - texelVertSize));
//
//    colorArr[3] = texIn.Sample(wrapSampler, float2(basePixUV.x - texelHoriSize, basePixUV.y));
//    colorArr[4] = basePix;
//    colorArr[5] = texIn.Sample(wrapSampler, float2(basePixUV.x + texelHoriSize, basePixUV.y));
//
//    colorArr[6] = texIn.Sample(wrapSampler, float2(basePixUV.x - texelHoriSize, basePixUV.y + texelVertSize));
//    colorArr[7] = texIn.Sample(wrapSampler, float2(basePixUV.x, basePixUV.y + texelVertSize));
//    colorArr[8] = texIn.Sample(wrapSampler, float2(basePixUV.x + texelHoriSize, basePixUV.y + texelVertSize));
//
//    // Evaluate contrast differences between pixels within the kernel
//    // generated above
//    [unroll]
//    for (int i = 0; i < 10; i += 1)
//    {
//        // If the contrast between the base pixel and the pixels
//        // at a tangent to it is high enough, assume the current
//        // pixel sits inside an aliased edge and attempt to
//        // reduce aliasing by averaging the current pixel's color
//        // with the colors of the other pixels in the kernel
//        // (i.e. apply a faint blur)
//        float maxContrast = 0.8f;
//        if (length(colorArr[i]))
//        {
//
//        }
//    }
//}

float4 main(Pixel pixIn) : SV_TARGET
{
    // Present the generated color for the current pixel
    return texIn.Sample(wrapSampler, pixIn.texCoord);
}