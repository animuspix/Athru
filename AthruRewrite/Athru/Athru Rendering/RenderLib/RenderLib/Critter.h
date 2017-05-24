#pragma once

class Critter
{
	public:
		Critter();
		~Critter();

	private:
		// Critter volumes defined through turtle functions
		// A given turtle function writes to five separate 3D textures representing
		// bounding cubes; one representing per-voxel frequency/amplitude pairs
		// (r+g is frequency, b+a is amplitude) per voxel, one representing
		// per-voxel emissive intensity (intensity == value), one representing base
		// color, another representing PBR roughness, and one last one representing
		// PBR reflectance

		// > The galaxy is defined with a given function /f/
		// > Every star system is defined with a given function /g/
		// > Every planet is associated with another function /h/
		// > Planetary function rules are modified by player interaction
		// > Changes to planetary functions trickle up to star functions
		// > Star function may return planetary functions as output
		// > Two distinct types of planetary function: long/lat and spherical surface
		//
		// > Concrete scene defined as a 200*200*200 grid of polygon-cube instances
		// > Scene textures have size exactly matching the size of the scene grid
		// > Current star system found by resolving the galactic function for the current position
		// > Sampled area properties gauged by resolving the star function (selected above) for the current position
		// > If the star function says the player is currently on the surface of a particular planet, fill the scene textures by sampling the long/lat function associated with that
		//   planet; otherwise sample the spherical surface function associated with every visible planet
		// > Baseline planetary sampling performed first (generating terrain surface)
		// > Followed by vegetative critter distribution sampling (placing trees, grasses, etc.)
		// > Followed by animalistic critter distribution sampling (placing living creatures in the scene)
		// > Planet generation functions should define 10 distinct organisms matching various conditions in each of 10 latitudinal bands (more variance can emerge from precise
		//   long/lat coordinates); those organisms should all have common properties specified by merging the appropriate .dna files into the relevant compute shader file
		//   during a pre-processing stage
		//
		// > Camera position defined in ordinary XYZ coordinates
		// > Camera is always at the center of the rendered sample-grid
		// > Sampling area can be defined as the 200*200*200 cube surrounding the camera :D
		//
		// Problem:
		// Compute shaders are immutable, so function evolution is either possible but very slow (CPU-side) or impossible (GPU-side)
		// Probably reasonable to follow NMS and store additive deltas for each voxel in known-modified regions; regional deltas can be
		// cached every time the player sees something change and stored in a texture array during runtime before being saved to a
		// series of files at shutdown
		//
		// Functions would have to be written per-galaxy/star system/planet, so make them user-editable in the longer-term version of Athru; save time by only writing primitive
		// versions of each for now (e.g. create a system with one system containing one star and a planet)
};

