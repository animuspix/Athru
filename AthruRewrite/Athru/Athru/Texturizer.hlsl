
// Mark [texIn] as a resource bound to the zeroth texture register ([register(t0)])
Texture2D texIn : register(t0);

// Mark [wrapSampler] as a resource bound to the zeroth sampler register ([register(s0)])
SamplerState wrapSampler : register(s0);

struct Pixel
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float4 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct DeferredPixel
{
    // Send color data to the raster buffer (target 0) and
    // normal data to the light buffer (target 1)
    float4 color : SV_Target0;
    float4 normal : SV_Target1;
};

DeferredPixel main(Pixel pixIn) : SV_TARGET
{
    DeferredPixel pixOut;
    // Multiplicative blend between the object color and the
    // texture color exposed by the sampler for the current pixel
    pixOut.color = pixIn.color * texIn.Sample(wrapSampler, pixIn.texCoord);
    pixOut.normal = pixIn.normal;
    return pixOut;
}