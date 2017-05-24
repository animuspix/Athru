#pragma once

#include <d3d11.h>
#include <directxmath.h>

#include "AthruTexture2D.h"
#include "AthruTexture3D.h"

// Storing separate shaders for every possible planet in a galaxy is essentially impossible;
// C++ doesn't support that degree of abstraction and the memory costs are insane anyway
// However, although the number of possible planets is extremely large it /is/ finite, so we
// can simplify the problem with *archetypes*. Each archetype represents a basic planetary
// pattern and the number of possible planets rises near-exponentially with the number of
// basic patterns (since patterns can be combined with arbitrary weightings per-pattern).
//
// This allows us to store planets as a series of pattern weights, where the rendering
// pre-processor flushes the scene textures with zeroes before sampling each pattern in
// sequence and lerping the scene texture towards their output with [t] equal to the
// weighting assigned to the relevant pattern
//
// System storage can also work with pattern blends, allowing relatively easy storage for
// millions of possible stars
//
// Critter generation may be entirely emergent of planetary properties. Plants and animals
// evolve for their environment, and it isn't really possible to load tens of thousands of
// shaders at runtime (hundreds of stars, with two-three planets each, with at least a
// hundred critters per planet; that's a minimum of 100*200 = 20000 separate critter
// functions that all need to be stored in separate shaders)
//
// Although it's very tempting to voxelize material density/pressure, we're *already*
// assuming mostly-earthlike systems. This means we don't /need/ to discretize density or
// pressure across an entire planet; it's effective enough (and much cheaper) to specify
// average surface-level air/water/lithic pressure/density directly inside a cbuffer and
// then extrapolate from there
//
// Emergent critter generation is extremely complicated. A mixture of pattern blends and known
// planetary conditions seems best, but that still produces enough unknowns that I should probably
// push it to the side for now and just focus on planet generation (/possibly/ with migration
// simulation and a small set of pre-built organisms)
//
// Tectonic textures should only be maintained for planetary patterns, not for every planet in the
// game at once; pattern weighting is used to blend altitudes for planets defined from multiple
// patterns at once
//
// Tectonic textures can be effectively represented as voronoi diagrams; there's a good
// description of procedural generation with voronoi diagrams available over here:
// http://pcg.wikidot.com/pcg-algorithm:voronoi-diagram
//
// Tectonic textures should be 512*512 RGBA 2D images; eventually they should be buildable inside
// the engine's (as yet unimplemented) development mode, but for now they can just be generated
// from a simple C# helper program
//
// Factorial scaling means that very few patterns are needed for a significant amount of variety;
// for example, just sixteen patterns scales to ~20 trillion possible combinations
//
// Avoid trying to implement the entire system above all at once; try to generate a single system
// containing a single planet first, then build up from there

// Scene contains the Galaxy + the main camera
// Galaxy contains an array of Systems
// Each System contains a (arbitrarily coloured
// for now) star and three planets
// Systems are assumed not to orbit the galactic centre
// Planets are assumed to orbit their central star
// Systems are associated with vector positions for
// "easy" (linear) access (should probably focus on
// improving that in the future)
//
// Per-pixel displacement can be inverted, then interpreted
// as relative displacement from the planetary surface downward;
// this negates the issue of deciding what origin to use for
// depth projection
//
//
// - Research areas -
// Segmented LOD (with ~18 nested cubes going from 1vx/m to 2^18vx/m);
// realizable with dynamic tesselation and multiple scene textures
//
// Cheap environmental modelling (climate, weather, critter distributions, etc.)
//
// Volumetric (texture-based) dynamics (displacement, torque, physics, animation, etc.)
//
// Physics for realistic planetary orbits/spins
//
// Some sort of algorithm for procedural mineral distributions (also some way to integrate
// those distributions for realistic planetary colors)
//
// Chemistry/Physics for varying star types (probably v. long term)
//
// Chemistry/Physics for more realistic stellar distributions within the galaxy
//
// Once segmented LOD is implemented, voxelized scene backgrounds (e.g. clouds, stellar features, etc.)
//

enum class AVAILABLE_PLANET_ARCHETYPES
{
	ARCHETYPE_0,
	NULL_ARCHETYPE
};

class Direct3D;
class Camera;
class Rasterizer;
class VoxelGrid;
class SystemSampler;
class PlanetSampler;
class Planet;
class Star;
class SceneIlluminator;
class PostProcessor;
class ScreenRect;
class RenderManager
{
	public:
		RenderManager(ID3D11ShaderResourceView* postProcessShaderResource);
		~RenderManager();

		// Sample continuous galactic data into the scene texture, render the
		// voxel grid before coloring it with data sampled from the scene texture,
		// then pass the rendered data through a series of post-processing filters
		// before presenting the result to the screen
		void Render(Camera* mainCamera, DirectX::XMVECTOR systemPos,
					Planet** planets, Star* star);

		// Retrieve a reference to the Direct3D handler class associated with [this]
		Direct3D* GetD3D();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Helper function, used to sample continuous galactic data at the current
		// camera position into a scene texture that can be applied to the voxel
		// grid during rendering
		void PreProcess(DirectX::XMVECTOR cameraPos, DirectX::XMVECTOR systemPos,
					    DirectX::XMVECTOR cameraViewVector,
					    Planet** planets, Star* star);

		// Helper function, used to render the voxel grid before coloring it with
		// data sampled from the scene texture during pre-processing (see above)
		void RenderScene(DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Helper function, used to perform post-processing and
		// draw the result to the screen
		void PostProcess(ScreenRect* screenRect,
						 DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Reference to the Direct3D handler class
		Direct3D* d3D;

		// Reference to the Direct3D device context
		ID3D11DeviceContext* d3dContext;

		// Voxel grid storage
		VoxelGrid* voxGrid;

		// Pipeline-shader storage
		Rasterizer* rasterizer;
		PostProcessor* postProcessor;

		// Compute-shader storage
		SystemSampler* systemSampler;
		PlanetSampler* planetSamplers[(byteUnsigned)AVAILABLE_PLANET_ARCHETYPES::NULL_ARCHETYPE];
		SceneIlluminator* sceneIlluminator;
};
