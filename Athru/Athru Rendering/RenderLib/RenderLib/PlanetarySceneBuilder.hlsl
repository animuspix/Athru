
// Player position resolution occurs CPU-side; dispatch this shader
// if the player is known to be on a planetary surface

// Scene globals defined here

// Scene color texture buffer/access view
RWStructuredBuffer<float4> colorTexBuf : register(u0);

// Scene normals texture buffer
RWStructuredBuffer<float4> normFieldBuf : register(u1);

// Scene PBR texture buffer
RWStructuredBuffer<float2> pbrTexBuf : register(u2);

// Scene emissivity texture buffer
RWStructuredBuffer<float> emissTexBuf : register(u3);

// Scene weather pattern texture buffer
RWStructuredBuffer<float> weatherTexBuf : register(u3);

// Camera position buffer; defined here so the planetary
// sampler can easily displace the camera if it intersects
// with the varying terrain altitudes defined in [planetDPosFieldBuf]
// (see below)
RWStructuredBuffer<float> cameraPosBuf : register(u4);

// Planetary globals defined here

// Planetary properties
cbuffer PlanetProps
{
    float4 atmosHue; // How things are tinted during daylight + sky colour normal to the planetary surface
    float atmosDensity; // Atmospheric density for this planet
    float medianAltitude; // Sea-level altitude for this planet
    float medianAltitudeTemp; // Temperature at this planet's median altitude
    float currOrbitalPos; // Current angle of [this] + distance from the local sun(s)
    float localRotation; // The current rotation of [this] about it's local axis (0 rads == facing towards the system COM)
}

[numthreads(8, 8, 16)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    // Sample terrain parameters for every voxel position rendered
    // into the scene, then modify the given textures as appropriate

    // Assumption: Players can only modify voxels visibly contiguous with an
    // above-ground "foundation block" that defines their "base" on each planet
    // This means we can avoid storing permanent data for the entire planet and
    // just maintain a single smaller volume texture for the base + other
    // smaller textures for areas known to be affected by natural disasters,
    // player-alterable space stations, player spaceships, etc.

    // Note:
    // Emissivity should be per-face, not boxecule; there's also six faces in
    // each cube, so the best approach is probably to make the emissivity
    // texture six times larger than the voxel grid width and process each
    // set of six emissivity voxels as the left, right, top, bottom, front,
    // and back faces for a single boxecule

    // Planetary scene breakdown
    // Geological sampling
    // - Tectonic feature sampling
    // - Surface/sub-surface feature generation + refinement
    // - Geological material (roughness, reflectance, color) sampling
    // - Don't go into this yet, but lake/ocean simulation will
    //   probably be a good idea at some point; lake/ocean water should
    //   be slightly less intangible than void (voxels with zero alpha)
    //   is atm
    // - Should implement weather sampling at some point...
    //
    // Critter distribution sampling
    // - Not much value performing that here; sampled weather data should
    //   be passed into a texture and sent along to a dedicated critter
    //   population modelling shader
    //
    // Stellar emissivity calculations
    // - Create stellar scene lighting from relative orientations of
    //   voxel normals and the local sun(s)

    // Geological terrain sampling breakdown:
    // - Alpha sampled from a pre-defined 2D map of tectonic boundaries and
    //   plate interactions
    // - Sub-surface alpha (caves and lava tubes) defined by l-systems
    //   branching off from the appropriate tectonic regions/events
    // - Roughness/reflectance can vary relative to local tectonic pressure;
    //   we might need a set of defined mineral types to really flesh either
    //   out and I kinda ceebs doing that for now
    // - RGB and non-zero alpha can be calculated from known mineral types
    // - Emissivity taken from the dot-product of the sun's position and the
    //   normals of each boxecule face in the scene (their lambert terms)

    // Flowmaps are _probably_ a good approach to physics and animation, but
    // take the time to research volumetric physics/animation in general
    // before settling on any particular technique
}