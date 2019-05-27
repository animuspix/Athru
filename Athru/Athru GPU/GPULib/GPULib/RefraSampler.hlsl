// Leaving for now, implementation depends on efficient/accurate interior ray-marching approaches
// Voxelization might be able to address that, but idk for sure without more testing

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex) {}