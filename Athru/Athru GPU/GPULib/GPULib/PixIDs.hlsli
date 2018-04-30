
// Small function to extract pixel IDs from the given render-pass ID, thread ID, number of
// progressive passes/frame, and thread-group ID
// Returns pixel coordinate in [(x, y)] and pixel validity (whether or not the pixel is
// within the range [([0...maxWidth], [0...maxHeight])]) in [z]
uint2 PixelID(uint2 rendPassID,
              uint threadID,
              uint3 groupID,
              uint numProgPatches,
              uint maxWidth,
              uint maxHeight,
              out bool validity)
{
    // "Width" of the current dispatch
    uint dispWidth = 0;
    float cubeRt = pow(numProgPatches.x, 1.0f / 3.0f);
    float cubeRtNearest = ceil(cubeRt);
    if (pow(cubeRtNearest, 3.0f) == (int)numProgPatches.x)
    {
        dispWidth = (uint)cubeRtNearest;
    }
    else
    {
        dispWidth = (uint)pow(numProgPatches.x, 0.5f);
    }


    // Linearized form of the group ID associated with the current thread
    uint linGroupID = (groupID.x + (groupID.y * dispWidth)) +
                      (groupID.z * (dispWidth * dispWidth));

    // The pixel ID (x/y coordinates) associated with the current
    // thread
    uint dispOffset = (uint)pow(numProgPatches.x, 0.5f);
    uint2 pixID = uint2(((rendPassID.x * (8 * dispOffset)) + threadID % 8) + ((linGroupID % dispOffset) * 8),
                        ((rendPassID.y * (8 * dispOffset)) + threadID / 8) + ((linGroupID / dispOffset) * 8));

    // Store whether or not [pixID] is a valid pixel (=> within the range [([0...maxWidth], [0...maxHeight])])
    validity = pixID.x < maxWidth &&
               pixID.y < maxHeight;

    // Return the generated pixel coordinates
    return pixID;
}