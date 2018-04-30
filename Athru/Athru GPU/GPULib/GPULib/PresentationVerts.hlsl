
struct ScreenVertex
{
    float4 pos : POSITION0;
    float2 texCoord : TEXCOORD0;
};

struct Pixel
{
    float4 pos : SV_POSITION0;
    float2 texCoord : TEXCOORD0;
};

Pixel main(ScreenVertex vertIn)
{
	// Create a returnable [Pixel]
    Pixel pixOut;

    // The screen-rect is positioned in NDC's by default, so no
    // reason to transform it
    pixOut.pos = vertIn.pos;

    // Pass vertex texture coordinates along to the output [Pixel]
    pixOut.texCoord = vertIn.texCoord;

    // Pass the output [Pixel] along to the pixel shader
    return pixOut;
}