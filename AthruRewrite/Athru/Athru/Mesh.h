#pragma once

#include "Material.h"
#include "Vertex.h"
#include "SQT.h"

class Mesh
{
	public:
		Mesh();
		~Mesh();

		void PassToGPU(ID3D11DeviceContext* deviceContext);
		void SetMaterial(Material& freshMaterial);
		Material& GetMaterial();

		// Get write-allowed raw transformation data
		SQT& FetchTransformations();

		// Get shader-friendly matrix transformation data
		DirectX::XMMATRIX GetTransform();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void* operator new[](size_t size);
		void operator delete(void* target);
		void operator delete[](void* target);

	protected:
		// The material associated with [this]
		// Each material contains sound data, a color, and a bunch of
		// shader types that make it easy to process inside the render
		// manager
		Material material;
		SQT transformations;
		ID3D11Buffer *vertBuffer;
		ID3D11Buffer *indexBuffer;
};

