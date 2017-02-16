#include "stdafx.h"
#include "Matrix4.h"

Matrix4::Matrix4()
{
	matrixData[0] = _mm_set_ps(1, 0, 0, 0);
	matrixData[1] = _mm_set_ps(0, 1, 0, 0);
	matrixData[2] = _mm_set_ps(0, 0, 1, 0);
	matrixData[3] = _mm_set_ps(0, 0, 0, 1);
}

Matrix4::Matrix4(float v0, float v1, float v2, float v3,
				 float v4, float v5, float v6, float v7,
				 float v8, float v9, float v10, float v11,
				 float v12, float v13, float v14, float v15)
{
	matrixData[0] = _mm_set_ps(v0, v1, v2, v3);
	matrixData[1] = _mm_set_ps(v4, v5, v6, v7);
	matrixData[2] = _mm_set_ps(v8, v9, v10, v11);
	matrixData[3] = _mm_set_ps(v12, v13, v14, v15);
}

Matrix4::~Matrix4()
{

}

void Matrix4::Transpose()
{
	// This is a weird side-effect-y macro, so it doesn't return the result of the
	// operation like most sse intrinsics do; instead, the changes are applied to
	// the data themselves and nothing is returned at all
	_MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);
}

float Matrix4::determinant(__m128* lower, __m128* upper, char luRowExchanges)
{
	float detInverter = (float)(-1 + (2 * (luRowExchanges % 2)));

	const float one = 1;
	const float scalarDetLower = lower[0].m128_f32[3] * lower[1].m128_f32[2] * lower[2].m128_f32[1] * lower[3].m128_f32[0];
	__m128 sseDetLower = _mm_broadcast_ss(&scalarDetLower);
	__m128 sseDetUpper = _mm_broadcast_ss(&one);
	__m128 sseLowerUpperProduct = _mm_mul_ps(sseDetLower, sseDetUpper);

	float lowerUpperScalarProduct = _mm_cvtss_f32(sseLowerUpperProduct);
	return detInverter * lowerUpperScalarProduct;
}

