#pragma once

#define RECT_INDEX_COUNT 6

#include <directxmath.h>
#include "Material.h"
#include "SQT.h"

class AthruRect
{
	public:
		AthruRect() {}
		AthruRect(Material rectMaterial,
				  float baseWidth, float baseHeight);
		~AthruRect();

		void PassToGPU(ID3D11DeviceContext* deviceContext);
		void SetMaterial(Material& freshMaterial);
		Material& GetMaterial();

		SQT& FetchTransformations();
		DirectX::XMMATRIX GetTransform();

		void* operator new(size_t size);
		void operator delete(void * target);

	private:
		// The material associated with [this]
		// Each material contains sound data, a color, and an array of
		// shader types that make it easy to process inside the render
		// manager
		Material material;

		SQT transformations;
		ID3D11Buffer *vertBuffer;
		ID3D11Buffer *indexBuffer;
};

