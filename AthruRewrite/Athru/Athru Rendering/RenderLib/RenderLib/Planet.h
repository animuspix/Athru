#pragma once

#include <directxmath.h>
#include "Typedefs.h"
#include "RenderManager.h"

// Mineral-based planet colors are probably best, but minerals definitions are a whole
// separate area that I haven't really thought about yet; just use single colors per-planet
// for now

// Should really use polar offset/rotation instead of cartesian offsets, since orbits are
// dominated by radial distances and angles of approach rather than static X/Y/Z positions

class Planet
{
	public:
		Planet(float givenMass, float givenRadius, DirectX::XMFLOAT4 givenAvgColor,
			   DirectX::XMVECTOR offsetFromStar, DirectX::XMFLOAT3 rotation,
			   float givenArchetypeWeights[(byteUnsigned)AVAILABLE_PLANET_ARCHETYPES::NULL_ARCHETYPE]);
		~Planet();

		// Retrieve the amount of matter associated
		// with [this] (kilograms)
		float GetMass();

		// Retrieve the average radius of [this]
		// (meters)
		float GetRadius();

		// Retrieve the average surface color of [this]
		DirectX::XMFLOAT4 GetAvgColor();

		// Fetch a write-allowed reference to the current
		// position of [this] relative to the local star
		DirectX::XMVECTOR& FetchStellarOffset();

		// Retrieve the rotation associated with [this]
		DirectX::XMVECTOR GetRotationQuaternion();

		// Incrementally rotate [this] with the euler rotation
		// defined by [rotateBy]
		void Rotate(DirectX::XMFLOAT3 rotateBy);

		// Retrieve an array describing the influence of each
		// planetary archetype on [this]
		float* GetArchetypeWeights();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		float mass;
		float radius;
		DirectX::XMFLOAT4 avgColor;
		DirectX::XMVECTOR stellarOffset;
		DirectX::XMVECTOR quaternionRotation;
		DirectX::XMFLOAT3 eulerRotation;
		float archetypeWeights[(byteUnsigned)AVAILABLE_PLANET_ARCHETYPES::NULL_ARCHETYPE];
};
