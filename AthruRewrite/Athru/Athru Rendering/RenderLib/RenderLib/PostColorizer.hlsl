#define HORI_RES 1024
#define VERT_RES 768

// Mark [texIn] as a resource bound to the zeroth texture register ([register(t0)])
Texture2D texIn : register(t0);

// Mark [wrapSampler] as a resource bound to the zeroth sampler register ([register(s0)])
SamplerState wrapSampler : register(s0);

cbuffer EffectStatusBuffer
{
    int4 blurActive;
    int4 drugsActive;
    int4 brightenActive;
};

struct Pixel
{
    float4 pos : SV_POSITION0;
    float2 texCoord : TEXCOORD0;
};

float4 Blur(float4 basePix, int blurRad, float2 flippedTexCoord)
{
    float texelHoriSize = 1.0f / HORI_RES;
    float texelVertSize = 1.0f / VERT_RES;

    [unroll]
    for (int i = 1; i < (blurRad + 1); i += 1)
    {
        basePix += texIn.Sample(wrapSampler, float2(flippedTexCoord.x - (texelHoriSize * i), flippedTexCoord.y));
        basePix += texIn.Sample(wrapSampler, float2(flippedTexCoord.x + (texelHoriSize * i), flippedTexCoord.y));
        basePix += texIn.Sample(wrapSampler, float2(flippedTexCoord.x, flippedTexCoord.y + (texelVertSize * i)));
        basePix += texIn.Sample(wrapSampler, float2(flippedTexCoord.x, flippedTexCoord.y - (texelVertSize * i)));
    }

    return basePix /= blurRad;
}

float4 Drugs(float4 basePix, float uvValue)
{
    return basePix % sin(uvValue);
}

float4 Brighten(float4 basePix)
{
    return basePix * 4.0f;
}

float4 main(Pixel pixIn) : SV_TARGET
{
    // Account for the different origins for the window
    // and UV space by flipping the given UV about the
    // Y-axis
    float2 flippedTexCoord = float2(pixIn.texCoord.x, 1 - pixIn.texCoord.y);

    // Sample basic color data for the current pixel
    float4 pixAtCurrUV = texIn.Sample(wrapSampler, flippedTexCoord);

    // Apply various post-effects
    float4 pixOut = pixAtCurrUV;
    if (blurActive[0])
    {
        pixOut = Blur(pixAtCurrUV, 10, flippedTexCoord);
    }

    if (drugsActive[0])
    {
        pixOut = Drugs(pixOut, pixIn.texCoord.y);
    }

    if (brightenActive[0])
    {
        pixOut = Brighten(pixOut);
    }

    // Return the processed color
    return saturate(pixOut);
}