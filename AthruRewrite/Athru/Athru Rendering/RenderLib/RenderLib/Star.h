#pragma once

#include <directxmath.h>

class Star
{
	public:
		Star() {}
		Star(float givenRadius, DirectX::XMFLOAT4 givenColor);
		~Star();

		// Retrieve the amount of matter associated
		// with [this]
		float GetRadius();

		// Retrieve the color associated with [this]
		DirectX::XMFLOAT4 GetColor();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		float radius;
		DirectX::XMFLOAT4 color;
};

