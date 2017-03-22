
cbuffer MatBuffer
{
    matrix world;
    matrix view;
    matrix projection;
};

// Normals are ignored here since we're treating lighting as
// a post-processing effect, but every shader class deriving
// from the same [Shader] super-class means the properties
// need to be there anyway

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
    float4 normal : COLOR1;
    float2 texCoord : TEXCOORD0;
};

Pixel main(Vertex vertIn)
{
    Pixel output;
    output.pos = mul(vertIn.pos, world);
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, projection);
    output.color = vertIn.color;
    output.normal = vertIn.normal;
    output.texCoord = vertIn.texCoord;
    return output;
}