Matrix4 Matrix4::inverse()
{
	// Very uncertain about how to optimise this with SSE; consider asking AIE teachers

	// Use LU decomposition to find the inverse of a matrix; algorithm sourced
	// from gamedev.net:
	// https://www.gamedev.net/resources/_/technical/math-and-physics/matrix-inversion-using-lu-decomposition-r3637

	__m128 lower[4] = { 0, 0, 0, 0,
						0, 0, 0, 0,
						0, 0, 0, 0,
						0, 0, 0, 0  };

	__m128 upper[4] = { 1, 0, 0, 0,
						0, 1, 0, 0,
						0, 0, 1, 0,
						0, 0, 0, 1  };

	// Fill the lower matrix's leftmost column
	lower[0].m128_f32[3] = matrixData[0].m128_f32[3];
	lower[1].m128_f32[3] = matrixData[1].m128_f32[3];
	lower[2].m128_f32[3] = matrixData[2].m128_f32[3];
	lower[3].m128_f32[3] = matrixData[3].m128_f32[3];

	// Fill the upper matrix's topmost row (optimisable with SSE)
	__m128 sseLower0Copy = lower[0];
	__m128 sseLower00 = _mm_shuffle_ps(lower[0], sseLower0Copy, _MM_SHUFFLE(3, 3, 3, 3));
	upper[0] = _mm_div_ps(matrixData[0], sseLower00);

	// Cache repeating values to save some CPU time
	const float upper00 = upper[0].m128_f32[3];
	const float upper01 = upper[0].m128_f32[2];
	const float upper02 = upper[0].m128_f32[1];
	const float upper03 = upper[0].m128_f32[3];
	const float upper13 = matrixData[1].m128_f32[0] - (lower[1].m128_f32[3] * upper[0].m128_f32[0]) / upper00;

	const float lower10 = lower[1].m128_f32[3];
	const float lower20 = lower[2].m128_f32[3];
	const float lower30 = lower[3].m128_f32[3];
	const float lower21 = lower[2].m128_f32[2];
	const float lower31 = lower[3].m128_f32[2];

	// Generate lower matrix
	lower[1].m128_f32[2] = matrixData[1].m128_f32[2] - (lower10 * upper01);
	lower[2].m128_f32[1] = matrixData[2].m128_f32[1] - (lower20 * upper01);
	lower[2].m128_f32[2] = matrixData[2].m128_f32[2] - (lower20 * upper01) + (lower21 * upper[2].m128_f32[1]);
	lower[3].m128_f32[0] = matrixData[3].m128_f32[0] - (lower31 * upper13);
	lower[3].m128_f32[1] = matrixData[3].m128_f32[1] - (lower20 * upper01) + (lower21 * upper[2].m128_f32[1]);
	lower[3].m128_f32[2] = matrixData[3].m128_f32[2] - (lower30 * upper01);

	// Generate upper matrix
	upper[1].m128_f32[1] = matrixData[1].m128_f32[2] - (lower10 * upper02) / upper00;
	upper[1].m128_f32[0] = matrixData[2].m128_f32[1] - (lower10 * upper03) / upper00;
	upper[2].m128_f32[0] = matrixData[2].m128_f32[2] - (lower20 * upper03) + (lower[2].m128_f32[2] * upper13) +
													   (lower[2].m128_f32[1] * upper[2].m128_f32[0]) / upper00;

	// The upper matrix has 1's on the diagonal, so we don't need to generate as many values there
	// Generate pivot/permutation matrix (skipped atm because it doesn't _seem_ necessary for transformation matrices)
	// Calculate determinant (below); raise an error if the determinant is equal to zero since matrices with a determinant
	// equal to zero either have no inverse or multiple possible inverses
	assert(determinant(lower, upper, 0) != 0);

	// Find the inverse by using the above decomposition to solve A(A^-1) = I

	// Define a 2D float-array to represent [I]
	__m128 sseIDMatrix[4] = { 1, 0, 0, 0,
							  0, 1, 0, 0,
							  0, 0, 1, 0,
							  0, 0, 0, 1 };

	// Construct an empty matrix to represent [A^-1]
	__m128 sseInverseMatrix[4] = { 1, 0, 0, 0,
								   0, 1, 0, 0,
								   0, 0, 1, 0,
								   0, 0, 0, 1 };

	// Define an SSE matrix values to represent different instances of [d] ([d] is an
	// intermediate vector used to convert the decomposition created above into a
	// solution for the inverse matrix; see
	// https://www.gamedev.net/resources/_/technical/math-and-physics/matrix-inversion-using-lu-decomposition-r3637
	// for more detail)
	__m128 sseDMatrix[4] = { 0, 0, 0, 0,
							 0, 0, 0, 0,
							 0, 0, 0, 0,
							 0, 0, 0, 0 };

	// Cache repeating values (if they aren't cached already) to save a bit more CPU time
	const float lower11 = lower[1].m128_f32[2];
	const float lower21Local = lower21;
	const float lower22 = lower[2].m128_f32[1];
	const float lower32 = lower[3].m128_f32[1];
	const float lower33 = lower[3].m128_f32[0];

	const float upper10 = upper[1].m128_f32[3];
	const float upper12 = upper[1].m128_f32[1];
	const float upper23 = upper[2].m128_f32[0];

	const float sseDMatrix00 = sseDMatrix[0].m128_f32[3];
	const float sseDMatrix01 = sseDMatrix[0].m128_f32[2];

	// Begin constructing the inverse
	// Consider asking AIE teachers if this can be optimised any further
	__m128 sseLocalLower00 = sseLower00;

	const float D1Decr = (lower10 * sseDMatrix00) / lower11;
	__m128 sseD1Decr = _mm_broadcast_ss(&D1Decr);

	const float D2Decr = (lower20 * sseDMatrix00) + (lower21Local * sseDMatrix01) / lower22;
	__m128 sseD2Decr = _mm_broadcast_ss(&D2Decr);

	const float D3Decr = (lower30 * sseDMatrix00) + (lower31 * sseDMatrix01) +
													(lower32 * sseDMatrix[0].m128_f32[2]) / lower33;
	__m128 sseD3Decr = _mm_broadcast_ss(&D3Decr);

	sseDMatrix[0] = _mm_div_ps(sseIDMatrix[0], sseLocalLower00);
	sseDMatrix[1] = _mm_sub_ps(sseIDMatrix[1], sseD1Decr);
	sseDMatrix[2] = _mm_sub_ps(sseIDMatrix[2], sseD2Decr);
	sseDMatrix[3] = _mm_sub_ps(sseIDMatrix[3], sseD3Decr);

	__m128 sseUpper10 = _mm_broadcast_ss(&upper10);
	__m128 sseUpper12 = _mm_broadcast_ss(&upper12);
	__m128 sseUpper23 = _mm_broadcast_ss(&upper23);
	__m128 sseUpper01 = _mm_broadcast_ss(&upper01);
	__m128 sseUpper02 = _mm_broadcast_ss(&upper02);
	__m128 sseUpper03 = _mm_broadcast_ss(&upper03);

	__m128 sseUpper23xD3 = _mm_mul_ps(sseUpper23, sseDMatrix[3]);
	__m128 sseUpper12xD2 = _mm_mul_ps(sseUpper12, sseDMatrix[2]);
	__m128 sseUpper10xD3 = _mm_mul_ps(sseUpper10, sseDMatrix[3]);

	__m128 sseUpper01xD1 = _mm_mul_ps(sseUpper01, sseDMatrix[1]);
	__m128 sseUpper02xD2 = _mm_mul_ps(sseUpper02, sseDMatrix[2]);
	__m128 sseUpper03xD3 = _mm_mul_ps(sseUpper03, sseDMatrix[3]);

	__m128 sseSum0 = _mm_add_ps(sseUpper12xD2, sseUpper10xD3);
	__m128 sseSum1 = _mm_add_ps(sseUpper01xD1, sseUpper02xD2);
	__m128 sseSum2 = _mm_add_ps(sseSum1, sseUpper03xD3);

	// Generate the inverse values within the matrix
	sseInverseMatrix[3] = sseDMatrix[3];
	sseInverseMatrix[2] = _mm_sub_ps(sseDMatrix[2], sseUpper23xD3);
	sseInverseMatrix[1] = _mm_sub_ps(sseDMatrix[2], sseUpper23xD3);
	sseInverseMatrix[0] = _mm_sub_ps(sseDMatrix[2], sseUpper23xD3);

	// Store the SSE values inside a temporary 2D array
	float tempArray[4][4];
	_mm_store_ps(tempArray[0], sseInverseMatrix[0]);
	_mm_store_ps(tempArray[1], sseInverseMatrix[1]);
	_mm_store_ps(tempArray[2], sseInverseMatrix[2]);
	_mm_store_ps(tempArray[3], sseInverseMatrix[3]);

	// Read the generated values from the temporary array into a returnable [Matrix4]
	Matrix4 inverseMatrix = Matrix4(tempArray[0][0], tempArray[0][1], tempArray[0][2], tempArray[0][3],
									tempArray[1][0], tempArray[1][1], tempArray[1][2], tempArray[1][3],
									tempArray[2][0], tempArray[2][1], tempArray[2][2], tempArray[2][3],
									tempArray[3][0], tempArray[3][1], tempArray[3][2], tempArray[3][3]);

	// Return the resolved inverse matrix
	return inverseMatrix;
}

