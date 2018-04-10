
// Mark [texIn] as a resource bound to the zeroth texture register ([register(t0)])
Texture2D texIn : register(t0);

// Mark [wrapSampler] as a resource bound to the zeroth sampler register ([register(s0)])
SamplerState wrapSampler : register(s0);

struct Pixel
{
    float4 pos : SV_POSITION0;
    float2 texCoord : TEXCOORD0;
};

float4 main(Pixel pixIn) : SV_TARGET
{
    // Present the generated color for the current pixel
    return texIn.Sample(wrapSampler, pixIn.texCoord);
}