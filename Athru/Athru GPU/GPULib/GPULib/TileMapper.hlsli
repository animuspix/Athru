
// Returns linear pixel index in [x], two-dimensional index in [yz]
uint3 TilePx(uint2 tileID,
             uint frameCtr,
             uint resX,
             uint2 tileArea)
{
    // Compute 2D pixel position
    uint2 px = uint2(frameCtr % 2,
                     ((frameCtr % 4) / 2)) + // Compute offset
                     (tileID * tileArea); // Sum into the scaled tile origin
    // Linearize pixel positions, then return both forms (1D in [x], 2D in [yz])
    return uint3(px.x + px.y * resX,
                 px);
}