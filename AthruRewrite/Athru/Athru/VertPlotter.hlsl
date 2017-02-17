
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

Pixel main(Vertex inVert)
{
    Pixel output;
	
    output.pos.w = 1.0f;
	output.pos = mul(inVert.pos, world);
	output.pos = mul(inVert.pos, view);
	output.pos = mul(inVert.pos, projection);
    output.color = inVert.color;
	
    return output;
}