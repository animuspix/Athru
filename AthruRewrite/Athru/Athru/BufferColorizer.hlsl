// Mark [postTexIn] as a resource bound to the zeroth texture register ([register(t0)])
Texture2D postTexIn : register(t0);

// Mark [wrapSampler] as a resource bound to the zeroth sampler register ([register(s0)])
SamplerState wrapSampler : register(s0);

struct Pixel
{
    // Most of these won't be used, they're just defined
    // so we can use this pixel shader with the [vertPlotter]
    // vertex shader and avoid duplicating everything
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float4 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

float4 main(Pixel pixIn) : SV_TARGET
{
    float4 color = postTexIn.Sample(wrapSampler, pixIn.texCoord);
    return color;
}