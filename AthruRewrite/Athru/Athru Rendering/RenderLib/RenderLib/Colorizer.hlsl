
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
    // If the current pixel is fully transparent, discard it; otherwise,
    // [return] it as input to the current render target
    float4 currPix = sceneTex.Sample(wrapSampler, pixIn.texCoord);
    if (currPix.a == 0)
    {
        discard;
    }

    //else if (currPix.a < 1)
    //{
    //  // DIY alpha blending here... (inbuilt blending creates artifacts during
    //  // post-processing)
    //}

    return currPix;
}