#define MAX_POINT_LIGHT_COUNT 16
#define MAX_SPOT_LIGHT_COUNT 16

cbuffer MatBuffer
{
    matrix world;
    matrix view;
    matrix projection;
    float4 animTimeStep;
};

struct SceneVertex
{
    float4 pos : POSITION0;
    float4 texCoord : TEXCOORD0;
};

struct Pixel
{
    float4 pos : SV_POSITION0;
    float3 texCoord : TEXCOORD0;
};

Pixel main(SceneVertex vertIn)
{
    // Create a returnable [Pixel]
    Pixel pixOut;

    // Lerp the current vertex position towards it's animation target
    vertIn.pos = lerp(vertIn.pos, vertIn.targPos, animTimeStep);

    // Cache a copy of the vertex position in world coordinates
    // (needed for transforming the point into screen space +
    // calculating relative light vectors)
    float4 vertWorldPos = mul(vertIn.pos, world);

    // Apply the world, view, and projection
    // matrices to [vertIn]'s position so that
    // it's aligned with it's parent space and
    // sits inside the window's viewport, then
    // store the results within the output
    // [Pixel]'s position property
    pixOut.pos = vertWorldPos;
    pixOut.pos = mul(pixOut.pos, view);
    pixOut.pos = mul(pixOut.pos, projection);

    // Mirror the input vert's texture
    // coordinates into the output [Pixel]
    pixOut.texCoord = float3(vertIn.texCoord.x, vertIn.texCoord.y, vertIn.texCoord.z);

    // Pass the output [Pixel] along to
    // the pixel shader
    return pixOut;
}