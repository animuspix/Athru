// Scene globals defined here

// Scene color texture buffer/access view
RWStructuredBuffer<float4> colorTexBuf : register(u0);

// Scene normals texture buffer
RWStructuredBuffer<float4> normFieldBuf : register(u1);

// Scene PBR texture buffer
RWStructuredBuffer<float2> pbrTexBuf : register(u2);

// Scene emissivity texture buffer
RWStructuredBuffer<float> emissTexBuf : register(u3);

[numthreads(8, 8, 16)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    //colorTexBuf[[iteratorVal]] = float4(1.0f, 1.0f, 1.0f, 1.0f);
    //normFieldBuf[[adjustedIteratorVal]] = conditionalVec;
    //pbrTexBuf[iteratorVal] = float4(1.0f, 1.0f);
    //emissTexBuf[[iteratorVal]] = float4(1.0f);
}