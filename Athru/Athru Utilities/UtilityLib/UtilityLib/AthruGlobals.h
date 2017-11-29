#pragma once

#include <chrono>
#include <type_traits>
#include <typeinfo>
#include <directxmath.h>
#include <math.h>
#include "Typedefs.h"

namespace TimeStuff
{
	// Initialise the "previous" time to zero (since the
	// game starts with no previous frame and has to work
	// from there)
	extern std::chrono::steady_clock::time_point timeAtLastFrame;

	// [inline] instead of a macro so we get type-safety :)
	inline float deltaTime()
	{
		std::chrono::steady_clock::time_point current = std::chrono::steady_clock::now();
		std::chrono::duration<float> deltaTimeValue = std::chrono::duration_cast<std::chrono::duration<float>>(current - timeAtLastFrame);
		return deltaTimeValue.count();
	}

	inline twoByteUnsigned FPS()
	{
		return (twoByteUnsigned)(1.0f / deltaTime());
	}
}

namespace MathsStuff
{
	// An approximation of pi; stored as a number rather than an inline
	// function to avoid repeated divisions and/or trig operations
	extern const float PI;

	// Calculate XMFLOAT4 magnitudes
	// Implemented because I assumed float4s could only be
	// set through XMFLOAT4's, but it seems like I was wrong
	// about that; I should probably get rid of this and adjust
	// my process towards a pure-XMVECTOR system for greater
	// efficiency
	inline float sisdVectorMagnitude(DirectX::XMFLOAT4 sisdVector)
	{
		float sqrX = sisdVector.x * sisdVector.x;
		float sqrY = sisdVector.y * sisdVector.y;
		float sqrZ = sisdVector.z * sisdVector.z;
		return sqrt(sqrX + sqrY + sqrZ);
	}

	// Convert a given XMVECTOR (SSE) to an XMFLOAT4 (an array
	// of scalars) and return the result
	// Same note as above, only exists because I haven't
	// changed over to an all-SSE system yet :P
	inline DirectX::XMFLOAT4 sseToScalarVector(DirectX::XMVECTOR sseVector)
	{
		DirectX::XMFLOAT4 scalarVec;
		DirectX::XMStoreFloat4(&scalarVec, sseVector);
		return scalarVec;
	}

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
}

namespace GraphicsStuff
{
	// Display properties
	extern const bool FULL_SCREEN;
	extern const bool VSYNC_ENABLED;
	extern const fourByteUnsigned DISPLAY_WIDTH;
	extern const fourByteUnsigned DISPLAY_HEIGHT;
	extern const float DISPLAY_ASPECT_RATIO;
	extern const float SCREEN_FAR;
	extern const float SCREEN_NEAR;
	extern const float FRUSTUM_WIDTH_AT_NEAR;
	extern const float FRUSTUM_HEIGHT_AT_NEAR;
	extern const float VERT_FIELD_OF_VIEW_RADS;
	extern const float HORI_FIELD_OF_VIEW_RADS;
	extern constexpr fourByteUnsigned SCREEN_RECT_INDEX_COUNT = 6;
	extern constexpr fourByteUnsigned PROG_PASS_HEIGHT = 64;
	extern const fourByteUnsigned PROG_PASS_COUNT;
	extern const fourByteUnsigned MAX_TRACE_COUNT;
	extern const fourByteUnsigned GI_SAMPLES_PER_RAY;
	extern const fourByteUnsigned GI_SAMPLE_TOTAL;
}

namespace SceneStuff
{
	extern constexpr fourByteUnsigned SYSTEM_COUNT = 1;
	extern constexpr fourByteUnsigned MAX_NUM_SCENE_FIGURES = 10;
	extern constexpr fourByteUnsigned MAX_NUM_SCENE_ORGANISMS = 8;
}

namespace PlantStuff
{
	extern const fourByteUnsigned MAX_PLANT_AGE;
	extern const float MAX_PLANT_SIZE;
	extern const float MAX_TROPISM;
	extern const fourByteUnsigned MAX_BRANCH_COUNT_AT_FORK;
}

namespace CritterStuff
{
}

namespace GPGPUStuff
{
	extern constexpr fourByteUnsigned RAND_STATE_COUNT = 16777216;
}