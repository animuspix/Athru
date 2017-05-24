
// Constant values for heightfield dimensions,
// voxel grid width, etc.
static const int HEIGHTFIELD_WIDTH = 512;
static const int VOXEL_GRID_WIDTH = 50;
static const int TWOS_COMPLEMENT_ALTITUDE = 18000;

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

// Upper-hemisphere heightfield
StructuredBuffer<float4> upperTectoTexBuf : register(t0);

// Lower-hemisphere heightfield
StructuredBuffer<float4> lowerTectoTexBuf : register(t1);

// Scene globals defined here

// Scene color texture buffer/access view
RWStructuredBuffer<float4> colorTexBuf : register(u0);

// Scene normals texture buffer
RWStructuredBuffer<VoxelNormals> normFieldBuf : register(u1);

// Scene PBR texture buffer
RWStructuredBuffer<float2> pbrTexBuf : register(u2);

// Scene emissivity texture buffer
RWStructuredBuffer<VoxelFaceEmissivities> emissTexBuf : register(u3);

// Per-dispatch constants accessed by [this]
cbuffer PlanetBuffer
{
	//float4 weighting;
    float4 cameraSurfacePos;
    float4 planetAvgColor;
    float4 starPos;
};

[numthreads(1, 1, 1)]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
    // Calculate voxel color

    // Assign the planet's average color to the scene color texture
    // by default
    int currVox = dispatchThreadID.x + dispatchThreadID.y + dispatchThreadID.z;
    colorTexBuf[currVox] = planetAvgColor;

    // Extract the elevation of the current planar area from either heightfield

    // Calculate the surface position of the current voxel
    float4 currVoxSurfacePos = float4((dispatchThreadID.x + cameraSurfacePos.x) - (VOXEL_GRID_WIDTH / 2),
                                      (dispatchThreadID.y + cameraSurfacePos.y) - (VOXEL_GRID_WIDTH / 2),
                                      (dispatchThreadID.z + cameraSurfacePos.z) - (VOXEL_GRID_WIDTH / 2), 1);

    // Modulate the player's two-dimensional position into a usable set of
    // heightfield coordinates
    float2 heightfieldPos = float2(cos(currVoxSurfacePos.x) * (currVoxSurfacePos.x % HEIGHTFIELD_WIDTH),
                                   sin(currVoxSurfacePos.y) * (currVoxSurfacePos.y % HEIGHTFIELD_WIDTH));

    // Calculate a one-dimensional index from the position generated above; then
    // use the index to extract an elevation (between -(altitude / 2) and +(altitude / 2))
    // in each heightfield for the camera's current position
    int heightFieldIndex = heightfieldPos.x + heightfieldPos.y;
    float upperHeightfieldElev = (TWOS_COMPLEMENT_ALTITUDE * upperTectoTexBuf[heightFieldIndex].r) - (TWOS_COMPLEMENT_ALTITUDE / 2);
    float lowerHeightfieldElev = (TWOS_COMPLEMENT_ALTITUDE * lowerTectoTexBuf[heightFieldIndex].r) - (TWOS_COMPLEMENT_ALTITUDE / 2);

    // Interpolate between the upper/lower heightfields as the player gets closer/further
    // from the equator (0, 0, [planetRadius])
    float localElev = lerp(upperHeightfieldElev, lowerHeightfieldElev, 1 / length(currVoxSurfacePos.xy));

    // If the the current voxel sits beyond the elevation defined by the heightfields, make
    // it transparent
    if ((float)dispatchThreadID.z > localElev)
    {
        colorTexBuf[currVox] = float4(0.0f, 0.0f, 0.0f, 0.0f);
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
    matrix leftRotation = matrix(cos(threeQuarterCircleRads), sin(threeQuarterCircleRads),       0, 0,
                                  sin(threeQuarterCircleRads) * -1, cos(threeQuarterCircleRads), 0, 0,
                                  0,                                      0,                     1, 0,
                                  0,                                      0,                     0, 1);

    // Generate right rotation matrix
    matrix rightRotation = matrix(cos(quarterCircleRads), sin(quarterCircleRads),      0, 0,
                                  sin(quarterCircleRads) * -1, cos(quarterCircleRads), 0, 0,
                                  0,                                 0,                1, 0,
                                  0,                                 0,                0, 1);

    // Generate forward rotation matrix
    matrix forwardRotation = matrix(cos(quarterCircleRads),       0,     sin(quarterCircleRads) * -1, 0,
                                    0,                            1,                               0, 0,
                                    sin(quarterCircleRads),       0,     cos(quarterCircleRads),      0,
                                    0,                            0,                               0, 1);

    // Generate backward rotation matrix
    matrix backwardRotation = matrix(cos(quarterCircleRads),       0,     sin(quarterCircleRads) * -1, 0,
                                     0,                            1,                               0, 0,
                                     sin(quarterCircleRads),       0,     cos(quarterCircleRads),      0,
                                     0,                            0,                               0, 1);


    // Calculate normals for the current voxel
    normFieldBuf[currVox].left = normalize(mul(currVoxSurfacePos, leftRotation));
    normFieldBuf[currVox].right = normalize(mul(currVoxSurfacePos, rightRotation));
    normFieldBuf[currVox].forward = normalize(mul(currVoxSurfacePos, forwardRotation));
    normFieldBuf[currVox].back = normalize(mul(currVoxSurfacePos, backwardRotation));
    normFieldBuf[currVox].down = normalize(currVoxSurfacePos * -1);
    normFieldBuf[currVox].up = normalize(currVoxSurfacePos);

    // Assign placeholder PBR values (no mineral backend atm, so no real way
    // of generating anything more realistic)
    float roughness = 0.5f;
    float reflectance = 0.5f;
    pbrTexBuf[currVox] = float2(roughness, reflectance);

    // Calculate initial emissivity values
    emissTexBuf[currVox].left = dot(normFieldBuf[currVox].left, starPos);
    emissTexBuf[currVox].right = dot(normFieldBuf[currVox].right, starPos);
    emissTexBuf[currVox].forward = dot(normFieldBuf[currVox].forward, starPos);
    emissTexBuf[currVox].back = dot(normFieldBuf[currVox].back, starPos);
    emissTexBuf[currVox].down = dot(normFieldBuf[currVox].down, starPos);
    emissTexBuf[currVox].up = dot(normFieldBuf[currVox].up, starPos);
}