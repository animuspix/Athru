#pragma once

#define BOXECULE_INDEX_COUNT 36

#include "Mesh.h"

class Boxecule : public Mesh
{
	public:
		Boxecule(ID3D11Device* device);
		~Boxecule();

		// The function used by [Mesh] to send data onto
		// the GPU wasn't implemented for instanced rendering,
		// so overload it here
		void PassToGPU(ID3D11DeviceContext* deviceContext);

	private:
		struct BoxeculeInstance
		{
			BoxeculeInstance() {}
			BoxeculeInstance(DirectX::XMFLOAT3 suppliedUVW) : 
							              uvw{ suppliedUVW.x, 
							              	   suppliedUVW.y, 
							              	   suppliedUVW.z } {};
			DirectX::XMFLOAT3 uvw;
		};

		// Boxecule instance properties
		ID3D11Buffer* instanceBuffer;
};

