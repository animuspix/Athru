#pragma once

struct SQTTransformer;
class Quaternion;
class Vector3
{
	public:
		Vector3();
		Vector3(float x, float y, float z);
		~Vector3();

		// Fetch the vector magnitude
		// Might be bad for performance since
		// it uses a square root
		float magnitude();

		// Fetch the squared vector magnitude
		// Less accurate since it's only the
		// square of the magnitude and not the
		// magnitude itself, but no roots means
		// better performance
		float Vector3::sqrMagnitude();

		// Scale [this] by a given vector
		// (multiply every component in one vector
		// by it's pair in another vector)
		void Scale(Vector3 scaleBy);

		// Scale [this] by a given scalar
		// (multiply every component of [this] by
		// the given float)
		void Scale(float scaleBy);

		// Same as the vector scale function above,
		// but returns a scaled copy of [this]
		// rather than scaling [this] and returning
		// [void]
		Vector3 GetScaled(Vector3 scaleBy);

		// Same as the scalar scale function above,
		// but returns a scaled copy of [this]
		// rather than scaling [this] and returning
		// [void]
		Vector3 GetScaled(float scaleBy);

		// Compute the dot-product of two vectors
		// (multiply every component in one vector
		// by it's pair in another vector, then
		// add the products together and return the
		// result)
		float Dot(Vector3 rhs);

		// Arbitrarily transform [this] by a given matrix
		void Transform(SQTTransformer transformMatrix);

		// Rotate [this] through the axis and rotation
		// defined by the given quaternion
		void Rotate(Quaternion rotateWith);

		// Compute the cross-product of two vectors
		// (multiply the components in each vector
		// together in a "crosswise" pattern; see
		// the Wikipedia article on the cross product
		// [https://en.wikipedia.org/wiki/Cross_product]
		// and/or the maths section of Game Engine
		// Architecture for a better explanation)
		Vector3 Cross(Vector3 rhs);

		// Normalise [this] (set its magnitude to one,
		// effectively converting it to a pure directional vector)
		void Normalize();

		// Overloads for vector addition/subtraction
		Vector3 operator+(Vector3 rhs);
		Vector3 operator-(Vector3 rhs);
		void operator+=(Vector3 rhs);
		void operator-=(Vector3 rhs);

		// Subscript operator overload for flexible
		// component access (readable and writable)
		float& operator[](char index);

	private:
		float vectorData[3];
};