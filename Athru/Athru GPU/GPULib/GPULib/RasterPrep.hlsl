#include "RenderBinds.hlsli"
#include "PhiloInit.hlsli"
#include "HDR.hlsli"
#ifndef ANTI_ALIASING_LINKED
    #include "AA.hlsli"
#endif

// Per-bounce indirect dispatch axes
// (tracing axes in the zeroth channel, sampling axes in (1...6))
RWBuffer<uint> dispAxes : register(u12);

// Append/consume counters for traceables + material primitives
RWBuffer<uint> traceCtr : register(u13);
RWBuffer<uint> diffuCtr : register(u14);
RWBuffer<uint> mirroCtr : register(u15);
RWBuffer<uint> refraCtr : register(u16);
RWBuffer<uint> snowwCtr : register(u17);
RWBuffer<uint> ssurfCtr : register(u18);
RWBuffer<uint> furryCtr : register(u19);

// Filtering/AA occurs with tiled pixel positions, so include the pixel/tile-mapper
// here
#include "TileMapper.hlsli"

// Constant denoising filter kernel
static float h[25] =
{
	1 / 256.0f,  1 / 64.0f,  3 / 128.0f,  1 / 64.0f,  1 / 256.0f,
	1 / 64.0f,  1 / 16.0f,  3 / 32.0f,  1 / 16.0f,  1 / 64.0f,
	3 / 128.0f,  3 / 32.0f,  9 / 64.0f,  3 / 32.0f,  3 / 128.0f,
	1 / 064.0f,  1 / 16.0f,  3 / 32.0f,  1 / 16.0f,  1 / 64.0f,
	1 / 256.0f,  1 / 64.0f,  3 / 128.0f,  1 / 64.0f,  1 / 256.0f
};

