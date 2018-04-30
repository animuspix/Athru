#pragma once

#include "SceneVertex.h"
#include "d3d11.h"
#include <wrl/client.h>

class Mesh
{
	public:
		Mesh();
		~Mesh();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void* operator new[](size_t size);
		void operator delete(void* target);
		void operator delete[](void* target);

	protected:
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
};

