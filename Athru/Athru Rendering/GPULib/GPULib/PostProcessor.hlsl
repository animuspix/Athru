
//#include "SharedLighting.hlsli"
//#include "SharedGI.hlsli"
#include "Lighting.hlsli"

float3 HDR(float3 traceColor, float exposure)
{
    // Apply exposure
    traceColor *= exposure;

    // Apply Hejl & Burgess-Dawson HDR
    float3 hdr = max(0.0f.xxx, traceColor - 0.004f.xxx);
    return (hdr * (6.2f.xxx * hdr + 0.5f.xxx)) / (hdr * (6.2f.xxx * hdr + 1.7f.xxx) + 0.06f.xxx);
}

float3 FXAA(TracePix trace)
{

}

float3 BoxBlur(TracePix trace)
{

}

float3 Brighten(float3 traceColor, float intensity)
{
    return traceColor * intensity;
}

[numthreads(4, 4, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // The pixel ID (x/y coordinates) associated with the current
    // thread
    uint2 pixID = groupID.xy;
    pixID.y = (rendPassID.x * 64) + threadID;

    // Return without performing post-processing if the pixel ID of
    // the current thread sits outside the screen texture
    if (pixID.y > DISPLAY_HEIGHT)
    {
        return;
    }

    else if (pixID.y < 0)
    {
        // Y-values will only ever wrap during the
        // zeroth render pass, so no need to account
        // for the general case
        pixID.y = (DISPLAY_HEIGHT - threadID);
    }

    // Calculate pixel color
    float4 postColor = 0.0f.xxxx;
    int linearPixID = pixID.x + (threadID * DISPLAY_WIDTH);
    for (uint i = 0; i < GI_SAMPLES_PER_RAY; i += 1)
    {
        postColor += giCalcBufReadable[(linearPixID * GI_SAMPLES_PER_RAY) + i].pixRGBA / GI_SAMPLES_PER_RAY;
    }
	postColor.a = 1.0f;

    // Convert color to normalized HDR
    postColor.rgb = HDR(postColor.rgb, 1.0f);

    // Apply FXAA (unimplemented atm)
    // Apply box-blur (optional)
    // Apply brightening (optional)

    // Store the transformed color
    displayTex[pixID] = saturate(postColor);
}