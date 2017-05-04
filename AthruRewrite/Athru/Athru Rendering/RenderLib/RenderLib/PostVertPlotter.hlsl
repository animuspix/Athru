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
    float2 texCoord : TEXCOORD0;
};

Pixel main(Vertex vertIn)
{
	// Create a returnable [Pixel]
    Pixel pixOut;

    // The screen-rect is positioned in NDC's by default, so no
    // reason to transform it
    pixOut.pos = vertIn.pos;

    // No need to persist color since we're getting all our
    // base color information from the screen texture; initialise
    // that and everything else mostly associated with boxecules
    // to [0]
    pixOut.texCoord = vertIn.texCoord;

    // Pass the output [Pixel] along to the pixel shader
    return pixOut;
}