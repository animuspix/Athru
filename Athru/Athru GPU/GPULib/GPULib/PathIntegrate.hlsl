
#include "RenderUtility.hlsli"
#include "ScenePost.hlsli"

// A two-dimensional texture representing the display; used
// as the image source during denoising/presentation
RWTexture2D<float4> displayTex : register(u0);

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Accumulate shading for each path
    uint2 pixID = floor(pixIn.texCoord * display.xy);
    LiPath currPath = pathBuffer[pixID.x + pixID.y * display.y];
    float3 rgb = 0.0f.xxx;
    for (uint i = 0; i < currPath.bounceCtr; i += 1)
    { rgb += currPath.vts.rho; }

    // Anti-alias, tonemap, record in [displayTex]
    displayTex[currTex] = float4(PathPost(rgb, currPath.camInfo.w), 1.0f);
}