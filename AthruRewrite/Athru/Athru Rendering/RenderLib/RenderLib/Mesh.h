#pragma once

#include "SceneVertex.h"
#include "d3d11.h"

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
		ID3D11Buffer *vertBuffer;
		ID3D11Buffer *indexBuffer;
};

