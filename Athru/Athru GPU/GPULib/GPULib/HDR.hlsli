
float3 HDR(float3 traceColor,
           float exposure)
{
    // Apply exposure
    traceColor *= exposure;

    // Apply Hejl & Burgess-Dawson HDR
    float3 hdr = max(0.0f.xxx, traceColor - 0.004f.xxx);
    return (hdr * (6.2f.xxx * hdr + 0.5f.xxx)) / (hdr * (6.2f.xxx * hdr + 1.7f.xxx) + 0.06f.xxx);
}
