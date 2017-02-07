#pragma once

#include "SQTTransformer.h"
#include "Quaternion.h"

class Vector2
{
	public:
		Vector2();
		Vector2(float x, float y);
		~Vector2();

		// Fetch the vector magnitude
		// Might be bad for performance since
		// it uses a square root
		float magnitude();

		// Fetch the squared vector magnitude
		// Less accurate since it's only the
		// square of the magnitude and not the
		// magnitude itself, but no roots means
		// better performance
		float Vector2::sqrMagnitude();

		// Scale [this] by a given vector
		// (multiply every component in one vector
		// by it's pair in another vector)
		void Scale(Vector2 scaleBy);

		// Scale [this] by a given scalar
		// (multiply every component of [this] by
		// the given float)
		void Scale(float scaleBy);

		// Compute the dot-product of two vectors
		// (multiply every component in one vector
		// by it's pair in another vector, then
		// add the products together and return the
		// result)
		float Dot(Vector2 rhs);

		// Arbitrarily transform [this] by a given matrix
		void Transform(SQTTransformer transformer);

		// Rotate [this] about a given point and by a
		// given number of radians
		void Rotate(Quaternion rotateWith);

		// Normalise [this] (set its magnitude to one,
		// effectively converting it to a pure directional vector)
		void Normalize();

		// Overloads for vector addition/subtraction
		Vector2 operator+(Vector2 rhs);
		Vector2 operator-(Vector2 rhs);
		void operator+=(Vector2 rhs);
		void operator-=(Vector2 rhs);

		// Subscript operator overload for flexible
		// component access (readable and writable)
		float& operator[](char index);

	private:
		float vectorData[2];
};

