#pragma once

#include "SQTTransformer.h"
#include "Quaternion.h"

// While this is technically a 4-d vector,
// it doesn't support the wedge product or 4-D
// transformations; the actual utility of
// 4-dimensional geometry doesn't seem to be
// worth the mathematical complexity involved :P
class Vector4
{
	public:
		Vector4();
		Vector4(float x, float y, float z, float w);
		~Vector4();

		// Fetch the vector magnitude
		// Might be bad for performance since
		// it uses a square root
		float magnitude();

		// Fetch the squared vector magnitude
		// Less accurate since it's only the
		// square of the magnitude and not the
		// magnitude itself, but no roots means
		// better performance
		float Vector4::sqrMagnitude();

		// Scale [this] by a given vector
		// (multiply every component in one vector
		// by it's pair in another vector)
		void Scale(const Vector4& scaleBy);

		// Scale [this] by a given scalar
		// (multiply every component of [this] by
		// the given float)
		void Scale(float scaleBy);

		// Same as the vector scale function above,
		// but returns a scaled copy of [this]
		// rather than scaling [this] and returning
		// [void]
		Vector4 GetScaled(const Vector4& scaleBy);

		// Same as the scalar scale function above,
		// but returns a scaled copy of [this]
		// rather than scaling [this] and returning
		// [void]
		Vector4 GetScaled(float scaleBy);

		// Compute the dot-product of two vectors
		// (multiply every component in one vector
		// by it's pair in another vector, then
		// add the products together and return the
		// result)
		float Dot(const Vector4& rhs);

		// Arbitrarily transform [this] by a given SQT
		// generic transformation system
		void Transform(SQTTransformer transformer);

		// Rotate [this] around a given axis and by a
		// given number of radians
		void Rotate(Quaternion rotateWith);

		// Normalise [this] (set its magnitude to one,
		// effectively converting it to a pure directional vector)
		void Normalize();

		// Overloads for vector addition/subtraction
		Vector4 operator+(const Vector4& rhs);
		Vector4 operator-(const Vector4& rhs);
		void operator+=(const Vector4& rhs);
		void operator-=(const Vector4& rhs);

		// Subscript operator overload for flexible
		// component access ((readable and writable)
		// (requires a non-const LHS))
		float& operator[](char index);

		// Subscript operator overload for flexible
		// component access ((readable and writable)
		// (requires a const LHS))
		const float operator[](char index) const;

	private:
		__m128 sseVectorData;
};