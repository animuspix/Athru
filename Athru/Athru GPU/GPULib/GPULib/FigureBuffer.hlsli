// Struct representing basic data associated with
// Athru figures
struct Figure
{
    // The location + uniform scale (in [w]) of this figure at
    // any particular time
    float4 linTransf;

    // Coefficients of the distance function used to render [this]
    float4 distCoeffs[3];
};

// A one-dimensional buffer holding the objects to render
// in each frame + their properties
StructuredBuffer<Figure> figures : register(t0);