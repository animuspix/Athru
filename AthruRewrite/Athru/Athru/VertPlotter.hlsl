
cbuffer MatBuffer
{
    matrix world;
    matrix view;
    matrix projection;
};

struct Vertex
{
    float4 pos : POSITION;
    float4 color : COLOR;
    float4 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct Pixel
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float4 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

Pixel main(Vertex vertIn)
{
    Pixel output;
    output.pos = mul(vertIn.pos, world);
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, projection);
    output.normal = vertIn.normal;
    output.texCoord = vertIn.texCoord;
    output.color = vertIn.color;
    return output;
}