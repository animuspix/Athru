
#include "FigureBuffer.hlsli"
#include "Fractals.hlsli"

// Small raster-specific input buffer
cbuffer RasterInput
{
    // Planet to rasterize in [x], buffer length in [y],
    // rasterization width in [z], half rasterization width in [w]
    uint4 raster;
};

// The read/write buffer we want to rasterize to
// Carries planets along [Y], animals along [X], plants along [Z]
RWBuffer<float> rasterAtl : register(u5);

// 512 threads/group
// Expected 4096 groups (128^3 threads total)
[numthreads(8, 8, 8)]
void main(uint3 tID : SV_DispatchThreadID)
{
    // Compute a buffer index for the current thread
    // Indices are offset by the planet index
    uint bufNdx = (tID.x + tID.y * raster.z) + // XY offset
                  (tID.z * (raster.z * raster.z)) + // Z-offset (one full "sheet" per z-index)
                  (raster.x * (raster.z * raster.z * raster.z)); // Planetary offset (one volume per planet index)

    // Immediately return for any excess threads
    // (since we're trying to maximise occupancy and many buffer lengths
    // won't have neat cube roots)
    if (bufNdx > raster.y) { return; }

    // Centre the current voxel position
    float3 vx = float3((float3)tID - raster.www);

    // Normalize to [0...1]
    vx /= raster.z * 0.5f;

    // Naive Julia rasterization gives spherical distances, considering alternatives here
    // (e.g. escape-time, only voxels with an especially high iteration count)

    // Output planetary distance at [vx]
    rasterAtl[bufNdx] = length(vx) - 0.5f;
                        //Julia(figures[0].distCoeffs[0], vx);
}