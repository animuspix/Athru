
#include "SharedLighting.hlsli"

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
        postColor = saturate(traceable.rgbaSrc * (traceable.rgbaGI / GI_SAMPLE_COUNT));
    }

    // Apply FXAA (unimplemented atm)
    // Apply box-blur (optional)
    // Apply brightening (optional)
    // Apply logarithmic distortion (optional)

    displayTex[postPixID] = postColor;
}