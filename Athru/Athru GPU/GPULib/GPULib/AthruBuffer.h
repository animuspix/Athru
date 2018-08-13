#pragma once

#include "UtilityServiceCentre.h"

template <typename ContentType,
		  typename AthruBufType> // Restrict to defined Athru buffer types when possible... (see Concepts in C++20)
struct AthruBuffer
{
	public:
		AthruBuffer() {}; // Default constructor, literally just here to work around MSVC fiddliness;
						  // generated values are invalid and a call to the expanded constructor (see below)
						  // will be needed to create a useful buffer/resource-view
		AthruBuffer(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
					const D3D11_SUBRESOURCE_DATA* baseDataPttr,
					fourByteUnsigned bufLength = 1,
					DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN) // Allow optional view-format definition for GPU-viewable,
															   // non-structured buffers; default assumes unknown structured data
		{
			// Describe the buffer we're creating and storing at
			// [bufPttr]
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = bufUsage();
			bufferDesc.ByteWidth = sizeof(ContentType) * bufLength;
			bufferDesc.BindFlags = bufBindFlags();
			bufferDesc.CPUAccessFlags = bufCPUAccess();
			bufferDesc.MiscFlags = bufMiscFlags();
			bufferDesc.StructureByteStride = bufByteStride();

			// Construct the state buffer using the seeds + description
			// defined above
			HRESULT result = device->CreateBuffer(&bufferDesc,
												  baseDataPttr,
												  buf.GetAddressOf());
			assert(SUCCEEDED(result));

			// Only define resource views for non-constant non-staging buffers
			if constexpr (!(std::is_same<AthruBufType, GPGPUStuff::CBuffer>::value ||
						    std::is_same<AthruBufType, GPGPUStuff::StgBuffer>::value))
			{
				// Choose between unordered-access-view and shader-resource-view construction as appropriate
				if constexpr (std::is_base_of<GPGPUStuff::GPURWBuffer, AthruBufType>::value)
				{
					// Describe the the shader-friendly read/write resource view we'll
					// use to access the buffer during general-purpose graphics
					// processing
					D3D11_BUFFER_UAV viewDescA;
					viewDescA.FirstElement = 0;
					viewDescA.Flags = bufUAVFlags();
					viewDescA.NumElements = bufLength;

					D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescB;
					viewDescB.Format = viewFmt;
					viewDescB.Buffer = viewDescA;
					viewDescB.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

					// Construct a DirectX11 "view" over the data at [bufPttr]
					result = device->CreateUnorderedAccessView(buf.Get(), &viewDescB, resrcView.GetAddressOf());
					assert(SUCCEEDED(result));
				}
				else
				{
					// Describe the the shader-friendly readable resource view we'll
					// use to access the buffer during general-purpose graphics
					// processing
					D3D11_BUFFER_SRV viewDescA;
					viewDescA.FirstElement = 0;
					viewDescA.NumElements = bufLength;

					D3D11_SHADER_RESOURCE_VIEW_DESC viewDescB;
					viewDescB.Format = viewFmt;
					viewDescB.Buffer = viewDescA;
					viewDescB.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;

					// Construct a DirectX11 "view" over the data at [bufPttr]
					result = device->CreateShaderResourceView(buf.Get(), &viewDescB, resrcView.GetAddressOf());
					assert(SUCCEEDED(result));
				}
			}
		}
		Microsoft::WRL::ComPtr<ID3D11Buffer> buf; // Internal DX11 buffer, never becomes invalid so safe to expose globally
		auto view() // Indirect access function used to safely extract the resource view for non-constant, non-staging buffers
		{
			static_assert(!(std::is_same<decltype(resrcView), std::nullptr_t>::value),
						  "No defined views for constant/staging buffers!");
			return resrcView;
		}
		bool hasView() { return !(std::is_same<decltype(resrcView), std::nullptr_t>::value); } // Return whether or not [this] has a defined
																							   // resource view
	private:
		typedef typename std::conditional<std::is_base_of<GPGPUStuff::GPURWBuffer,
							     						  AthruBufType>::value,
										  ID3D11UnorderedAccessView,
										  ID3D11ShaderResourceView>::type innerViewType;
		typedef typename std::conditional<std::is_same<AthruBufType, GPGPUStuff::CBuffer>::value || std::is_same<AthruBufType, GPGPUStuff::StgBuffer>::value,
										  std::nullptr_t,
										  Microsoft::WRL::ComPtr<innerViewType>>::type outerViewType;
		outerViewType resrcView; // Internal DX11 resource view, becomes invalid for
								 // constant/staging buffers (neither have separate
								 // resource views) so restrict access to an indirect
								 // helper function instead of using a public member
								 // value
		D3D11_USAGE bufUsage() // Small function returning appropriate usage for different buffer types
		{
			if constexpr (std::is_base_of<GPGPUStuff::GPURWBuffer, AthruBufType>::value)
			{
				return D3D11_USAGE_DEFAULT;
			}
			else if constexpr (std::is_same<AthruBufType, GPGPUStuff::CBuffer>::value)
			{
				return D3D11_USAGE_DYNAMIC;
			}
			else if constexpr (std::is_same<AthruBufType, GPGPUStuff::GPURBuffer>::value)
			{
				return D3D11_USAGE_IMMUTABLE;
			}
			else
			{
				return D3D11_USAGE_STAGING;
			}
		}
		fourByteUnsigned bufBindFlags() // Small function returning appropriate binding flags for different buffer types
		{
			if constexpr (std::is_base_of<GPGPUStuff::GPURWBuffer, AthruBufType>::value)
			{
				return D3D11_BIND_UNORDERED_ACCESS;
			}
			else if constexpr (std::is_same<AthruBufType, GPGPUStuff::StgBuffer>::value)
			{
				return 0;
			}
			else if constexpr (std::is_same<AthruBufType, GPGPUStuff::CBuffer>::value)
			{
				return D3D11_BIND_CONSTANT_BUFFER;
			}
			else
			{
				return D3D11_BIND_SHADER_RESOURCE;
			}
		}
		fourByteUnsigned bufCPUAccess() // Small function returning appropriate CPU access for different buffer types
		{
			if constexpr ((std::is_same<AthruBufType, GPGPUStuff::StgBuffer>::value))
			{
				return D3D11_CPU_ACCESS_READ;
			}
			else if constexpr (std::is_same<AthruBufType, GPGPUStuff::CBuffer>::value)
			{
				return D3D11_CPU_ACCESS_WRITE;
			}
			else
			{
				return 0;
			}
		}
		fourByteUnsigned bufMiscFlags() // Small function returning appropriate miscellaneous flags for different buffer types
		{
			if constexpr ((std::is_class<ContentType>::value || std::is_same<AthruBufType, GPGPUStuff::AppBuffer>::value) &&
						  (!(std::is_same<decltype(resrcView), std::nullptr_t>::value)))
			{
				return D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			}
			else
			{
				return 0;
			}
		}
		fourByteUnsigned bufByteStride() // Small function returning structure byte stride (when appropriate)
		{
			return sizeof(ContentType) * ((std::is_class<ContentType>::value || std::is_same<AthruBufType, GPGPUStuff::AppBuffer>::value) &&
										  !(std::is_same<AthruBufType, GPGPUStuff::CBuffer>::value)); // Structure-byte-stride is zeroed
																									  // for unstructured and constant
																									  // buffers
		}
		fourByteUnsigned bufUAVFlags()
		{
			if constexpr (std::is_same<AthruBufType, GPGPUStuff::AppBuffer>::value)
			{
				return D3D11_BUFFER_UAV_FLAG_APPEND;
			}
			else
			{
				return 0;
			}
		}
};