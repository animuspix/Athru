#pragma once

#include "AppGlobals.h"
#include <functional>
#include <tuple>

namespace AthruGPU
{
	// Supported buffer types in Athru
	struct CBuffer {}; // Constant buffer, read/writable from the CPU/GPU
	struct GPURWBuffer {}; // Generic UAV buffer, read/writable from the GPU but not the CPU
	struct AppBuffer : GPURWBuffer {}; // Generic UAV buffer with the same read/write qualities as [GPURWBuffer], but behaves
									   // like a parallel stack instead of an array (insertion with [Append()], removal with
									   // [Consume()])
	struct WLimitedBuffer : GPURWBuffer {}; // Same behaviour as GPURWBuffer, GPU writability can be toggled on/off
	struct GPURBuffer {}; // Generic SRV buffer, readable but not writable from the GPU, inaccessible from the CPU
	struct StrmBuffer {}; // Streaming buffer with CPU-write permissions but not CPU-read; GPU read-only
	struct StgBuffer {}; // Staging buffer, used to copy GPU-only information across to the CPU (useful for e.g.
						 // extracting structure counts from append/consume buffers)
	struct CtrBuffer : GPURWBuffer {}; // Counter buffer used to streamline compute-shader dispatch; contains at least one set of three elements
									   // representing a dispatch vector

	// Typed Athru-specific buffer class
	template <typename ContentType,
			  typename AthruBufType> // Restrict to defined Athru buffer types when possible... (see Concepts in C++20)
	struct AthruBuffer
	{
		public:
			AthruBuffer() {}; // Empty default constructor, allows uninitialized buffers
			AthruBuffer(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
						const void* baseDataPttr,
						u4Byte bufLength = 1,
						DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN) // Allow optional view-format definition for GPU-viewable,
																   // non-structured buffers; default assumes unknown structured data
			{
				// Describe the local buffer
				D3D11_BUFFER_DESC bufferDesc;
				bufferDesc.Usage = bufUsage();
				bufferDesc.ByteWidth = sizeof(ContentType) * bufLength;
				bufferDesc.BindFlags = bufBindFlags();
				bufferDesc.CPUAccessFlags = bufCPUAccess();
				bufferDesc.MiscFlags = bufMiscFlags();
				bufferDesc.StructureByteStride = bufByteStride();

				// Format starting data inside a [D3D11_SUBRESOURCE]
				D3D11_SUBRESOURCE_DATA baseData;
				baseData.pSysMem = baseDataPttr;
				baseData.SysMemPitch = 0;
				baseData.SysMemSlicePitch = 0;

				// Subresource pointers must be [nullptr] if [pSysMem] is undefined,
				// so create an interface to safely remap those cases here
				const D3D11_SUBRESOURCE_DATA* safeBaseData[2] = { nullptr,
																  &baseData };

				// Construct the local buffer using the starting data + description
				// defined above
				HRESULT result = device->CreateBuffer(&bufferDesc,
													  safeBaseData[baseDataPttr != nullptr],
													  buf.GetAddressOf());
				assert(SUCCEEDED(result));

				// Only define resource views for non-constant non-staging buffers
				if constexpr (!(std::is_same<AthruBufType, AthruGPU::CBuffer>::value ||
							    std::is_same<AthruBufType, AthruGPU::StgBuffer>::value))
				{
					// Choose between unordered-access-view and shader-resource-view construction as appropriate
					if constexpr (std::is_base_of<AthruGPU::GPURWBuffer, AthruBufType>::value)
					{
						// Generate UAV descriptions, construct UAV
						D3D11_BUFFER_UAV viewDescA;
						viewDescA.FirstElement = 0;
						viewDescA.Flags = bufUAVFlags();
						viewDescA.NumElements = bufLength;

						D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescB;
						viewDescB.Format = viewFmt;
						viewDescB.Buffer = viewDescA;
						viewDescB.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
						result = device->CreateUnorderedAccessView(buf.Get(), &viewDescB, resrcView.GetAddressOf());
						assert(SUCCEEDED(result));

						// Generate read-only resource view for write-limited buffers
						if constexpr (std::is_same<AthruBufType, AthruGPU::WLimitedBuffer>::value) { CreateCastingSRV(device, bufLength,
																														viewFmt, result); }
					}
					else
					{
						CreateCoreSRV(device, bufLength, viewFmt, result);
					}
				}
			}
			Microsoft::WRL::ComPtr<ID3D11Buffer> buf; // Internal DX11 buffer, never becomes invalid so safe to expose globally

