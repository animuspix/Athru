#pragma once

#include "Vector4.h"

class Matrix4
{
	public:
		Matrix4();
		Matrix4(float* vecA, float* vecB, float* vecC, float* vecD);
		~Matrix4();

		// Overload subscript operators for easy access

	private:
		float matrixData[4][4][4][4];
};

