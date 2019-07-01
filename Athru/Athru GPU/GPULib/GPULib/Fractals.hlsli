
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
#define BAILOUT_JULIA_DE 256.0f

// Quaternionic Julia iteration function (designed for distance estimation)
float Julia(float4 juliaCoeffs,
            float3 coord)
{
    // Initialise iteration point (z), escape-time derivative (dz),
    // and Julia constant (c)
    float4 z = float4(coord, juliaCoeffs.w);
    float4 dz = float4(1.0f, 0.0f, 0.0f, 0.0f);
    float4 c = juliaCoeffs;

    // Iterate the fractal
    float i = 0;
    float sqrBailout = BAILOUT_JULIA_DE;
    while (i < ITERATIONS_JULIA &&
           dot(z, z) < sqrBailout)
    {
        // Update the Julia differential [dz]
        dz = 2.0f * QtnProduct2(z, dz);

        // Displace the Julia coordinate [z]
        z = QtnProduct2(z, z) + c;

        // Update the iteration count
        i += 1.0f;
    }

    // Return relevant values (number of iterations, terminal
    // value of [z], terminal value of [z]'s derivative [dz])
    float r = length(z);
    return (0.5 * log(r) * r / length(dz));
}