float& Matrix4::FetchValue(char row, char column)
{
	return matrixData[row].m128_f32[column];
}

const float Matrix4::FetchValue(char row, char column) const
{
	return matrixData[row].m128_f32[column];
}

const __m128& Matrix4::GetVector(char index)
{
	return matrixData[index];
}

void Matrix4::SetRotateX(float angleInRads)
{
	float angleSin = sin(angleInRads);
	float angleNegaSin = angleSin * -1;
	float angleCos = cos(angleInRads);

	matrixData[0] = _mm_set_ps(1, 0, 0, 0);
	matrixData[1] = _mm_set_ps(0, angleCos, angleSin, 0);
	matrixData[2] = _mm_set_ps(0, angleNegaSin, angleCos, 0);
	matrixData[3] = _mm_set_ps(0, 0, 0, 1);
}

void Matrix4::SetRotateY(float angleInRads)
{
	float angleSin = sin(angleInRads);
	float angleNegaSin = angleSin * -1;
	float angleCos = cos(angleInRads);

	matrixData[0] = _mm_set_ps(angleCos, 0, angleNegaSin, 0);
	matrixData[1] = _mm_set_ps(0, 1, 0, 0);
	matrixData[2] = _mm_set_ps(angleSin, 0, angleCos, 0);
	matrixData[3] = _mm_set_ps(0, 0, 0, 1);
}