// Offset map for the kernels defined above
static uint2 krnOffs[25] =
{
    uint2(-2,-2), uint2(-1,-2), uint2(0,-2), uint2(1,-2), uint2(2,-2),
    uint2(-2,-1), uint2(-1,-1), uint2(0,-1), uint2(1,-1), uint2(2,-1),
    uint2(-2,0), uint2(-1,0), uint2(0,0), uint2(1,0), uint2(2,0),
    uint2(-2,1), uint2(-1,1), uint2(0,1), uint2(1,1), uint2(2,1),
    uint2(-2,2), uint2(-1,2), uint2(0,2), uint2(1,2), uint2(2,2)
};

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Extract a pixel ID from the given thread/group IDs
    uint2 tileID = uint2((groupID.x * TRACING_GROUP_WIDTH) + (threadID % TRACING_GROUP_WIDTH),
                         (groupID.y * TRACING_GROUP_WIDTH) + (threadID / TRACING_GROUP_WIDTH));
    uint linTileID = tileID.x + (tileID.y * gpuInfo.tilingInfo.x);
    if (linTileID > gpuInfo.tilingInfo.z - 1) { return; } // Mask off excess threads

    // Clear counters & dispatch axes in the zeroth thread
    if (threadID == 0)
    {
        if (linTileID == 0)
        {
            dispAxes[0] = 0.0f;
            dispAxes[1] = 0.0f;
            dispAxes[2] = 0.0f;
            dispAxes[3] = 0.0f;
            dispAxes[4] = 0.0f;
            dispAxes[5] = 0.0f;
            traceCtr[0] = 0;
            diffuCtr[0] = 0;
            mirroCtr[0] = 0;
            refraCtr[0] = 0;
            snowwCtr[0] = 0;
            ssurfCtr[0] = 0;
            furryCtr[0] = 0;
        }
    }

    // Generate per-frame pixel coordinate, sample texel
    uint3 px = TilePx(tileID,
                      uint(gpuInfo.tInfo.z),
                      gpuInfo.resInfo.x,
                      gpuInfo.tileInfo.xy);
	float4 tx = displayTex[px.yz];

    // Apply edge-avoiding wavelet a-trous denoising
    // Source paper:
    // http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.672.3796&rep=rep1&type=pdf
    // Reference implementation:
    // https://www.shadertoy.com/view/ldKBzG
    // Viable and *probably* well-implemented, but not useful for Athru atm because image tiling
    // interferes with the edge-stopping function
    // The same problem exists for every denoiser because the tiling pattern is entangled with the
    // per-frame signal (and if I don't entangle it )
    float3 c = tx.rgb;
    #define DENOISE_WAVELET
    //#define DENOISE_MEANS
    #ifdef DENOISE_WAVELET
        float3 n = nrmBuffer[px.x];
        float3 x = posBuffer[px.x];
        float stepWidth = gpuInfo.denoiseInfo.x; // Unsure how much I should set this externally

        // 5x5 basis filter
        // Compute rt/nomal/position means
        float3x3 m = float3x3(0.0f.xxx,
                              0.0f.xxx,
                              0.0f.xxx);
        for (uint l = 0u; l < 25u; l += 1u)
        {
            uint2 eltID = clamp(px.yz + krnOffs[l] * stepWidth, 0.0f.xx, gpuInfo.resInfo.xy); // Keep kernel positions within screen-space bounds
            uint linEltID = eltID.x + eltID.y * gpuInfo.resInfo.x;
            m += float3x3(displayTex[eltID].rgb,
                          nrmBuffer[linEltID],
                          posBuffer[linEltID]);
        }
        m /= 25.0f;

        // Compute standard deviations (sigma values) for the current pixel
        float3x3 vsigmas = float3x3(abs(m[0] - tx.rgb),
                                    abs(m[1] - posBuffer[px.x]),
                                    abs(m[2] - nrmBuffer[px.x]));
        float3 sigmas = 0.0f.xxx;
        sigmas = float3(dot(vsigmas[0], vsigmas[0]),
                        dot(vsigmas[1], vsigmas[1]),
                        dot(vsigmas[2], vsigmas[2]));

        // Compute denoising contribution for the current pass
        float3 k = 0.0f;
        float3 d = 0.0f;
        for (uint j = 0u; j < 25u; j += 1u)
        {
            // Update filter normalization factor
            uint2 eltID = clamp(px.yz + krnOffs[j] * stepWidth, 0.0f.xx, gpuInfo.resInfo.xy); // Keep kernel positions within screen-space bounds
            uint linEltID = eltID.x + eltID.y * gpuInfo.resInfo.x;
            float3x3 qTx = float3x3(displayTex[eltID].rgb,
                                    nrmBuffer[linEltID],
                                    posBuffer[linEltID]);
            float3x3 dI = float3x3(c - qTx[0],
                                   n - qTx[1],
                                   x - qTx[2]);
            float3 sqrPixDist = float3(dot(dI[0], dI[0]),
                                       dot(dI[1], dI[1]),
                                       dot(dI[2], dI[2]));
            float3 wrt = min(exp(sqrPixDist[0] / sigmas.x), 1.0f);
            float3 wn = min(exp(-(max(sqrPixDist[1], 0.0f)) / sigmas.y), 1.0f);
            float3 wx = min(exp(-sqrPixDist[2] / sigmas.z), 1.0f);
            float3 currK = h[j] * wrt * wn * wx;
            k += currK;
            // Update (non-normalized) filter value for the current element
            d += currK * qTx[0];
        }
        c = d / ((k == 0.0f) ? 1.0f : k); // Apply the current filter pass over the starting pixel colour/previous passes

    #endif
    #ifdef DENOISE_MEANS
        c = 0.0f.xxx;
        for (uint i = 0u; i < 25u; i += 1)
        {
            uint2 eltID = clamp(px.yz + krnOffs[i], 0.0f.xx, gpuInfo.resInfo.xy); // Keep kernel positions within screen-space bounds
            c += displayTex[eltID].rgb;
        }
        c /= 25.0f;
    #endif

    // Apply filtering + temporal AA + motion blur
    c = FrameSmoothing(c,
                       tx.a,
                       px.x,
                       gpuInfo.resInfo.z);

    // Tonemap, then transfer to LDR output + the screenshot buffer
    // Fixed exposure for now, might connect to a histogram in future builds;
    //displayTex[px.yz] = 0.001f.xxxx;
    float4 imgOut = float4(HDR(c, 1.0f), 1.0f);
    px.z = gpuInfo.resInfo.y - px.z; // Flip pixel y-axes to match window space (origin in the upper-left)
    displayTexLDR[px.yz] = imgOut; 
                           //float4(tx.rgb, 1.0f);
    screenshotBuf[px.y + px.z * gpuInfo.resInfo.x] = imgOut;
}