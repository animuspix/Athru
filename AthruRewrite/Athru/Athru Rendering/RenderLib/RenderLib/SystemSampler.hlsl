
// Constant value holding voxel grid
// width
static const int VOXEL_GRID_WIDTH = 50;

// Scene globals defined here

// Data-blob containing the six float4s needed for
// each package of voxel normals in [normFieldBuf] (see below)
struct VoxelNormals
{
    float4 left;
    float4 right;
    float4 forward;
    float4 back;
    float4 down;
    float4 up;
};

// Data-blob containing the six floats needed for
// each package of face emissivity values in [emissTexBuf]
// (see below)
struct VoxelFaceEmissivities
{
    float left;
    float right;
    float forward;
    float back;
    float down;
    float up;
};

// Scene color texture buffer/access view
RWTexture3D<float4> colorTexBuf : register(u0);

// Scene normals texture buffer
RWStructuredBuffer<VoxelNormals> normFieldBuf : register(u1);

// Scene PBR texture buffer
RWStructuredBuffer<float2> pbrTexBuf : register(u2);

// Scene emissivity texture buffer
RWStructuredBuffer<VoxelFaceEmissivities> emissTexBuf : register(u3);

// Per-dispatch constants accessed by [this]
cbuffer SystemBuffer
{
    float4 cameraPos;

    float4 starPos;
    float4 starColor;

    float4 planetAPos;
    float4 planetBPos;
    float4 planetCPos;

    float4 planetAColor;
    float4 planetBColor;
    float4 planetCColor;
};

bool CompareFloat4s(float4 first, float4 second)
{
    bool componentZero = (first.r == second.r);
    bool componentOne = (first.g == second.g);
    bool componentTwo = (first.b == second.b);
    bool componentThree = (first.a == second.a);
    return componentZero && componentOne && componentTwo && componentThree;
}

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    // Calculate voxel color

    // Cache the position of the current voxel
    float4 currVoxPos = float4(dispatchThreadID.x, dispatchThreadID.y, dispatchThreadID.z, 0);

    // If the position of the current voxel matches the position of a planet,
    // adjust it's base color to match the average color of the planet;
    // otherwise, make it transparent
    if (CompareFloat4s(currVoxPos, planetAPos))
    {
        colorTexBuf[dispatchThreadID] = planetAColor;
    }
    else if (CompareFloat4s(currVoxPos, planetBPos))
    {
        colorTexBuf[dispatchThreadID] = planetBColor;
    }
    else if (CompareFloat4s(currVoxPos, planetCPos))
    {
        colorTexBuf[dispatchThreadID] = planetCColor;
    }
    else if (CompareFloat4s(currVoxPos, starPos))
    {
        colorTexBuf[dispatchThreadID] = starColor;
    }
    else
    {
        colorTexBuf[dispatchThreadID] = float4(0, 0, 0, 0); //sin(dispatchThreadID.z));
    }

    if (dispatchThreadID.y > 24)
    {
        //colorTexBuf[dispatchThreadID] = float4(0, 0, 0, 0);
    }

    // Calculate lighting properties (voxel normals + PBR)

    // Calculate forward/back and left/right rotation matrices
    // (needed for calculating normals that aren't parallel to the
    // planetary z-axis)

    // Create reference angles for a quarter-circle rotation ("positive 90 degrees")
    // and a three-quarter circle rotation ("negative ninety degrees")
    const float halfCircleRads = 3.14;
    const float quarterCircleRads = halfCircleRads / 2;
    const float threeQuarterCircleRads = quarterCircleRads * 3;

    // Generate left rotation matrix
    matrix leftRotation = matrix(cos(threeQuarterCircleRads), sin(threeQuarterCircleRads), 0, 0,
                                  sin(threeQuarterCircleRads) * -1, cos(threeQuarterCircleRads), 0, 0,
                                  0, 0, 1, 0,
                                  0, 0, 0, 1);

    // Generate right rotation matrix
    matrix rightRotation = matrix(cos(quarterCircleRads), sin(quarterCircleRads), 0, 0,
                                  sin(quarterCircleRads) * -1, cos(quarterCircleRads), 0, 0,
                                  0, 0, 1, 0,
                                  0, 0, 0, 1);

    // Generate forward rotation matrix
    matrix forwardRotation = matrix(cos(quarterCircleRads), 0, sin(quarterCircleRads) * -1, 0,
                                    0, 1, 0, 0,
                                    sin(quarterCircleRads), 0, cos(quarterCircleRads), 0,
                                    0, 0, 0, 1);

    // Generate backward rotation matrix
    matrix backwardRotation = matrix(cos(quarterCircleRads), 0, sin(quarterCircleRads) * -1, 0,
                                     0, 1, 0, 0,
                                     sin(quarterCircleRads), 0, cos(quarterCircleRads), 0,
                                     0, 0, 0, 1);


    // Calculate normals for the current voxel
    float4 upVec = float4(0, 0, 1, 0);
    normFieldBuf[groupIndex].left = normalize(mul(upVec, leftRotation));
    normFieldBuf[groupIndex].right = normalize(mul(upVec, rightRotation));
    normFieldBuf[groupIndex].forward = normalize(mul(upVec, forwardRotation));
    normFieldBuf[groupIndex].back = normalize(mul(upVec, backwardRotation));
    normFieldBuf[groupIndex].down = normalize(upVec * -1);
    normFieldBuf[groupIndex].up = normalize(upVec);

    // Assign placeholder PBR values (no mineral backend atm, so no real way
    // of generating anything more realistic)
    float roughness = 0.5f;
    float reflectance = 0.5f;
    pbrTexBuf[groupIndex] = float2(roughness, reflectance);

    // Calculate initial emissivity values
    emissTexBuf[groupIndex].left = dot(normFieldBuf[groupIndex].left, starPos);
    emissTexBuf[groupIndex].right = dot(normFieldBuf[groupIndex].right, starPos);
    emissTexBuf[groupIndex].forward = dot(normFieldBuf[groupIndex].forward, starPos);
    emissTexBuf[groupIndex].back = dot(normFieldBuf[groupIndex].back, starPos);
    emissTexBuf[groupIndex].down = dot(normFieldBuf[groupIndex].down, starPos);
    emissTexBuf[groupIndex].up = dot(normFieldBuf[groupIndex].up, starPos);
}