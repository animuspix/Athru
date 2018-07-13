
// Small flag to mark whether [this] has already been
// included somewhere in the build process
#define GEOMETRIC_UTILITIES_LINKED

// Converts the given normalized spherical (theta, phi) coordinates to a normalized
// Cartesian vector
float3 AnglesToVec(float2 thetaPhi)
{
    float4 thetaPhiSiCo;
    sincos(thetaPhi.x, thetaPhiSiCo.x, thetaPhiSiCo.y);
    sincos(thetaPhi.y, thetaPhiSiCo.z, thetaPhiSiCo.w);
    return float3(thetaPhiSiCo.x * thetaPhiSiCo.w,
                  thetaPhiSiCo.x * thetaPhiSiCo.z,
                  thetaPhiSiCo.y);
}

// Converts the given Cartesian vector to normalized spherical (theta, phi) coordinates;
// the [r] term is dropped because all the generated values are known to sit on the unit
// sphere
float2 VecToAngles(float3 rayVec)
{
    // Extract spherical coordinates, then return the result
    // Conversion function assumes y-up
    return float2(acos(rayVec.z),
                  atan(rayVec.y / rayVec.x));
}

// Generates input/output spherical angles for
// use with surface BXDFs; swaps ray directions
// lying on light paths so we can avoid calling
// specialized adjoint BSDFs during direct
// gathers + path attenuation
float4 RaysToAngles(float3 inVec,
                    float3 outVec,
                    bool lightPath)
{
    float2x2 thetaPhiIO = float2x2(VecToAngles(inVec), // Evaluate [theta, phi] for the incoming ray
                                   VecToAngles(outVec)); // Evaluate [theta, phi] for the outgoing ray

    // Return the generated angles + swap the incoming/outgoing
    // directions if they lie on a light path
    return float4(thetaPhiIO[lightPath].xy,
                  thetaPhiIO[(lightPath + 1) % 2].xy);
}

// Generates a transformation matrix for the
// local space defined by the given surface
// normal
float3x3 NormalSpace(float3 normal)
{
    // Each possible solution for the [x-basis] assumes some direction is zero and uses that
    // to solve the plane equation exactly orthogonal to the normal direction
    // I.e. given (n.x)x + (n.y)y + (n.z)z = 0 we can resolve the x-basis in three different ways
    // depending on which axis we decide to zero:
    // y = 0 >> (n.x)x + (n.z)z = 0 >> (n.x)x = 0 - (n.z)z >> n.x(x) = -n.z(z) >> (x = n.z, z = -n.x)
    // x = 0 >> (n.y)y + (n.z)z = 0 >> (n.y)y = 0 - (n.z)z >> n.y(y) = -n.z(z) >> (y = n.z, z = -n.y)
    // z = 0 >> (n.x)x + (n.y)y = 0 >> (n.x)x = 0 - (n.y)y >> n.x(x) = -n.y(y) >> (x = n.z, y = -n.x)
    // This assumption becomes less reliable as the direction chosen by any of the solutions gets further
    // and further away from zero; the most effective way to minimize that error is to use the solution
    // corresponding to the smallest direction
    // In other words...
    // When x = min(x, y, z), solve for x = 0
    // When y = min(x, y, z), solve for y = 0
    // When z = min(x, y, z), solve for z = 0
    float3 absNormal = abs(normal);
    float minAxis = min(min(absNormal.x, absNormal.y),
                        absNormal.z);
    // My minimum-selection function
    float3 basisX = 0.0f.xxx;
    //if (minAxis == absNormal.x)
    //{
    //    // Define the x-basis when [x] is the smallest axis
    //    basisX = float3(0.0f, normal.z, normal.y * -1.0f);
    //}
    //else if (minAxis == absNormal.y)
    //{
    //    // Define the x-basis when [y] is the smallest axis
    //    basisX = float3(normal.z, 0.0f, normal.x * -1.0f);
    //}
    //else
    //{
    //    // Define the x-basis when [z] is the smallest axis
    //    basisX = float3(normal.z, normal.x * -1.0f, 0.0f);
    //}

    // Scratchapixel's alternative (only defined for the x <=> y relationship, also
    // used by PBR)
    // Not totally sure why this works...should probably ask CGSE
    if (absNormal.x > absNormal.y)
    {
        basisX = float3(normal.z, 0.0f, normal.x * -1.0f);
    }
    else
    {
        basisX = float3(0.0f, normal.z, normal.y * -1.0f);
    }

    // Build the normal-space from the normal vector and the x-basis we generated before,
    // then return the result
    return float3x3(normalize(basisX),
                    normal,
                    normalize(cross(normal, basisX)));
}

