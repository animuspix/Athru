#pragma once

#include <d3d11.h>
#include <directxmath.h>

#include "Quaternion.h"

class Boxecule
{
	public:
		Boxecule(ID3D11Device* device);
		~Boxecule();

		void PassToGPU(ID3D11DeviceContext* deviceContext);

	private:
		struct Vertex
		{
			Vertex() {};
			Vertex(float x, float y, float z,
				   float r, float g, float b, float a) :
				   position{x, y, z, 1 },
				   color{r, g, b, a} {}

			DirectX::XMFLOAT4 position;
			DirectX::XMFLOAT4 color;
		};

		//Quaternion rotation;
		//DirectX::XMMATRIX transform;
		ID3D11Buffer *m_vertexBuffer;
		ID3D11Buffer *m_indexBuffer;
};

