// Leaving for now, will implement with volumetric light transport

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{}