
cbuffer Spatializer
{
	matrix world;
	matrix view;
	matrix projection;
};

struct Vertex
{
    float4 pos : POSITION;
    float4 color : COLOR;
};

struct Pixel
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

Pixel main(Vertex vertIn) : SV_POSITION
{
	vertIn.pos.w = 1.0f;
	vertIn.pos = mul(vertIn.pos, world);
	vertIn.pos = mul(vertIn.pos, view);
	vertIn.pos = mul(vertIn.pos, projection);
	return vertIn.pos;
}