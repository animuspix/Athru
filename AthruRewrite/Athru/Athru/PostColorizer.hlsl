// Mark [texIn] as a resource bound to the zeroth texture register ([register(t0)])
Texture2D texIn : register(t0);

// Mark [wrapSampler] as a resource bound to the zeroth sampler register ([register(s0)])
SamplerState wrapSampler : register(s0);

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

float4 main(Pixel pixIn) : SV_TARGET
{
    return texIn.Sample(wrapSampler, pixIn.texCoord);
}