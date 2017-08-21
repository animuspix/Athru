
#include "SharedLighting.hlsli"

// The maximum number of steps to ray-march during path-tracing
#define MAX_PATH_MARCHER_STEPS 255

// A read-only view of a copy of the display texture (read-only views support
// multi-dimensional reads, read/write views do not)
Texture2D<float4> displayTexReadOnly : register(t1);

[numthreads(4, 4, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Too few group registers to load everything into the x-axis during dispatch, so
    // linearize the current group offset over here
    // We can get to 4K support with only the X and Y dispatch registers occupied,
    // so a simple 2D linear map will work just as well as a more complicated 3D
    // linearization
    uint linearGroupID = groupID.x + (groupID.y * ((64 * DISPLAY_WIDTH) / 4));

    // Too few registers in Shader Model 5.0 to deploy 256 separate
    // threads in each group, so deploy 64 threads/group and four
    // groups/trace instead and just mod the current group ID against
    // the number of traces per group (i.e. 65536) to generate an
    // appropriate trace index for each thread
    uint currTraceNdx = linearGroupID % (64 * DISPLAY_WIDTH);

    // Not all potential trace points are valid, so test the
    // point selected by the current thread and break out
    // immediately if appropriate
    if (!(traceables[currTraceNdx].isValid.x))
    {
        return;
    }

    // Cache the figure + 3D point associated with the
    // selected ray intersection
    uint currFig = traceables[currTraceNdx].figID.x;
    float3 baseCoord = traceables[currTraceNdx].coord.xyz;

    // Fetch the local scene normal
    float3 norm = GetNormal(baseCoord);

    // Make lighting faintly material-dependant by XORing the direction
    // of each ray against the color channels of the current figure
    // Note: the buffer used for GPU random-number generation is mutable,
    // so although the below calls all access the same index the value
    // stored at that index will change unpredictably after each call,
    // and the values that actually go into the x/y/z axes won't
    // neccessarily have any relationship to the value of [i] that was
    // used to access them
    uint3 diffColorInt = (uint)(figuresReadable[currFig].surfRGBA.rgb * 255.0f);
    float3 rayDir = normalize(float3(uint3(rand1D(threadID + (linearGroupID * 64)),
                                           rand1D(threadID + (linearGroupID * 64)),
                                           rand1D(threadID + (linearGroupID * 64)))));// ^ diffColorInt));

    // Try to guarantee that [rayDir] is never below the figure surface
    // (since surfaces won't be receiving radiation contributions from
    // themselves)
    // This works by evaluating the dot product (linear projection) of
    // the ray direction and the surface normal we calculated before,
    // then multiplying the ray direction by the sign of that value;
    // if the ray is inside the surface it's projection against the
    // normal will be negative and the operation will flip it back out,
    // whereas if the normal is outside the surface it's projection
    // will be positive and the direction of the ray will stay the
    // same (since n * 1 == n)
    float rayProj = dot(rayDir, norm);
    rayDir *= (rayProj / abs(rayProj));

    // March down the path taken by the ray associated with the
    // current thread
    float maxPathDist = 10.0f;
    float currRayDist = rayEpsilon * 10.0f;
    for (int j = 0; j < MAX_PATH_MARCHER_STEPS; j += 1)
    {
        float3 pathVec = baseCoord + (rayDir * currRayDist);
        DFData pathData = SceneField(pathVec);
        if (pathData.dist < rayEpsilon)
        {
            // Accumulate the traced color into the overall color for the
            // current path
			traceables[currTraceNdx].rgbaGI += float4(PerPointDirectIllum(pathVec, pathData.rgbaColor.rgb,
                                                                          GetNormal(pathVec)), 1.0f);

            return;
        }
		
        currRayDist += pathData.dist;

        if (currRayDist > (maxPathDist - rayEpsilon))
        {
            break;
        }
    }

	// No occluders or light sources encountered, so assume that the current
    // ray has no effect on the output color for the current trace point
    traceables[currTraceNdx].rgbaGI += saturate(traceables[currTraceNdx].rgbaGI +
                                                (1.0f.xxxx / 256.0f));
}