			// Indirect access function used to safely extract the resource view for non-constant, non-staging buffers
			const auto& view()
			{
				static_assert(!(std::is_same<decltype(resrcView), std::nullptr_t>::value),
							  "No defined views for constant/staging buffers");
				static_assert(!(std::is_same<AthruBufType, AthruGPU::WLimitedBuffer>::value),
							  "Write-limited buffers must specify view read/write view type by calling [asReadOnly] or [asRW]");
				return resrcView;
			}

			// Altenative to [view] specialized for write-limited buffers
			// Would prefer to only make this available when [AthruBufType] is write-limited, but [std::enable_if] is unavailable
			// on MSVC and template hacks cause lots of side-effects; am expecting to maybe be able to do that with [std::function<...>]
			// (having concepted return types) after C++20 is released
			const auto& rwView()
			{
				static_assert(std::is_same<AthruBufType, AthruGPU::WLimitedBuffer>::value,
							  "[rwView] is intended to specifically return a read/write-allowed view for write-limited buffers;\
							   other buffer types should prefer [view()]");
				return resrcView;
			}

			// Alternative to [view] specialized for write-limited buffers
			// Would prefer to only make this available when [AthruBufType] is write-limited, but  template hacks cause too many side-effects for a stable implementation;
			// am expecting to maybe be able to do that with [std::function<...>] after C++20 is released
			const auto& rView()
			{
				static_assert(std::is_same<AthruBufType, AthruGPU::WLimitedBuffer>::value,
							  "Only write-limited buffers can be optionally read as read-only;\ndefault read-only \
							   types (such as constant-buffers and [GPURBuffer]s should be accessed through [view])");
				return castResrcView;
			}

			// Return whether or not [this] has a defined
			// resource view
			bool hasView() { return !(std::is_same<decltype(resrcView), std::nullptr_t>::value); }

		private:
			typedef typename std::conditional<std::is_base_of<AthruGPU::GPURWBuffer,
								     						  AthruBufType>::value,
											  ID3D11UnorderedAccessView,
											  ID3D11ShaderResourceView>::type innerViewType;
			typedef typename std::conditional<std::is_same<AthruBufType, AthruGPU::CBuffer>::value || std::is_same<AthruBufType, AthruGPU::StgBuffer>::value,
											  std::nullptr_t,
											  Microsoft::WRL::ComPtr<innerViewType>>::type outerViewType;
			outerViewType resrcView; // Internal DX11 resource view, becomes invalid for
									 // constant/staging buffers (neither have separate
									 // resource views) so restrict access to an indirect
									 // helper function instead of using a public member
									 // value
			typedef typename std::conditional<std::is_same<AthruBufType, AthruGPU::WLimitedBuffer>::value,
											  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>,
											  std::nullptr_t>::type castingOuterType;
			castingOuterType castResrcView; // Secondary resource view, returned from [view(...)] (instead of [resrcView])
											// when [asReadOnly] is [true]

