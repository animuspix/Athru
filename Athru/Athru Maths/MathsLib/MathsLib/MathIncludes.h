#pragma once

#include <cmath>
#include <intrin.h>

#define PI (float)(((4 * atan(0.2)) - atan(1 / 239)) * 4)

// "Squash" _m128 values into a single scalar through shuffles and SSE additions
static float sseSquash(__m128 sseVector)
{
	__m128 sseVectorCopy = sseVector;

	__m128 sseXXXXVector = _mm_shuffle_ps(sseVector, sseVectorCopy, _MM_SHUFFLE(3, 3, 3, 3));
	__m128 sseYYYYVector = _mm_shuffle_ps(sseVector, sseVectorCopy, _MM_SHUFFLE(2, 2, 2, 2));
	__m128 sseZZZZVector = _mm_shuffle_ps(sseVector, sseVectorCopy, _MM_SHUFFLE(1, 1, 1, 1));
	__m128 sseWWWWVector = _mm_shuffle_ps(sseVector, sseVectorCopy, _MM_SHUFFLE(0, 0, 0, 0));

	__m128 sseXPlusYVector = _mm_add_ps(sseXXXXVector, sseYYYYVector);
	__m128 sseXPlusYPlusZVector = _mm_add_ps(sseXPlusYVector, sseZZZZVector);
	__m128 sseSumVector = _mm_add_ps(sseXPlusYPlusZVector, sseWWWWVector);

	return _mm_cvtss_f32(sseSumVector);
}
