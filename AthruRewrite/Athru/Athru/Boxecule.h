#pragma once

#include "d3d11.h"
#include "Vector4.h"
#include "SQTTransformer.h"

#include "Material.h"

class Boxecule
{
	public:
		Boxecule(ID3D11Device* device);
		~Boxecule();

		// Render [this]
		void Render(ID3D11DeviceContext* deviceContext);

	private:
		#define BOXECULE_VERT_COUNT 8

		struct Vertex
		{
			Vertex(float x, float y, float z) : position{ x, y, z, 1 } {}
			float position[4];
		};

		// Buffer containing the un-ordered vertices in [this]
		ID3D11Buffer* vertBuffer;

		// Buffer containing indices to each un-ordered vertex
		// in the [vertBuffer]
		ID3D11Buffer* indexBuffer;

		// Single property representing current rotation, 
		// translation, and scale for [this]
		SQTTransformer transformData;

		// Single property representing the visual 
		// (e.g. reflectiveness, color, etc.) and 
		// physical (e.g. compressive/shear strength, density, etc.)
		// properties of [this]
		Material material;
};