			D3D11_USAGE bufUsage() // Small function returning appropriate usage for different buffer types
			{
				if constexpr (std::is_base_of<AthruGPU::GPURWBuffer, AthruBufType>::value)
				{
					return D3D11_USAGE_DEFAULT;
				}
				else if constexpr (std::is_same<AthruBufType, AthruGPU::CBuffer>::value ||
								   std::is_same<AthruBufType, AthruGPU::StrmBuffer>::value)
				{
					return D3D11_USAGE_DYNAMIC;
				}
				else if constexpr (std::is_same<AthruBufType, AthruGPU::GPURBuffer>::value)
				{
					return D3D11_USAGE_IMMUTABLE;
				}
				else
				{
					return D3D11_USAGE_STAGING;
				}
			}
			u4Byte bufBindFlags() // Small function returning appropriate binding flags for different buffer types
			{
				if constexpr (std::is_base_of<AthruGPU::GPURWBuffer, AthruBufType>::value)
				{
					if constexpr (std::is_same<AthruBufType, AthruGPU::WLimitedBuffer>::value) { return D3D11_BIND_UNORDERED_ACCESS |
																										  D3D11_BIND_SHADER_RESOURCE; }
					else { return D3D11_BIND_UNORDERED_ACCESS; }
				}
				else if constexpr (std::is_same<AthruBufType, AthruGPU::StgBuffer>::value)
				{
					return 0;
				}
				else if constexpr (std::is_same<AthruBufType, AthruGPU::CBuffer>::value)
				{
					return D3D11_BIND_CONSTANT_BUFFER;
				}
				else
				{
					return D3D11_BIND_SHADER_RESOURCE;
				}
			}
			u4Byte bufCPUAccess() // Small function returning appropriate CPU access for different buffer types
			{
				if constexpr ((std::is_same<AthruBufType, AthruGPU::StgBuffer>::value))
				{
					return D3D11_CPU_ACCESS_READ;
				}
				else if constexpr (std::is_same<AthruBufType, AthruGPU::CBuffer>::value ||
								   std::is_same<AthruBufType, AthruGPU::StrmBuffer>::value)
				{
					return D3D11_CPU_ACCESS_WRITE;
				}
				else
				{
					return 0;
				}
			}
			u4Byte bufMiscFlags() // Small function returning appropriate miscellaneous flags for different buffer types
			{
				if constexpr ((std::is_class<ContentType>::value || std::is_same<AthruBufType, AthruGPU::AppBuffer>::value) &&
							  (!std::is_same<AthruBufType, AthruGPU::CBuffer>::value))
				{
					return D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				}
				else if constexpr (std::is_same<AthruBufType, AthruGPU::CtrBuffer>::value)
				{
					return D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
				}
				else
				{
					return 0;
				}
			}
			u4Byte bufByteStride() // Small function returning structure byte stride (when appropriate)
			{
				return sizeof(ContentType) * ((std::is_class<ContentType>::value || std::is_same<AthruBufType, AthruGPU::AppBuffer>::value) &&
											  !(std::is_same<AthruBufType, AthruGPU::CBuffer>::value)); // Structure-byte-stride is zeroed
																										  // for unstructured and constant
																										  // buffers
			}
			u4Byte bufUAVFlags()
			{
				if constexpr (std::is_same<AthruBufType, AthruGPU::AppBuffer>::value)
				{
					return D3D11_BUFFER_UAV_FLAG_APPEND;
				}
				else
				{
					return 0;
				}
			}

			void CreateCoreSRV(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
							   const u4Byte& bufLength, const DXGI_FORMAT& viewFmt, HRESULT& result)
			{
				// Generate SRV descriptions, construct SRV
				D3D11_BUFFER_SRV viewDescA;
				viewDescA.FirstElement = 0;
				viewDescA.NumElements = bufLength;
				D3D11_SHADER_RESOURCE_VIEW_DESC viewDescB;
				viewDescB.Format = viewFmt;
				viewDescB.Buffer = viewDescA;
				viewDescB.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
				result = device->CreateShaderResourceView(buf.Get(), &viewDescB, resrcView.GetAddressOf());
				assert(SUCCEEDED(result));
			}

			void CreateCastingSRV(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
								  const u4Byte& bufLength, const DXGI_FORMAT& viewFmt, HRESULT& result)
			{
				// Generate SRV descriptions, construct SRV
				D3D11_BUFFER_SRV viewDescA;
				viewDescA.FirstElement = 0;
				viewDescA.NumElements = bufLength;
				D3D11_SHADER_RESOURCE_VIEW_DESC viewDescB;
				viewDescB.Format = viewFmt;
				viewDescB.Buffer = viewDescA;
				viewDescB.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
				result = device->CreateShaderResourceView(buf.Get(), &viewDescB, castResrcView.GetAddressOf());
				assert(SUCCEEDED(result));
			}
	};
}