void Matrix4::SetRotateZ(float angleInRads)
{
	float angleSin = sin(angleInRads);
	float angleNegaSin = angleSin * -1;
	float angleCos = cos(angleInRads);

	matrixData[0] = _mm_set_ps(angleCos, angleSin, 0, 0);
	matrixData[1] = _mm_set_ps(angleNegaSin, angleCos, 0, 0);
	matrixData[2] = _mm_set_ps(0, 0, 1, 0);
	matrixData[3] = _mm_set_ps(0, 0, 0, 1);
}

void Matrix4::SetTranslate(float xDisp, float yDisp, float zDisp)
{
	matrixData[0] = _mm_set_ps(0,       0,     0,   0);
	matrixData[1] = _mm_set_ps(0,       0,     0,   0);
	matrixData[2] = _mm_set_ps(0,       0,     0,   0);
	matrixData[3] = _mm_set_ps(xDisp, yDisp, zDisp, 1);
}

void Matrix4::SetScale(float xScale, float yScale, float zScale)
{
	matrixData[0] = _mm_set_ps(xScale, 0, 0, 0);
	matrixData[1] = _mm_set_ps(0, yScale, 0, 0);
	matrixData[2] = _mm_set_ps(0, 0, zScale, 0);
	matrixData[3] = _mm_set_ps(0, 0,   0,    1);
}

float Matrix4::sseSquash(__m128 sseVector)
{
	__m128 sseVectorCopy = sseVector;

	__m128 sseXXXXVector = _mm_shuffle_ps(sseVector, sseVectorCopy, _MM_SHUFFLE(3, 3, 3, 3));
	__m128 sseYYYYVector = _mm_shuffle_ps(sseVector, sseVectorCopy, _MM_SHUFFLE(2, 2, 2, 2));
	__m128 sseZZZZVector = _mm_shuffle_ps(sseVector, sseVectorCopy, _MM_SHUFFLE(1, 1, 1, 1));
	__m128 sseWWWWVector = _mm_shuffle_ps(sseVector, sseVectorCopy, _MM_SHUFFLE(0, 0, 0, 0));

	__m128 sseXPlusYVector = _mm_add_ps(sseXXXXVector, sseYYYYVector);
	__m128 sseSumVector = _mm_add_ps(sseXPlusYVector, sseWWWWVector);

	return _mm_cvtss_f32(sseSumVector);
}

