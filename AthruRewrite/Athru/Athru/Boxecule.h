#pragma once

#include <d3d11.h>
#include <directxmath.h>
#include "SQT.h"
#include "Material.h"

#define BOXECULE_INDEX_COUNT 36

class Boxecule
{
	public:
		Boxecule() {}
		Boxecule(Material boxeculeMaterial);
		~Boxecule();

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

	private:
		struct Vertex
		{
			Vertex() {};
			Vertex(float x, float y, float z,
				   float* baseColor) :
				   position{ x, y, z, 1 },
				   color{ baseColor[0], baseColor[1], baseColor[2], baseColor[3] } {}

			DirectX::XMFLOAT4 position;
			DirectX::XMFLOAT4 color;
		};

		// The material associated with [this]
		// Each material contains sound data, a color, and an array of
		// shader types that make it easy to sort within the render
		// queue and display in parallel with as many other objects
		// using the same shaders as possible
		Material material;

		SQT transformations;
		ID3D11Buffer *m_vertexBuffer;
		ID3D11Buffer *m_indexBuffer;
};

