#pragma once

#include <chrono>

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
		float test = deltaTimeValue.count();
		return deltaTimeValue.count();
	}
}
