#pragma once

#include <chrono>
#include <d3d11.h>
#include <random>
#include "UtilityServiceCentre.h"

struct GPURand
{
	GPURand(const Microsoft::WRL::ComPtr<ID3D11Device>& device)
	{
		// Before we actually initialise our own RNG we'll need
		// to initialise the standard C++ RNG we're going to use
		// to seed it; to do that, we'll need to catch the current
		// time, so do that here
		std::chrono::steady_clock::time_point currTime = std::chrono::steady_clock::now();
		eightByteUnsigned currTimeNumeric = currTime.time_since_epoch().count();

		// We want our seeds to avoid coherence over a long period
		// so create a local instance of the C++ Mersenne Twister
		// random number generator
		std::mt19937 cppRand;
		cppRand.seed((fourByteUnsigned)(currTimeNumeric & 0x00000000FFFFFFFF));

		// Use the Mersenne Twister to define each seed value used with the
		// GPU Xorshift random number generator
		fourByteUnsigned* gpuRandSeeds = new fourByteUnsigned[GPGPUStuff::NUM_RAND_SEEDS];
		for (fourByteUnsigned i = 0; i < GPGPUStuff::NUM_RAND_SEEDS; i += 1)
		{
			gpuRandSeeds[i] = cppRand();
		}

		// Package the generated seeds into a DirectX sub-resource so that we
		// can easily pass them along to the GPU
		D3D11_SUBRESOURCE_DATA seeds;
		seeds.pSysMem = gpuRandSeeds;
		seeds.SysMemPitch = 0;
		seeds.SysMemSlicePitch = 0;

		// Build the GPU RNG's state buffer
		DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
		fourByteUnsigned numSeeds = GPGPUStuff::NUM_RAND_SEEDS;
		GPGPUStuff::BuildRWBuffer<fourByteUnsigned>(device,
													gpuRandState,
													&seeds,
													gpuRandStateView,
													format,
													(D3D11_BUFFER_UAV_FLAG)0,
													numSeeds);

		// We've safely passed our RNG seeds along to the GPU, so
		// delete the CPU-side copies and banish their address to [nullptr]
		delete[] gpuRandSeeds;
		gpuRandSeeds = nullptr;
	}

	// Overload the standard allocation/de-allocation operators
	void* operator new(size_t size);
	void operator delete(void* target);

	// State buffer to improve GPU random number
	// generation, where each individual block of
	// 32-bit state is associated with an
	// instance of the 32-bit Xorshift RNG
	// described here:
	// https://en.wikipedia.org/wiki/Xorshift
	Microsoft::WRL::ComPtr<ID3D11Buffer> gpuRandState;

	// Shader-friendly view of the GPU RNG state
	// buffer described above
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> gpuRandStateView;
};