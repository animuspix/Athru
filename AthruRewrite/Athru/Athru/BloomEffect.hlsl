
// Mark [colorTexIn] as a resource bound to the zeroth texture register ([register(t0)])
Texture2D colorTexIn : register(t0);

// Mark [lightTexIn] as a resource bound to the first texture register ([register(t1)])
Texture2D lightTexIn : register(t1);

// Mark [postTexIn] as a resource bound to the second texture register ([register(t2)])
Texture2D postTexIn : register(t2);

// Mark [wrapSampler] as a resource bound to the zeroth sampler register ([register(s0)])
SamplerState wrapSampler : register(s0);

struct LitDeferredPixel
{
    float2 texCoord : TEXCOORD0;
};

struct FilteredPixel
{
    float4 rawColor : SV_TARGET0;
    float4 rawNormal : SV_TARGET1;
    float4 outColor : SV_TARGET2;
};

FilteredPixel main(LitDeferredPixel pixIn)
{
    FilteredPixel pixOut;

    // HLSL requires us to write to every render target in each pass,
    // so we just clone out the existing color and normal values
    // into the relevant targets before sending the actual post data
    // to [outColor]
    pixOut.rawColor = colorTexIn.Sample(wrapSampler, pixIn.texCoord);
    pixOut.rawNormal = lightTexIn.Sample(wrapSampler, pixIn.texCoord);
        
    // Lighting shaders write their output to the post buffer, so we can
    // pull the data we want to manipulate out of there before processing it
    // Post-processing would happen here...
    pixOut.outColor = postTexIn.Sample(wrapSampler, pixIn.texCoord);
    return pixOut;
}