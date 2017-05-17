#pragma once

#define VOXEL_INDEX_COUNT 36

#include "Mesh.h"

class VoxelGrid : public Mesh
{
	public:
		VoxelGrid(ID3D11Device* device);
		~VoxelGrid();

		// Pass the voxel grid onto the GPU for rendering
		void PassToGPU(ID3D11DeviceContext* deviceContext);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		struct VoxelInstance
		{
			VoxelInstance() {}
			VoxelInstance(DirectX::XMFLOAT3 suppliedUVW) :
							                uvw{ suppliedUVW.x,
							                	 suppliedUVW.y,
							                	 suppliedUVW.z } {};
			DirectX::XMFLOAT3 uvw;
		};

		// Per-instance voxel properties
		ID3D11Buffer* instanceBuffer;
};

