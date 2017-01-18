#pragma once

#include "Vector3.h"
#include "Matrix4.h"

class Cube
{
	public:
		Cube();
		Cube(Matrix4 suppliedVertices);
		~Cube();

	private:
		Matrix4 vertices;
};

