
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

float4 main(Vertex inVert) : SV_POSITION
{
	inVert.pos.w = 1.0f;
	inVert.pos = mul(inVert.pos, world);
	inVert.pos = mul(inVert.pos, view);
	inVert.pos = mul(inVert.pos, projection);
	return inVert.pos;
}