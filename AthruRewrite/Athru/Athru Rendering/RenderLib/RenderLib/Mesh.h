#pragma once

#include "Vertex.h"
#include "d3d11.h"

class Mesh
{
	public:
		Mesh();
		~Mesh();

		void PassToGPU(ID3D11DeviceContext* deviceContext);

		// Get shader-friendly matrix transformation data
		DirectX::XMMATRIX GetTransform();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void* operator new[](size_t size);
		void operator delete(void* target);
		void operator delete[](void* target);

	protected:
		ID3D11Buffer *vertBuffer;
		ID3D11Buffer *indexBuffer;
};

