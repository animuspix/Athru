#pragma once

#include <d3d11.h>
#include <directxmath.h>

class Triangle
{
	public:
		Triangle(ID3D11Device* device);
		~Triangle();

		void Render(ID3D11DeviceContext* deviceContext);

	private:
		struct Vertex
		{
			DirectX::XMFLOAT4 position;
			DirectX::XMFLOAT4 color;
		};

		ID3D11Buffer *m_vertexBuffer;
		ID3D11Buffer *m_indexBuffer;
};