Matrix4 Matrix4::operator*(const Matrix4& rhs)
{
	__m128 lhsRow0 = matrixData[0];
	__m128 lhsRow1 = matrixData[1];
	__m128 lhsRow2 = matrixData[2];
	__m128 lhsRow3 = matrixData[3];

	__m128 rhsCol0 = _mm_set_ps(rhs.FetchValue(0, 0),
								rhs.FetchValue(1, 0),
								rhs.FetchValue(2, 0),
								rhs.FetchValue(3, 0));

	__m128 rhsCol1 = _mm_set_ps(rhs.FetchValue(0, 1),
								rhs.FetchValue(1, 1),
								rhs.FetchValue(2, 1),
								rhs.FetchValue(3, 1));

	__m128 rhsCol2 = _mm_set_ps(rhs.FetchValue(0, 2),
								rhs.FetchValue(1, 2),
								rhs.FetchValue(2, 2),
								rhs.FetchValue(3, 2));

	__m128 rhsCol3 = _mm_set_ps(rhs.FetchValue(0, 3),
								rhs.FetchValue(1, 3),
								rhs.FetchValue(2, 3),
								rhs.FetchValue(3, 3));

	__m128 row0xCol0 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row0xCol1 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row0xCol2 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row0xCol3 = _mm_mul_ps(lhsRow0, rhsCol0);

	__m128 row1xCol0 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row1xCol1 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row1xCol2 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row1xCol3 = _mm_mul_ps(lhsRow0, rhsCol0);

	__m128 row2xCol0 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row2xCol1 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row2xCol2 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row2xCol3 = _mm_mul_ps(lhsRow0, rhsCol0);

	__m128 row3xCol0 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row3xCol1 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row3xCol2 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row3xCol3 = _mm_mul_ps(lhsRow0, rhsCol0);

	Matrix4 product = Matrix4(sseSquash(row0xCol0), sseSquash(row0xCol1), sseSquash(row0xCol2), sseSquash(row0xCol3),
							  sseSquash(row1xCol0), sseSquash(row1xCol1), sseSquash(row1xCol2), sseSquash(row1xCol3),
							  sseSquash(row2xCol0), sseSquash(row2xCol1), sseSquash(row2xCol2), sseSquash(row2xCol3),
							  sseSquash(row3xCol0), sseSquash(row3xCol1), sseSquash(row3xCol2), sseSquash(row3xCol3));
	return product;
}

void Matrix4::operator*=(const Matrix4& rhs)
{
	__m128 lhsRow0 = matrixData[0];
	__m128 lhsRow1 = matrixData[1];
	__m128 lhsRow2 = matrixData[2];
	__m128 lhsRow3 = matrixData[3];

	__m128 rhsCol0 = _mm_set_ps(rhs.FetchValue(0, 0),
								rhs.FetchValue(1, 0),
								rhs.FetchValue(2, 0),
								rhs.FetchValue(3, 0));

	__m128 rhsCol1 = _mm_set_ps(rhs.FetchValue(0, 1),
								rhs.FetchValue(1, 1),
								rhs.FetchValue(2, 1),
								rhs.FetchValue(3, 1));

	__m128 rhsCol2 = _mm_set_ps(rhs.FetchValue(0, 2),
								rhs.FetchValue(1, 2),
								rhs.FetchValue(2, 2),
								rhs.FetchValue(3, 2));

	__m128 rhsCol3 = _mm_set_ps(rhs.FetchValue(0, 3),
								rhs.FetchValue(1, 3),
								rhs.FetchValue(2, 3),
								rhs.FetchValue(3, 3));

	__m128 row0xCol0 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row0xCol1 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row0xCol2 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row0xCol3 = _mm_mul_ps(lhsRow0, rhsCol0);

	__m128 row1xCol0 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row1xCol1 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row1xCol2 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row1xCol3 = _mm_mul_ps(lhsRow0, rhsCol0);

	__m128 row2xCol0 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row2xCol1 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row2xCol2 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row2xCol3 = _mm_mul_ps(lhsRow0, rhsCol0);

	__m128 row3xCol0 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row3xCol1 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row3xCol2 = _mm_mul_ps(lhsRow0, rhsCol0);
	__m128 row3xCol3 = _mm_mul_ps(lhsRow0, rhsCol0);

	matrixData[0] = _mm_set_ps(sseSquash(row0xCol0), sseSquash(row0xCol1), sseSquash(row0xCol2), sseSquash(row0xCol3));
	matrixData[1] = _mm_set_ps(sseSquash(row1xCol0), sseSquash(row1xCol1), sseSquash(row1xCol2), sseSquash(row1xCol3));
	matrixData[2] = _mm_set_ps(sseSquash(row2xCol0), sseSquash(row2xCol1), sseSquash(row2xCol2), sseSquash(row2xCol3));
	matrixData[3] = _mm_set_ps(sseSquash(row3xCol0), sseSquash(row3xCol1), sseSquash(row3xCol2), sseSquash(row3xCol3));
}