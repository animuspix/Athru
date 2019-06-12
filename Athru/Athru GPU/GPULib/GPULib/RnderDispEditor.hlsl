#include "RenderBinds.hlsli"

// Per-bounce indirect dispatch axes
// (tracing axes in (0...2), sampling axes in (3...5), input traceable
// count in [6], input sampling count in [7]
RWBuffer<uint> dispAxes : register(u9);

// Append/consume counters for traceables + material primitives
RWBuffer<uint> traceCtr : register(u10);
RWBuffer<uint> diffuCtr : register(u11);
RWBuffer<uint> mirroCtr : register(u12);
RWBuffer<uint> refraCtr : register(u13);
RWBuffer<uint> snowwCtr : register(u14);
RWBuffer<uint> ssurfCtr : register(u15);
RWBuffer<uint> furryCtr : register(u16);

// Resources below aren't strictly needed for dispatch editing, but keeping
// them around simplifies root descriptor management during command-list
// assembly at startup

// Buffer carrying intersections across ray-march iterations
// Simply ordered, used to minimize dispatch sizes for
// ray-marching
AppendStructuredBuffer<uint> traceables : register(u17);

// Material intersection buffers
// Can save some performance + simplify header structure by converting these
// back to standard buffers and appending/consuming manually; might implement
// that after/during validation
ConsumeStructuredBuffer<uint> diffuIsections : register(u18);
ConsumeStructuredBuffer<uint> mirroIsections : register(u19);
ConsumeStructuredBuffer<uint> refraIsections : register(u20);
ConsumeStructuredBuffer<uint> snowwIsections : register(u21);
ConsumeStructuredBuffer<uint> ssurfIsections : register(u22);
ConsumeStructuredBuffer<uint> furryIsections : register(u23);

[numthreads(2, 1, 1)]
void main(uint threadID : SV_GroupIndex)
{
    // Generate tracing/sampling axes
    // Possible to add extra complexity here by spreading dispatches evenly across
    // (x, y, z)
    uint dSampling = 0;
    switch(threadID)
    {
        case 0:
            dispAxes[0] = (traceCtr[0] != 0) ? (traceCtr[0] / 256) + 1 : 0;
            dispAxes[1] = 1;
            dispAxes[2] = 1;
            dispAxes[6] = traceCtr[0];
            break;
        case 1:
            uint dSampling = diffuCtr[0] + mirroCtr[0] + refraCtr[0] +
                             snowwCtr[0] + ssurfCtr[0] + furryCtr[0];
            dispAxes[3] = (dSampling != 0) ? (dSampling / 256) + 1 : 0;
            dispAxes[4] = 1;
            dispAxes[5] = 1;
            dispAxes[7] = dSampling;
            break;
    }
}
