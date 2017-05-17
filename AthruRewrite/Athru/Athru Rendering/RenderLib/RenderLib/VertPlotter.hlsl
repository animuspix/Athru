
#define VOXEL_GRID_WIDTH 50

cbuffer MatBuffer
{
    matrix world;
    matrix view;
    matrix projection;
};

struct SceneVertex
{
    float4 pos : POSITION0;
    float3 texCoord : TEXCOORD0;
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

    // Apply the transformation matrices to the current
    // vertex so that it's positioned/scaled/rotated appropriately
    // relative to the world, then apply the view + projection matrices
    // so that it moves relative to the camera and (if visible)
    // sits inside the window's viewport
    vertIn.pos += float4(vertIn.texCoord * VOXEL_GRID_WIDTH, 0);
    pixOut.pos = mul(vertIn.pos, world);
    pixOut.pos = mul(pixOut.pos, view);
    pixOut.pos = mul(pixOut.pos, projection);

    // Mirror the input vert's texture
    // coordinates into the output [Pixel]
    pixOut.texCoord = vertIn.texCoord;

    // Pass the output [Pixel] along to
    // the pixel shader
    return pixOut;
}