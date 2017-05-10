
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

// Scene critter-distribution texture buffer
RWStructuredBuffer<float> critterDistTexBuf : register(u3);

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
    // Sample alpha variance from the delta-color texture segment
    // Check if the camera is inside the variated terrain; if it is,
    // displace it out above the closest point on the surface

    // Resolve a temperature for each voxel in the current scene, then
    // use that information to adjust critter distributions, local
    // colour, local emissivity, etc.

    // Unsure about megatexture sampling vs. mixed delta-texture/scene-texture
    // sampling vs. pure function-to-scene-texture sampling; ask Adam when possible

    // Static distributions are not a problem
    // Animal/plant distributions can be calculated statistically from short functions
    // given enough initial data, also can be made context-sensitive so that they vary
    // depending on time of day, distributions of prey species, etc.

    // Dynamic distributions are harder
    // Why? Distributions calculated per-frame are equivalent to dynamic distributions
    // Sequential distribution calculation can even allow chaotic species models, where
    // the combination of solar and/or weather change results in local migrations
    // Animals generally move in groups
    // Animal-type critter distributions can be modulated by density + food availability

    // But what about animations?
    // Critters are assumed to take up an arbitrary given total volume (a bounding box)
    // Animation is permitted within a critter's volume
    // Animations are only tracked while critters are within the scene
    // Critter animations are (initially) defined as morphs between different base forms
    // More complex animations may be defined by algebraic distortions applied to critter
    // functions; probably better to focus on basic morphs before considering that :P

    // Critter distributions can be solved with system modelling and statistics
    // Voxel distributions may be harder (e.g. after displacement)
    // This basically entails either (a) modelling entire planets with megatextures or
    // (b) modelling the terrain of each planet as a complex system
    // Complex system modelling is very expensive
    // But can easily represent nearly any displacement
    // Can it be performed in discrete tiles?
    // If so, tiled distributions may be an option
    // Possibly also take up an old idea and model each 1x1x1m cube as a set of smaller
    // grains, then model each 1x1x1m cube as a grain within the scene
    // Alternatively: compute scene flow from raw forces in a separate shader, then
    // import the results and apply each vector to the appropriate area of the scene
    // (displacement can be represented with selective opacity masking)
    // Flowmaps would have to be calculated per-planet-tile and alternately
    // loaded/cached/saved/unloaded as the player moved around the map
    // Tiles in a 3x3 grid surrounding the player could be interpolated to provide
    // smooth displacement changes as the player transitioned between tiles

    // Flowmaps are a *static* distribution technique; however, re-calculating them each
    // frame /should/ result in the separate static moments blending together and producing
    // a dynamic effect

    // Flowmaps and critter statistics sound good, but more research into voxel animation is
    // probably a good idea; once I know how my approach stacks up against published
    // alternatives I can move forward and define the relevant critter properties, define a
    // critter buffer inside the critter-building shader, and work out how to interpret
    // the turtle-functions used to define each critter
}