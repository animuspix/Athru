#pragma once

#ifndef __m128
#include "intrin.h"
#endif

// The 4-by-4 version of the identity matrix
// (https://en.wikipedia.org/wiki/Identity_matrix)
#define MATRIX_4_ID Matrix4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)

// The empty 4-by-4 matrix
#define MATRIX_4_EMPTY Matrix4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

// The number of components in a 4-by-4 matrix
#define MATRIX_4_AREA 16

class Matrix4
{
	public:
		Matrix4();
		Matrix4(float v0, float v1, float v2, float v3,
				float v4, float v5, float v6, float v7,
				float v8, float v9, float v10, float v11,
				float v12, float v13, float v14, float v15);
		~Matrix4();

		// Transpose [this] (https://en.wikipedia.org/wiki/Transpose)
		void Transpose();

		// Find the inverse of [this] (http://www.mathwords.com/i/inverse_of_a_matrix.htm)
		// (https://en.wikipedia.org/wiki/Invertible_matrix)
		Matrix4 inverse();

		// Fetch a reference to a value with the given column/row indices
		// (readable and writable, expects a non-const caller)
		float& FetchValue(char row, char column);

		// Fetch a reference to a value with the given column/row indices
		// (readable and writable, expects a const caller)
		const float FetchValue(char row, char column) const;

		// Return individual vectors from [this] as packed SSE values
		const __m128& GetVector(char index);

		// Convert [this] to a rotation matrix about x
		void SetRotateX(float angleInRads);

		// Convert [this] to a rotation matrix about y
		void SetRotateY(float angleInRads);

		// Convert [this] to a rotation matrix about z
		void SetRotateZ(float angleInRads);

		// Convert [this] into a translation matrix
		// "Disp" is short for "Displacement"
		void SetTranslate(float xDisp, float yDisp, float zDisp);

		// Convert [this] into a scaling matrix
		// Applying a scaling matrix to a vector multiplies every
		// axis by the given scaling information; when you use this,
		// think of the scaling function as making something so many
		// "times" bigger rather than incrementing it's size along
		// every axis by a given number of units
		void SetScale(float xScale, float yScale, float zScale);

		// Overloads for matrix multiplication
		Matrix4 operator*(const Matrix4& rhs);
		void operator*=(const Matrix4& rhs);

	private:
		__m128 matrixData[4];

		// Find the determinant of [this] (https://en.wikipedia.org/wiki/Determinant)
		// Kept [private] because it's really only useful for calculating the inverse
		// matrix
		float determinant(__m128* lower, __m128* upper, char luRowExchanges);

		// "Squash" _m128 values into a single scalar through shuffles and SSE additions
		// Kept [private] because it's only used for matrix multiplication in this context
		// and I'd rather not make it globally available
		float sseSquash(__m128 sseVector);
};
