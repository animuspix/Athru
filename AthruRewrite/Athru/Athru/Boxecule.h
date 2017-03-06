#pragma once

#include <d3d11.h>
#include <directxmath.h>

#include "Material.h"
#include "Quaternion.h"

class Boxecule
{
	public:
		Boxecule(ID3D11Device* device);
		~Boxecule();

		void PassToGPU(ID3D11DeviceContext* deviceContext);
		Material GetMaterial();

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

		// Is [this] glowing?
		bool isLightSource;

		//Quaternion rotation;
		//DirectX::XMMATRIX transform;
		ID3D11Buffer *m_vertexBuffer;
		ID3D11Buffer *m_indexBuffer;
};

