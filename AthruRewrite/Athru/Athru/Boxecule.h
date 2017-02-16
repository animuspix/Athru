#pragma once

#include "d3d11.h"
#include "Vector4.h"
#include "SQTTransformer.h"
#include "Vector4.h"

#include "Material.h"

#define BOXECULE_VERT_COUNT 8

class Boxecule
{
	public:
		Boxecule(ID3D11Device* device, Material mat, SQTTransformer transformStuff);
		~Boxecule();

		// Render [this]
		void Render(ID3D11DeviceContext* deviceContext);

	private:
		// Struct representing a generic vertex
		struct Vertex
		{
			Vertex(float x, float y, float z,
				   float r, float g, float b, float a) : 
				   position{ x, y, z, 1 }, 
				   color{ r, g, b, a } {}
			float position[4];
			float color[4];
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