// Small convenience function for 3x3 matrix inverses
// 3x3-specific inversion algorithm from
// https://www.wikihow.com/Find-the-Inverse-of-a-3x3-Matrix
//float3x3 MatInv(float3x3 mat)
//{
//    // Find 3D determinant of the initial matrix
//    float det3D = determinant(mat);
//    mat = transpose(mat); // Tranpose [mat]
//    if (det3D == 0) { return mat; } // Return the transpose for matrices with no valid inverse
//
//    // Generate cofactor matrix
//    // 1 0 0
//    // 0 1 0
//    // 0 0 1
//    float3x3 cofMat = float3x3(determinant(float2x2(mat._22_23_32_33)), determinant(float2x2(mat._21_32_13_33)), determinant(float2x2(mat._12_22_13_23)),
//                               determinant(float2x2(mat._21_31_32_33)), determinant(float2x2(mat._21_32_13_33)), determinant(float2x2(mat._21_32_13_33)),
//                               determinant(float2x2(mat._21_21_22_23)), determinant(float2x2(mat._21_32_13_33)), determinant(float2x2(mat._21_32_13_33)));
//
//    // Alter signs as appropriate, apply the inverse 3D determinant, then return
//    return cofMat * float3x3(1.0f, -1.0f, 1.0f,
//                             -1.0f, 1.0f, -1.0f,
//                             1.0f, -1.0f, 1.0f) / det3D;
//}

// Physically-correct path tracing is strictly defined in terms of area, but it's often
// more convenient to use integrals over solid angle instead; this function provides a
// quick way to convert solid angle values into their area-based equivalents
// (conveniently unnecessary for ordinary incremental path-tracing (evaluating a given
// solid-angle PDF in terms of differential area means applying the angle-to-area
// transformation to sampled probabilities; dividing samples by that PDF means nullifying
// the original effects of the angle-to-area conversion on the path-tracing integral and
// removing it from the integral's definition), but required for BDPT (deterministic
// visibility rays >> 100% chance of tracing any given path between fixed vertices >>
// no reason to remove stochastic bias by applying a PDF))
float AngleToArea(float3 posA,
                  float3 posB,
                  float3 normal,
                  bool overSurf)
{
    // Projection over differential area is described with
    // (dA * cos(theta)) / d^2
    // for boundary surfaces, and
    // dA / d^2
    // for scattering volumes (grains are assumed small enough and uniform enough to be unaffected
    // by facing ratio)
    // This is derived from the definitions of radiance over area, where it makes more sense;
    // a differentially-sized patch of light will emit energy in the direction [posA - posB] equivalent
    // to the size of the patch ([dA]) scaled by the facing ratio of the patch towards [normal]
    // (if appropriate) and divided by the squared length of [posA - posB] (since radiance spreads out
    // over the surface area of the light volume at any given distance)
    // [dA] isn't passed in as a parameter here, but it doesn't need to be; any solid-angle value scaled
    // against the output of [this] will convert into it's area-based equivalent

    // Define values we'll need for the conversion function
    float4 outVec = float4(posB - posA, 0.0f);
    outVec.w = length(outVec.xyz);

    // Most vertices will interact with surfaces rather than media, so
    // evaluate facing ratio here
    float cosAtten = dot(normal,
                         outVec.xyz / outVec.w);
    cosAtten = (cosAtten * !overSurf) + overSurf; // Lock attenuation to [1.0f] for vertices within scattering volumes
                                                  // (like e.g. media, variable-density glass, vegetation...)
    return cosAtten / (outVec.w * outVec.w); // Scale attenuation by the squared distance between incoming/outgoing vertices, then
                                             // return the generated conversion to the callsite
}

// Specialized version of [AngleToArea(...)] for converting combined forward/reverse PDFs during MIS
float2 AnglePDFsToArea(float3 inPos,
                       float3 basePos,
                       float3 outPos,
                       float2x3 normals,
                       bool2 overSurf)
{
    // Define values we'll need for the conversion function
    float2x3 dirSet = float2x3(outPos - basePos,
                               inPos - basePos);
    float2 distVec = float2(length(dirSet[0]),
                            length(dirSet[1]));
    float2 distSqrVec = distVec * distVec;

    // Most vertices will interact with surfaces rather than media, so
    // evaluate facing ratio here
    float2 cosAtten = float2(dot(normals[0],
                                 dirSet[0] / distVec.x),
                             dot(normals[1],
                                 dirSet[1] / distVec.y));
    cosAtten = (cosAtten * !overSurf) + overSurf; // Lock attenuation to [1.0f] for vertices within scattering volumes
                                                  // (like e.g. media, variable-density glass, vegetation...)
    return cosAtten / distSqrVec; // Scale attenuation by the squared distance between incoming/outgoing vertices, then
                                  // return the generated conversion to the callsite
}