
// Mark [texIn] as a resource bound to the zeroth texture register ([register(t0)])
Texture3D sceneTex : register(t0);

// Mark [wrapSampler] as a resource bound to the zeroth sampler register ([register(s0)])
SamplerState wrapSampler : register(s0);

struct Pixel
{
    float4 pos : SV_POSITION0;
    float3 texCoord : TEXCOORD0;
};

float4 main(Pixel pixIn) : SV_TARGET
{
    // Sample boxecule color from the scene texture
    float4 pixOut = sceneTex.Sample(wrapSampler, pixIn.texCoord);

    // Return final color as a (clamped with [saturate]) multiplicative blend
    // between the raw material color ([pixOut]) and the average light color
    // ([lightColor])
    return saturate(pixOut);
}