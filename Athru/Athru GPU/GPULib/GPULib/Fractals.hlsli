
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
#define BAILOUT_JULIA_ESCAPE 4.0f

// Analytical Julia normal generator; original from:
// https://www.shadertoy.com/view/MsfGRr
// by [iq]. Thanks! :D
float3 JuliaGrad(float4 juliaCoeffs,
                 float3 coord)
{
    // Initialise iteration point (z) and Julia constant (c)
    float4 z = float4(coord, juliaCoeffs.w);
    float4 c = juliaCoeffs;

    // Initialize the Jacobian matrix [j]; used to compute the
    // gradient at [z] (i.e. the analytical normal)
    // We need a Jacobian matrix here because [dz] captures
    // average rate of change near [z], whereas the quality we
    // want (the gradient) is defined as the direction of
    // *greatest* change at the same point, which depends on
    // the rates of change along every direction near [z]
    // (=> the partial derivatives in the Jacobian)
    float4x4 j = float4x4(1.0f, 0.0f, 0.0f, 0.0f,
                          0.0f, 1.0f, 0.0f, 0.0f,
                          0.0f, 0.0f, 1.0f, 0.0f,
                          0.0f, 0.0f, 0.0f, 1.0f);
    // Iterate the fractal
    int i = 0;
    float sqrBailout = BAILOUT_JULIA_DE;
    while (i < ITERATIONS_JULIA &&
           dot(z, z) < sqrBailout)
    {
        // Update the Jacobian term [j]
        float3 w = z.yzw * -1.0f.xxx;
        j = j * float4x4(z.x, w.x, w.y, w.z,
                         z.y, z.x, 0.0f, 0.0f,
                         z.z, 0.0f, z.x, 0.0f,
                         z.w, 0.0f, 0.0f, z.x);

        // Displace the Julia coordinate [z]
        z = QtnProduct2(z, z) + c;

        // Update the iteration count
        i += 1;
    }

    // Extract local gradient from the iterated Jacobian, then
    // return
    return normalize(mul(j, z).xyz);
}

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
    int i = 0;
    float sqrBailout = BAILOUT_JULIA_DE;
    while (i < ITERATIONS_JULIA &&
           dot(z, z) < sqrBailout)
    {
        // Update the Julia differential [dz]
        dz = 2.0f * QtnProduct2(z, dz);

        // Displace the Julia coordinate [z]
        z = QtnProduct2(z, z) + c;

        // Update the iteration count
        i += 1;
    }

    // Return relevant values (number of iterations, terminal
    // value of [z], terminal value of [z]'s derivative [dz])
    float r = length(z);
    return (0.5 * log(r) * r / length(dz));
}
