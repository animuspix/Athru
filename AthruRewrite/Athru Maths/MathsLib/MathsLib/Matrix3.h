#pragma once

class Matrix3
{
	public:
		Matrix3();
		Matrix3(float* vecA, float* vecB, float* vecC);
		~Matrix3();

		// Overload subscript operators for easy access

	private:
		float matrixData[3][3][3];
};

