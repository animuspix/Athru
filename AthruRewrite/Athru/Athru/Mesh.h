#pragma once

#include <d3d11.h>
#include "Vertex.h"
#include "SQT.h"
#include "Material.h"

class Mesh
{
	public:
		Mesh() {}
		Mesh(Material meshMaterial);
		~Mesh();

		void PassToGPU(ID3D11DeviceContext* deviceContext);
		void SetMaterial(Material& freshMaterial);
		Material& GetMaterial();

		// Get write-allowed raw transformation data
		SQT& FetchTransformations();

		// Retrieve data for the vertices associated with [this]
		Vertex* GetVerts();

		// Update the vertices associated with [this] and map
		// the updated data to the relevant buffer
		void SetVerts();

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
		Vertex* vertices;
		SQT transformations;
		ID3D11Buffer *vertBuffer;
		ID3D11Buffer *indexBuffer;
};

