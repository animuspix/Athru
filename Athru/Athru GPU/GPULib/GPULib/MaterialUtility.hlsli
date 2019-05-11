
#ifndef RENDER_BINDS_LINKED
    #include "RenderBinds.hlsli"
#endif
#ifndef CORE_3D_LINKED
    #include "Core3D.hlsli"
#endif

// Small flag to mark whether [this] has already been
// included somewhere in the build process
#define MATERIAL_UTILITIES_LINKED

// Fixed vector indices-of-refraction + extinction
// coefficients for atmosphere/vacuum
// Will likely make atmospheric refraction planet-dependant
// at some point (>> different gasses with different colors/densities)
// but not until I have a solid baseline renderer
// Atmospheric refraction based on earthen air at sea level, measured
// value at 0.8 micrometers found here:
// https://refractiveindex.info/?shelf=other&book=air&page=Ciddor
#define IOR_ATMOS float2(1.00027505f, 0.0f)

// Vacuum has no reflective/refractive effect on incident light
#define IOR_VACUU float2(1.0f, 0.0f)

// Constant stellar brightness; might make this a [cbuffer] value
// so I can vary it between systems
//#define STELLAR_BRIGHTNESS 800000.0f
#define STELLAR_BRIGHTNESS 8000000.0f

// Stellar radiance at unit brightness; mostly just defined for
// readability since any star bright enough to illuminate the
// system will wash out to white for human observers
#define STELLAR_RGB 1.0f.xxx

#ifndef MAT_SYNTH_LINKED
    #include "MatSynth.hlsli"
#endif

// Return whether or not the given BXDF-ID describes a delta distribution
// Only valid for reflective/refractive specular materials atm; I'll update for
// volumetric materials once I've implemented them + decided how to preserve
// diffuse/specular particle choice within [bxdfID]
bool isDeltaBXDF(uint bxdfID)
{
    return bxdfID == BXDF_ID_MIRRO || bxdfID == BXDF_ID_REFRA;
}

// Evaluate the GGX microfacet distribution function for
// the given half-vector + roughness + input direction
// Assumes strictly isotropic roughness
float GGX(float2x3 dirs, // half-vector is in [0], local normal is in [1]
          float vari)
{
    float cTheta = dot(dirs[0], dirs[1]);
    cTheta *= cTheta;
    float denom = ((vari - 1.0f) * cTheta) + 1.0f;
    if (vari == 0.0f) { return 1.0f; } // Return a unit distribution (zero occlusion) for
    								   // perfectly smooth surfaces
    return vari / (PI * (denom * denom));
}

// Evaluate the Smith masking/shadowing function for the
// given directions + surface roughness
// Assumes strictly isotropic roughness
float Smith(float3x3 dirs, // Input vector in [0], half-vector
            	    	   // in [1], output vector in [2]
            float vari)
{
    float2 cThetaIO = float2(dot(dirs[1], dirs[0]),
                             dot(dirs[1], dirs[2]));
    float2 sThetaIO = sqrt(1.0f.xx - (cThetaIO * cThetaIO));
    float2 tThetaIO = abs(sThetaIO / cThetaIO);
    float2 phiIO = float2(atan(dirs[0].y / (dirs[0].x + EPSILON_MIN)),
                          atan(dirs[2].y / (dirs[2].x + EPSILON_MIN)));
    float2 cPhiIO = cos(phiIO);
    cPhiIO *= cPhiIO; // We only use squared cos(phi), no reason to preserve the non-squared version
    float2 sPhiIO = (1.0f.xx - cPhiIO); // No reason to take the root here since we square again later anyway...
    if (isinf(tThetaIO.x) || isinf(tThetaIO.y)) { return 0.0f; } // Try to avoid infinities; assume very shallow microfacet
    									  						 // angles (given by the half-vector) are occluded
    float2 tThetaIOSqr = tThetaIO * tThetaIO;
    float2 lamAlph = sqrt((cPhiIO * vari) +
                          (sPhiIO * vari)); // Valid for isotropic distributions, but probably overcomplicated; would
    									    // like to find/derive a cleaner strictly-isotropic alternative if possible
    lamAlph *= lamAlph * tThetaIOSqr;
    float2 lamIO = (sqrt(1.0f.xx + lamAlph) - 1.0f.xx) * 0.5f;
    // Avert [NaN]s and infinities here
    if (isinf(lamIO.x) || isinf(lamIO.y) ||
        isnan(lamIO.x) || isnan(lamIO.y)) { return 1.0f; }
    else { return 1.0f / (1.0f + lamIO.x + lamIO.y); }
}

// PDF for normals sampled from GGX with Smith masking/shadowing
// Taken from
// Physically Based Rendering - From Theory to Implementation
// (Pharr, Jakob, Humphreys)
// Works because GGX, Smith, and angles between normals and
// intersection directions contribute significantly to reflection
float GGXSmithPDF(float4x3 dirs, // Input direction in [0], half-vector in [1],
    						     // output direction in [2], macrosurface normal
    						     // in [3]
                  float vari)
{
    float2 cThetas = abs(float2(dot(dirs[1], dirs[2]),
                        	    dot(dirs[3], dirs[2])));
    return GGX(float2x3(dirs[1],
                   	    dirs[3]),
               vari) *
           Smith(float3x3(dirs[0],
                	  	  dirs[1],
                 	      dirs[2]),
                 vari) *
           cThetas.x /
           cThetas.y;
}

// Small utility function returning whether two rays lie in the same hemisphere
bool SameHemi(float3x3 dirs) // Input direction in [0], normal in [1], output direction
                             // in [2]
{
    return (sign(dot(dirs[1], dirs[0])) == sign(dot(dirs[1], dirs[2])));
}

// PSR mollification function
// PSR implementation is after Kaplanyan and Dachsbacher, see:
// http://cg.ivd.kit.edu/english/PSR.php
// Small changes needed here to support MLT + refraction; see
// the original paper + supplement for details
#define basePSRRadius PLANETARY_RING_RADIUS * 0.01
#define psrLam -0.16666666667f // Approximately ((1.0f / 6.0f) * -1.0f)
float PSRMollify(float radius,
                 float2x3 ioDirs,
                 float dist)
{
    radius = radius / dist; // Scale mollification radius to match the distance between the
                            // outgoing/incoming vertices
    float mollifAngle = atan(radius); // Evaluate the mollfication angle for the given radius
    float cosIOAngle = dot(ioDirs[0], ioDirs[1]); // Evaluate cosine of the angle between the incoming/outgoing directions for
                                                  // the mollified vertex
    if (acos(cosIOAngle) < mollifAngle) {
        return (1.0f / (TWO_PI * (1.0f - cos(mollifAngle)))) / cosIOAngle; // Return the mollified PDF :D
    }
    else {
        return 0.0f; // Zero probabilities for bounces passing outside the mollification cone
    }
}
