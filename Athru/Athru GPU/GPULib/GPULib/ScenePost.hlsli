#ifndef ANTI_ALIASING_LINKED
    #include "AA.hlsli"
#endif

float3 HDR(float3 traceColor,
           float exposure)
{
    // Apply exposure
    traceColor *= exposure;

    // Apply Hejl & Burgess-Dawson HDR
    float3 hdr = max(0.0f.xxx, traceColor - 0.004f.xxx);
    return (hdr * (6.2f.xxx * hdr + 0.5f.xxx)) / (hdr * (6.2f.xxx * hdr + 1.7f.xxx) + 0.06f.xxx);
}

float3 PathPost(float3 pathRGB,
                float filtVal,
                uint linPixID)
{
    // Apply temporal anti-aliasing, also filter the sample appropriately + apply motion blur
    pathRGB = FrameSmoothing(pathRGB,
                             filtVal,
                             linPixID);

    // Transform colors to low-dynamic-range with the Hejl and Burgess-Dawson operator
    // Exposure will (eventually) be controlled by varying aperture size in a physically-based
    // camera, so avoid adjusting luminance inside the tonemapping operator itself
    return HDR(pathRGB, 1.0f);
}