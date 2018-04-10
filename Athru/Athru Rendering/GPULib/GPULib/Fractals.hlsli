
// Rigorous quaternion product, works for any length
// (DE-friendly)
// Adapted from MacSlow's "qMult" function, viewable
// over here:
// https://www.shadertoy.com/view/lttSzX
float4 QtnProduct2(float4 qtnA, float4 qtnB)
{
    float x = qtnA.x * qtnB.x - dot(qtnA.yzw, qtnB.yzw);
    float3 yzw = (qtnA.x * qtnB.yzw) +
                 (qtnB.x * qtnA.yzw) +
                 cross(qtnA.yzw, qtnB.yzw);
    return float4(x, yzw);
}

// Maximum iteration count + bailout distance
// for the quaternionic Julia distance estimator
#define ITERATIONS_JULIA 8
#define BAILOUT_JULIA 16.0

// Stored iteration count, value of [z], and value
// of [dz] for an arbitrary quaternionic Julia
// fractal
struct JuliaProps
{
    float iter;
    float4 z;
    float4 dz;
};

// Quaternionic Julia iteration function (designed for distance estimation)
JuliaProps Julia(float4 juliaCoeffs,
                 float3 coord,
                 int maxIter)
{
    // Initialise escape-time value (z), escape-time derivative (dz),
    // and Julia constant (c)
    float4 z = float4(coord, juliaCoeffs.w);
    float4 dz = float4(1.0f, 0.0f, 0.0f, 0.0f);
    float4 c = juliaCoeffs;

    // Dynamically calculated scalar factor for the
    // Julia derivative [dz]
    // Distance-dependant variance brings the fractal
    // rate-of-change (represented by [dz]) closer to
    // the rate of change in the image signal between
    // separate rays
    // [2.0f] is the base case (applies when the camera
    // is positioned at the fractal's local origin)
    float dzScalar = 2.0f + length(coord) / 10.0f;

    // Iterate the fractal
    int i = 0;
    float sqrBailout = (BAILOUT_JULIA * BAILOUT_JULIA);
    while (i < maxIter &&
           dot(z, z) < sqrBailout)
    {
        dz = dzScalar * QtnProduct2(z, dz);
        z = QtnProduct2(z, z) + c;
        i += 1;
    }

    // Return relevant values (number of iterations, terminal
    // value of [z], terminal value of [z]'s derivative [dz])
    JuliaProps props;
    props.iter = i;
    props.z = z;
    props.dz = dz;
    return props;
}