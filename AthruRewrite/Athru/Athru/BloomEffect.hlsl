
struct DeferredPixel
{
    // Send color data to the raster buffer (target 0) and
    // normal data to the light buffer (target 1)
    float4 color : SV_Target0;
    float4 normal : SV_Target1;
};

struct FilteredDeferredPixel
{

};

FilteredDeferredPixel main(DeferredPixel pixIn)
{
    FilteredDeferredPixel pixOut;
    pixOut.color = pixIn.color;
    pixOut.normal = pixIn.normal;
    return pixOut;
}