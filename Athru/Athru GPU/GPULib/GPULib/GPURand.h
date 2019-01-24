#pragma once

#include <chrono>
#include <d3d11.h>
#include <random>
#include "GPUGlobals.h"
#include "AthruBuffer.h"
#include "PhiloStrm.h"
#include "UtilityServiceCentre.h"

struct GPURand
{
	GPURand(const Microsoft::WRL::ComPtr<ID3D11Device>& device)
	{
		// Build the GPU RNG's state buffer
		// No initial data! :O
		// Too finicky to pass starting seeds along to Athru's buffer interface, so we'll
		// just seed with GPU hashes instead :P
		u4Byte numSeeds = AthruGPU::NUM_RAND_SEEDS;
		gpuRandState = AthruGPU::AthruBuffer<PhiloStrm, AthruGPU::GPURWBuffer>(device,
																			   nullptr,
																			   AthruGPU::NUM_RAND_SEEDS);
	}
	~GPURand(){}

	// Overload the standard allocation/de-allocation operators
	void* operator new(size_t size);
	void operator delete(void* target);

	// State buffer to improve GPU random number
	// generation, where each individual block of
	// 32-bit state is associated with an
	// instance of the 32-bit Xorshift RNG
	// described here:
	// https://en.wikipedia.org/wiki/Xorshift
	AthruGPU::AthruBuffer<PhiloStrm, AthruGPU::GPURWBuffer> gpuRandState;
};