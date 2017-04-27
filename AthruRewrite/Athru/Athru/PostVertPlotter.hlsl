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

    // Cache a copy of the vertex position in world coordinates
    // (needed for transforming the point into screen space +
    // calculating relative light vectors)
    float4 vertWorldPos = mul(vertIn.pos, world);

    // Apply the world, view, and projection matrices to
    // [vertIn]'s position so that it's aligned with it's
    // parent space and sits inside the window's viewport,
    // then store the results within the output [Pixel]'s
    // position property
    //pixOut.pos = vertIn.pos; //vertWorldPos;
    //pixOut.pos = mul(pixOut.pos, view);
    //pixOut.pos = mul(pixOut.pos, projection);
    pixOut.pos = vertIn.pos; // No real reason to animate the screen-rect atm

    // No need to persist color since we're getting all our
    // base color information from the screen texture; initialise
    // that and everything else mostly associated with boxecules
    // to [0]
    pixOut.color = float4(0, 0, 0, 0);
    pixOut.normal = float4(0, 0, 0, 0);
    pixOut.texCoord = vertIn.texCoord;
    pixOut.grain = 0;
    pixOut.reflectFactor = 0;
    pixOut.surfaceView = float4(0, 0, 0, 0);
    pixOut.vertWorldPos = float4(0, 0, 0, 0);

    // Pass the output [Pixel] along to the pixel shader
    return pixOut;
}