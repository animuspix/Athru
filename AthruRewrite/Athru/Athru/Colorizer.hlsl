
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
    pixOut.color = pixIn.color;
    pixOut.normal = pixIn.normal;
    return pixOut;
}