#pragma once

#include "Mesh.h"

class DemoMeshImport : public Mesh
{
	public:
		DemoMeshImport(ID3D11Device* d3dDevice);
		~DemoMeshImport();

		// .obj's store data in triangle lists rather
		// than strips, so take the quick + easy approach
		// (remember, this is a procedural engine! Actual
		// mesh import is only here because it needs to be
		// for an assignment) and overload [PassToGPU] to
		// format the incoming data as a triangle list
		// instead
		void PassToGPU(ID3D11DeviceContext * deviceContext);

		// Retrieve index count (needed to tell shaders how
		// many indices to render after passing [this] onto
		// the GPU)
		fourByteUnsigned GetIndexCount();

	private:
		fourByteUnsigned indexCount;
};

