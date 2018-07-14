
#include "GenericUtility.hlsli"

// A 2D texture fitting the window size (i.e. the display texture);
// carries the multi-frame image data generated by [SceneVis]
Texture2D<float4> displayTex : register(t0);

// A texture sampler; needed to cleanly sample values from the
// display texture in [t0]
SamplerState wrapSampler : register(s0);

struct Pixel
{
    float4 pos : SV_POSITION0;
    float2 texCoord : TEXCOORD0;
};

float4 main(Pixel pixIn) : SV_TARGET
{
    // Read color for the current pixel from [displayTex]
    float4 currPx = displayTex.Sample(wrapSampler, pixIn.texCoord);

    // Define randomly-screened pixels as the box-filtered average of their
    // immediate neighbours
    //if (currPx.a == RAND_PATH_REDUCTION)
    //{
    //    // UV steps with x-increment in [xy] and y-increment in [zw]
    //    float4 uvStep = float4(float2(1.0f / DISPLAY_WIDTH, 0.0f),
    //                           float2(0.0f, 1.0f / DISPLAY_HEIGHT));
    //    float4x3 nearPxls = float4x3(displayTex.Sample(wrapSampler, pixIn.texCoord + uvStep.xy).rgb,
    //                                 displayTex.Sample(wrapSampler, pixIn.texCoord - uvStep.xy).rgb,
    //                                 displayTex.Sample(wrapSampler, pixIn.texCoord + uvStep.zw).rgb,
    //                                 displayTex.Sample(wrapSampler, pixIn.texCoord - uvStep.zw).rgb);
    //    currPx.rgb = (nearPxls[0] +
    //                  nearPxls[1] +
    //                  nearPxls[2] +
    //                  nearPxls[3]) / 4.0f;
    //}

    // Pass the filtered screen texture into the display :)
    return currPx;
}