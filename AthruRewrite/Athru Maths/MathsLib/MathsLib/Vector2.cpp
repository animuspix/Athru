#include "stdafx.h"
#include "Vector2.h"
#include "Vector3.h"

Vector2::Vector2()
{
	vectorData[0] = 0;
	vectorData[1] = 0;
}

Vector2::Vector2(float x, float y)
{
	vectorData[0] = x;
	vectorData[1] = y;
}

Vector2::~Vector2()
{

}

float Vector2::magnitude()
{
	float aSquared = vectorData[0] * vectorData[0];
	float bSquared = vectorData[1] * vectorData[1];
	float cSquared = aSquared + bSquared;
	return sqrt(cSquared);
}

float Vector2::sqrMagnitude()
{
	float aSquared = vectorData[0] * vectorData[0];
	float bSquared = vectorData[1] * vectorData[1];
	float cSquared = aSquared + bSquared;
	return cSquared;
}

void Vector2::Scale(float scaleBy)
{
	vectorData[0] *= scaleBy;
	vectorData[1] *= scaleBy;
}

void Vector2::Scale(Vector2 scaleBy)
{
	vectorData[0] *= scaleBy[0];
	vectorData[1] *= scaleBy[1];
}

float Vector2::Dot(Vector2 rhs)
{
	float resultX = vectorData[0] * rhs[0];
	float resultY = vectorData[1] * rhs[1];
	return resultX + resultY;
}

void Vector2::Transform(SQTTransformer transformer)
{
	Scale(transformer.scaleFactor);
	Rotate(transformer.rotation);

	vectorData[0] += transformer.translation[0];
	vectorData[1] += transformer.translation[1];
}

void Vector2::Rotate(Quaternion rotateWith)
{
	// Vector2's can't rotate about X or Y,
	// so raise an error if the axis of rotation
	// isn't parallel with Z
	assert(rotateWith.GetAxis()[0] == 0);
	assert(rotateWith.GetAxis()[1] == 0);

	// The operation to rotate a vector by a quaternion is defined as
	// qvq*, where q* is the conjugate of the rotating quaternion and
	// v is the rotating vector in quaternion form (defined as x, y, z, 0)

	// Consider extended mathematical breakdown here...

	Quaternion rotationConjugate = rotateWith.conjugate();
	// Send [this] into a quaternion of the form (x, y, z, 0)
	// by specifically creating a quaternion rotating around
	// [this] with an angle of [pi] degrees so that cos(0.5pi) = 0
	// and sin(0.5pi) = 1; see the Quaternion .cpp file for why
	// those particular rules have to be met
	Quaternion quaternionFormVector = Quaternion(Vector3(vectorData[0], vectorData[1], 0), PI);

	// Generate the product qv
	Quaternion QuaternionLHS = rotateWith.BasicBlend(quaternionFormVector);

	// Use the previous product to complete the quaternion operation (qv)q*
	Quaternion completeRotation = QuaternionLHS.BasicBlend(rotationConjugate);

	// Retrieve + assign vector components here
	vectorData[0] = completeRotation.GetAxis()[0];
	vectorData[1] = completeRotation.GetAxis()[1];
}

void Vector2::Normalize()
{
	float cachedMagnitude = magnitude();
	vectorData[0] /= cachedMagnitude;
	vectorData[1] /= cachedMagnitude;
}

Vector2 Vector2::operator+(Vector2 rhs)
{
	Vector2 result(vectorData[0], vectorData[1]);
	result[0] += rhs[0];
	result[1] += rhs[1];
	return result;
}

Vector2 Vector2::operator-(Vector2 rhs)
{
	Vector2 result(vectorData[0], vectorData[1]);
	result[0] -= rhs[0];
	result[1] -= rhs[1];
	return result;
}

void Vector2::operator+=(Vector2 rhs)
{
	vectorData[0] += rhs[0];
	vectorData[1] += rhs[1];
}

void Vector2::operator-=(Vector2 rhs)
{
	vectorData[0] -= rhs[0];
	vectorData[1] -= rhs[1];
}

float& Vector2::operator[](char index)
{
	return vectorData[index];
}