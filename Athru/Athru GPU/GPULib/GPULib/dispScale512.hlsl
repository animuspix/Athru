#include "DispatchScale.hlsli"

[numthreads(2, 3, 4)]
void main(uint ndx : SV_GroupIndex)
{
    DispatchScale(512, ndx);
}