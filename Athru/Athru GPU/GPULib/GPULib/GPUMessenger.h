#pragma once

#include <d3d12.h>
#include <functional>
#include <thread>
#include "SceneFigure.h"
#include "AthruResrc.h"
#include "ComputePass.h"
#include <wrl\client.h>

class GPUMessenger
{
	public:
		GPUMessenger(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                     AthruGPU::GPUMemory& gpuMem);
		~GPUMessenger();

		// Upload per-system scene data to the GPU
        // Per-planet upload is unimplemented atm, but expected once I have planetary UVs ready
		// All bulk uploads will probably occur here so I can supply a reasonable amount of data to
		// the copy command-list
		void SysToGPU(SceneFigure::Figure* system);

		// Pass per-frame inputs along to the GPU
		void InputsToGPU(const DirectX::XMFLOAT4& sysOri,
                         const Camera* camera);

		// Expose interfaces to lodepng, the png encoding/decoding library used by Athru
		// Saved textures are expected LDR/4-channel; copies into readback memory should have occurred before
		// [SaveTexture(...)] is invoked (-> saves [GPUMessenger] having to maintain a separate command-list
		// for every possible texture export)
		// lodepng is distributed by Lode Vandevenne (thanks! :D) under the zlib license, which reads
		// as follows:
		//	Copyright(c) 2005 - 2018 Lode Vandevenne
		//
		//	This software is provided 'as-is', without any express or implied
		//	warranty.In no event will the authors be held liable for any damages
		//	arising from the use of this software.
		//
		//	Permission is granted to anyone to use this software for any purpose,
		//	including commercial applications, and to alter itand redistribute it
		//	freely, subject to the following restrictions :
		//
		//	1. The origin of this software must not be misrepresented; you must not
		//	claim that you wrote the original software.If you use this software
		//	in a product, an acknowledgment in the product documentation would be
		//	appreciated but is not required.
		//
		//	2. Altered source versions must be plainly marked as such, and must not be
		//	misrepresented as being the original software.
		//
		//	3. This notice may not be removed or altered from any source
		//	distribution.
		void SaveTexture(const char* file, u4Byte width, u4Byte height);
		void LoadTexture(const char* file, u4Byte width, u4Byte height, uByte** output);

		// Access data copied into the read-back buffer at a given offset, in a given format
		template<typename DataFmt>
		void DataFromGPU(u4Byte offs, u4Byte dataLen, DataFmt* readInto)
		{
			D3D12_RANGE range;
			range.Begin = offs;
			range.End = offs + dataLen;
			HRESULT hr = rdbkBuf.resrc->Map(0, &range, (void**)&gpuReadable);
			assert(SUCCEEDED(hr));
			memcpy(readInto, gpuReadable, dataLen);
			range.Begin = 0;
			range.End = 0;
			rdbkBuf.resrc->Unmap(0, &range);
		}

		// Acquire a const reference to the resource behind the readback buffer
		const Microsoft::WRL::ComPtr<ID3D12Resource>& AccessReadbackBuf();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Per-frame constant inputs
		struct GPUInput
		{
			DirectX::XMFLOAT4 tInfo; // Time info for each frame;
									 // delta-time in [x], current total time (seconds) in [y],
									 // frame count in [z], nothing in [w]
			DirectX::XMFLOAT4 systemOri; // Origin for the current star-system; useful for predicting figure
										 // positions during ray-marching/tracing (in [xyz], [w] is unused)
			DirectX::XMVECTOR cameraPos; // Camera position in [xyz], tracing epsilon value in [w]
			DirectX::XMMATRIX viewMat; // View matrix
			DirectX::XMMATRIX iViewMat; // Inverse view matrix
			DirectX::XMUINT4 resInfo; // Resolution info carrier; contains app resolution in [xy],
									  // AA sampling rate in [z], and display area in [w]
			DirectX::XMUINT4 tilingInfo; // Tiling info carrier; contains spatial tile counts in [x]/[y] and cumulative tile area in [z] ([w] is unused)
			DirectX::XMUINT4 tileInfo; // Per-tile info carrier; contains tile width/height in [x]/[y] and per-tile area in [z] ([w] is unused)
			DirectX::XMFLOAT4 denoiseInfo; // Baseline denoise filter width in [0].x, number of filter passes in [0].y; [z]/[w] are unused
			DirectX::XMFLOAT4 excess; // Padding out to 256-byte alignment
		};
        // + CPU-side mapping point
        GPUInput* gpuInput;

		// System buffer
        AthruGPU::AthruResrc<SceneFigure::Figure,
                             AthruGPU::RResrc<AthruGPU::Buffer>> sysBuf;
		// Length of the system buffer, in bytes
		static constexpr u4Byte sysBytes = SceneStuff::ALIGNED_PARAMETRIC_FIGURES_PER_SYSTEM * sizeof(SceneFigure);

		// CPU->GPU messaging buffer
        AthruGPU::AthruResrc<uByte,
							 AthruGPU::MessageBuffer> msgBuf;

		// GPU->CPU readback buffer
		AthruGPU::AthruResrc<uByte,
							 AthruGPU::ReadbkBuffer> rdbkBuf;
		// + CPU-side mapping point
		uByte* gpuReadable;

		// Core messaging command-list + shared messaging allocator
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> msgCmds;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> msgAlloc;

		// Specialty thread for file output (output wrapper declared as a free-function within
		// [GPUMessenger.cpp])
		std::thread fileWriter;

		// Intermediate CPU-side storage for file output
		// Defined as an array instead of a pointer so that storage can be contiguous with class data
		// within Athru's memory allocator
		uByte tmpFileStorage[AthruGPU::EXPECTED_SHARED_GPU_RDBK_MEM];
};