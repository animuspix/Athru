#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "GPUGlobals.h"
#include "SceneVertex.h"

template <typename VtType> // Restrict this to vertices defined by Athru
						   // when possible (i.e. with Concepts in C++20)
class Mesh
{
	public:
		Mesh() // Default constructor, literally just here to work around MSVC fiddliness;
			   // generated values are invalid and a call to the expanded constructor (see below)
			   // will be needed to create a useful pipeline mesh
		{
			vertBuffer = nullptr;
			indexBuffer = nullptr;
		}
		Mesh(VtType* vtSet,
			 u4Byte* ndxSet,
			 u4Byte numVts,
			 u4Byte numIndices,
			 const Microsoft::WRL::ComPtr<ID3D11Device>& device)
		{
			// Set up the description of the static vertex buffer
			D3D11_BUFFER_DESC vertBufferDesc;
			vertBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			vertBufferDesc.ByteWidth = sizeof(VtType) * numVts;
			vertBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vertBufferDesc.CPUAccessFlags = 0;
			vertBufferDesc.MiscFlags = 0;
			vertBufferDesc.StructureByteStride = 0;

			// Place a reference to the vertices in a subresource structure
			D3D11_SUBRESOURCE_DATA vertData;
			vertData.pSysMem = vtSet;
			vertData.SysMemPitch = 0;
			vertData.SysMemSlicePitch = 0;

			// Create the vertex buffer
			HRESULT result = device->CreateBuffer(&vertBufferDesc, &vertData, &vertBuffer);
			assert(SUCCEEDED(result));

			// Set up the description of the static index buffer
			D3D11_BUFFER_DESC indexBufferDesc;
			indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			indexBufferDesc.ByteWidth = sizeof(u4Byte) * numIndices;
			indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			indexBufferDesc.CPUAccessFlags = 0;
			indexBufferDesc.MiscFlags = 0;
			indexBufferDesc.StructureByteStride = 0;

			// Place a reference to the vertex indices in a subresource structure
			D3D11_SUBRESOURCE_DATA indexData;
			indexData.pSysMem = ndxSet;
			indexData.SysMemPitch = 0;
			indexData.SysMemSlicePitch = 0;

			// Create the index buffer
			result = device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);
			assert(SUCCEEDED(result));
		}

		~Mesh()
		{
			// Nullify the index buffer
			indexBuffer = nullptr;

			// Nullify the vertex buffer
			vertBuffer = nullptr;
		}

		void PassToGPU(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
		{
			// Set vertex buffer stride and offset.
			unsigned int stride = sizeof(VtType);
			unsigned int offset = 0;

			// Set the vertex buffer to active in the input assembler so it can be rendered
			context->IASetVertexBuffers(0, 1, vertBuffer.GetAddressOf(), &stride, &offset);

			// Pass the index buffer along to the GPU
			context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

			// Set the 2D primitive type used to create the Mesh; we're making simple
			// geometry, so triangular primitives make the most sense
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		}

		// Overload the standard allocation/de-allocation operators

		// Push constructions for this class through Athru's custom allocator
		void* operator new(size_t size)
		{
			StackAllocator* allocator = AthruCore::Utility::AccessMemory();
			return allocator->AlignedAlloc(size, (uByte)std::alignment_of<Mesh>(), false);
		}

		// We aren't expecting to use [delete], so overload it to do nothing
		void operator delete(void* target) { return; }
	protected:
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
};

