
#include "SharedLighting.hlsli"

float3 HDR(TracePoint trace)
{

}

float3 FXAA(TracePoint trace)
{

}

float3 BoxBlur(TracePoint trace)
{

}

float3 Brighten(float3 traceColor, float intensity)
{
    return traceColor * intensity;
}

float3 LogDistort(float3 traceColor)
{
    return log10(traceColor) * 10;
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

    // Wrap pixel y-indexing if appropriate
    int2 postPixID = pixID;
    postPixID.y = (rendPassID.x * 64) + threadID;
    if (postPixID.y < 0)
    {
        // Y-values will only ever wrap during the
        // zeroth render pass, so no need to account
        // for the general case
        postPixID.y = (DISPLAY_HEIGHT - threadID);
    }

    // Cache references to the trace point associated with the current
    // pixel + its source color (surface color before indirect
    // illumination)
    TracePoint traceable = traceables[((pixID.x * 64) + threadID)];
    float4 postColor = traceable.rgbaSrc;

    // Apply indirect illumination
    if (traceable.isValid.x)
    {
        postColor = traceable.rgbaGI; //saturate(traceable.rgbaSrc * (traceable.rgbaGI / GI_SAMPLE_COUNT));
		postColor.a = 1.0f;

        if (postColor.r > 10.0f)
        {
            postColor.rgb = float3(0.0f, postColor.r, 0.0f);
        }
    }

    // Convert color to normalized HDR (unimplemented atm)
    // Apply FXAA (unimplemented atm)
    // Apply box-blur (optional)
    // Apply brightening (optional)
    // Apply logarithmic distortion (optional)
    postColor.rgb = LogDistort(postColor.rgb);

    displayTex[postPixID] = postColor;
}