#pragma once

#include <chrono>
#include <d3d12.h>
#include <random>
#include "GPUGlobals.h"
#include "AthruResrc.h"
#include "PhiloStrm.h"
#include "GPUMemory.h"
#include "UtilityServiceCentre.h"

struct GPURand
{
	GPURand(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
			AthruGPU::GPUMemory& gpuMem)
	{
		// Build the path-tracing RNG's state buffer
        // (unseeded from CPU bc we can synthesize seeds on GPU with some hash functions anyways, and seeding
        // on the GPU lets us avoid copying data onto the upload heap or having to think about efficient
        // initialization strategies for each stream at startup)
		ptRandState.InitBuf(device, gpuMem, AthruGPU::NUM_RAND_PT_SEEDS);
	}
	~GPURand(){}

	// Overload the standard allocation/de-allocation operators
	void* operator new(size_t size);
	void operator delete(void* target);

	// Path-tracing RNG state buffer; each chunk of state is a separate Philox stream
    // (see documentation in [PhiloStrm.h])
	AthruGPU::AthruResrc<PhiloStrm,
						 AthruGPU::RWResrc<AthruGPU::Buffer>,
						 AthruGPU::RESRC_COPY_STATES::NUL,
						 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> ptRandState;
};