
// Mark [sceneTex] as a resource bound to the zeroth texture register ([register(t0)])
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
    // Return the color stored in the scene texture for the
    // current pixel
    return float4(sin(pixIn.texCoord.x), 1, sin(pixIn.texCoord.z), 1);//sceneTex.Sample(wrapSampler, pixIn.texCoord);
}