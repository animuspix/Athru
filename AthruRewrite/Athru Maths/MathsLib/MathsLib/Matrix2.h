#pragma once

class Matrix2
{
	public:
		Matrix2();
		Matrix2(float* vecA, float* vecB);
		~Matrix2();

		// Overload subscript operators for easy access

	private:
		float matrixData[2][2];
};

