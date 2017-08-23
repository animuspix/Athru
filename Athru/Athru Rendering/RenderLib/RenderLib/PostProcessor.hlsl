
#include "SharedLighting.hlsli"
#include "SharedGI.hlsli"

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

    // Cache references to the trace point associated with the current
    // pixel + its source color (surface color before indirect
    // illumination)
    TracePoint traceable = traceables[((pixID.x * 64) + threadID)];
    float4 postColor = traceable.rgbaSrc;

    // Apply indirect illumination
    if (traceable.isValid.x)
    {
        float offsetScale = 0.0f;
        float4 giSum = float4(0.0f, 0.0f, 0.0f, 0.0f);
        for (uint i = 0; i < GI_SAMPLES_PER_RAY; i += 1)
        {
            giSum += giCalcBufReadable[(((pixID.x * 64) + threadID) * GI_SAMPLES_PER_RAY) + i];
        }

        postColor = saturate(traceable.rgbaSrc * (giSum / GI_SAMPLES_PER_RAY));
		postColor.a = 1.0f;
    }

    // Convert color to normalized HDR (unimplemented atm)
    // Apply FXAA (unimplemented atm)
    // Apply box-blur (optional)
    // Apply brightening (optional)

    displayTex[pixID] = postColor;
}