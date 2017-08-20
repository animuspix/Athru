#pragma once

#include <chrono>
#include <d3d11.h>
#include <random>
#include "UtilityServiceCentre.h"

struct GPURand
{
	GPURand(ID3D11Device* device)
	{
		// Before we actually initialise our own RNG we'll need
		// to initialise the standard C++ RNG we're going to use
		// to seed it; to do that, we'll need to catch the current
		// time, so do that here
		std::chrono::steady_clock::time_point currTime = std::chrono::steady_clock::now();
		eightByteUnsigned currTimeNumeric = currTime.time_since_epoch().count();

		// We want our seeds to avoid coherence over a long period
		// (4 * GPGPUStuff::RAND_STATE_COUNT), so create a local
		// instance of the C++ Mersenne Twister random number
		// generator
		std::mt19937 cppRand;
		cppRand.seed((fourByteUnsigned)(currTimeNumeric & 0xFFFFFFFF00000000));

		// Use the Mersenne Twister to define each seed value used with the
		// GPU Xorshift random number generator
		fourByteUnsigned* gpuRandSeeds = new fourByteUnsigned[GPGPUStuff::RAND_STATE_COUNT];
		for (fourByteUnsigned i = 0; i < (GPGPUStuff::RAND_STATE_COUNT / 2); i += 1)
		{
			gpuRandSeeds[0] = cppRand();
		}

		// Package the generated seeds into a DirectX sub-resource so that we
		// can easily pass them along to the GPU
		D3D11_SUBRESOURCE_DATA seeds;
		seeds.pSysMem = gpuRandSeeds;
		seeds.SysMemPitch = 0;
		seeds.SysMemSlicePitch = 0;

		// Create a buffer + an unordered-access-view to keep the state of the
		// GPU RNG on the graphics card

		// Describe the state buffer
		D3D11_BUFFER_DESC gpuRandStateDesc;
		gpuRandStateDesc.Usage = D3D11_USAGE_DEFAULT;
		gpuRandStateDesc.ByteWidth = sizeof(fourByteUnsigned) * GPGPUStuff::RAND_STATE_COUNT;
		gpuRandStateDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		gpuRandStateDesc.CPUAccessFlags = 0;
		gpuRandStateDesc.MiscFlags = 0;
		gpuRandStateDesc.StructureByteStride = 0;

		// Construct the state buffer using the seeds + description
		// defined above
		HRESULT result;
		result = device->CreateBuffer(&gpuRandStateDesc, &seeds, &gpuRandState);
		assert(SUCCEEDED(result));

		// We've safely passed our RNG seeds along to the GPU, so
		// delete the CPU-side copies and banish their address to [nullptr]
		// over here
		delete[] gpuRandSeeds;
		gpuRandSeeds = nullptr;

		// Describe the view we'll use to access the state buffer
		// in the relevant shaders

		// Describe some basic properties associated with the state
		// buffer
		D3D11_BUFFER_UAV gpuRandStateBufferProps;
		gpuRandStateBufferProps.FirstElement = 0;
		gpuRandStateBufferProps.Flags = 0;
		gpuRandStateBufferProps.NumElements = GPGPUStuff::RAND_STATE_COUNT;

		// Describe the state buffer view itself
		D3D11_UNORDERED_ACCESS_VIEW_DESC gpuRandStateViewDesc;
		gpuRandStateViewDesc.Format = DXGI_FORMAT_R32_UINT;
		gpuRandStateViewDesc.Buffer = gpuRandStateBufferProps;
		gpuRandStateViewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

		// Construct the state buffer view using the description defined above
		result = device->CreateUnorderedAccessView(gpuRandState, &gpuRandStateViewDesc, &gpuRandStateView);
		assert(SUCCEEDED(result));
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
	ID3D11Buffer* gpuRandState;

	// Shader-friendly view of the GPU RNG state
	// buffer described above
	ID3D11UnorderedAccessView* gpuRandStateView;
};