#define MAX_POINT_LIGHT_COUNT 16
#define MAX_SPOT_LIGHT_COUNT 16

cbuffer MatBuffer
{
    matrix world;
    matrix view;
    matrix projection;
    float4 animTimeStep;
};

struct Vertex
{
    float4 pos : POSITION0;
    float4 targPos : POSITION1;
    float4 color : COLOR0;
    float4 targColor : COLOR1;
    float4 normal : NORMAL0;
    float4 targNormal : NORMAL1;
    float2 texCoord : TEXCOORD0;
    float grain : COLOR2;
    float reflectFactor : COLOR3;
};

struct Pixel
{
    float4 pos : SV_POSITION0;
    float4 color : COLOR0;
    float4 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float grain : COLOR1;
    float reflectFactor : COLOR2;
    float4 surfaceView : NORMAL1;
    float4 vertWorldPos : NORMAL2;
};

Pixel main(Vertex vertIn)
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

    // Lerp the current vertex color towards it's animation
    // target, then store it in the output [Pixel]
    pixOut.color = lerp(vertIn.color, vertIn.targColor, animTimeStep);

    // Update the input vert's normal with the
    // current world matrix, then re-normalize
    // it
    vertIn.normal = lerp(vertIn.normal, vertIn.targNormal, animTimeStep);
    pixOut.normal = mul(vertIn.normal, world);
    pixOut.normal = normalize(pixOut.normal);

    // Mirror the input vert's texture
    // coordinates into the output [Pixel]
    pixOut.texCoord = vertIn.texCoord;

    // Mirror the input vert's grain
    // into the output [Pixel]
    pixOut.grain = vertIn.grain;

    // Mirror the input vert's reflectiveness
    // into the output [Pixel]
    pixOut.reflectFactor = vertIn.reflectFactor;

    // Extract the camera's look-at vector from the view matrix
    float4 viewColZero = view._m00_m10_m20_m30;
    float4 viewColOne = view._m01_m11_m21_m31;
    float4 viewColTwo = view._m02_m12_m22_m32;
    float4 lookAtVec = float4(viewColZero.z, viewColOne.z, viewColTwo.z, 1);

    // Generate + store a vector representing the normal vector
    // from the current vertex (the "surface") to the camera
    // view
    pixOut.surfaceView = normalize(vertWorldPos - lookAtVec);
    pixOut.surfaceView.w = 1;

    // Store the world-space position of the current vertex
    // within the output pixel (simplifies spot-light masking
    // calculations)
    pixOut.vertWorldPos = vertWorldPos;

    // Pass the output [Pixel] along to
    // the pixel shader
    return pixOut;
}