
struct PostVertex
{
    float4 pos : POSITION0;
    float4 texCoord : TEXCOORD0;
};

struct Pixel
{
    float4 pos : SV_POSITION0;
    float2 texCoord : TEXCOORD0;
};

Pixel main(PostVertex vertIn)
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
    pixOut.texCoord = float2(vertIn.texCoord.x, vertIn.texCoord.y);

    // Pass the output [Pixel] along to the pixel shader
    return pixOut;
}