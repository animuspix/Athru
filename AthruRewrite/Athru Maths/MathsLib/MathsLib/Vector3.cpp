#include "stdafx.h"
#include "Vector3.h"
#include "SQTTransformer.h"
#include "Quaternion.h"

Vector3::Vector3()
{
	vectorData[0] = 0;
	vectorData[1] = 0;
	vectorData[2] = 0;
}

Vector3::Vector3(float x, float y, float z)
{
	vectorData[0] = x;
	vectorData[1] = y;
	vectorData[2] = z;
}

Vector3::~Vector3()
{

}

float Vector3::magnitude()
{
	float aSquared = vectorData[0] * vectorData[0];
	float bSquared = vectorData[1] * vectorData[1];
	float cSquared = vectorData[2] * vectorData[2];
	float dSquared = aSquared + bSquared + cSquared;
	return sqrt(dSquared);
}

float Vector3::sqrMagnitude()
{
	float aSquared = vectorData[0] * vectorData[0];
	float bSquared = vectorData[1] * vectorData[1];
	float cSquared = vectorData[2] * vectorData[2];
	float dSquared = aSquared + bSquared + cSquared;
	return dSquared;
}

void Vector3::Scale(Vector3 scaleBy)
{
	vectorData[0] *= scaleBy[0];
	vectorData[1] *= scaleBy[1];
	vectorData[2] *= scaleBy[2];
}

void Vector3::Scale(float scaleBy)
{
	vectorData[0] *= scaleBy;
	vectorData[1] *= scaleBy;
	vectorData[2] *= scaleBy;
}

Vector3 Vector3::GetScaled(Vector3 scaleBy)
{
	Vector3 result = Vector3(vectorData[0], vectorData[1], vectorData[2]);
	result[0] *= scaleBy[0];
	result[1] *= scaleBy[1];
	result[2] *= scaleBy[2];
	return result;
}

Vector3 Vector3::GetScaled(float scaleBy)
{
	Vector3 result = Vector3(vectorData[0], vectorData[1], vectorData[2]);
	result[0] *= scaleBy;
	result[1] *= scaleBy;
	result[2] *= scaleBy;
	return result;
}

float Vector3::Dot(Vector3 rhs)
{
	float resultX = vectorData[0] * rhs[0];
	float resultY = vectorData[1] * rhs[1];
	float resultZ = vectorData[2] * rhs[2];
	return resultX + resultY + resultZ;
}

void Vector3::Transform(SQTTransformer transformer)
{
	Scale(transformer.scaleFactor);
	Rotate(transformer.rotation);

	vectorData[0] += transformer.translation[0];
	vectorData[1] += transformer.translation[1];
	vectorData[2] += transformer.translation[2];
}

void Vector3::Rotate(Quaternion rotateWith)
{
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
	Quaternion quaternionFormVector = Quaternion(*this, PI);

	// Generate the product qv
	Quaternion QuaternionLHS = rotateWith.BasicBlend(quaternionFormVector);

	// Use the previous product to complete the quaternion operation (qv)q*
	Quaternion completeRotation = QuaternionLHS.BasicBlend(rotationConjugate);

	// Retrieve + assign vector components here
	vectorData[0] = completeRotation.GetAxis()[0];
	vectorData[1] = completeRotation.GetAxis()[1];
	vectorData[2] = completeRotation.GetAxis()[2];
}

Vector3 Vector3::Cross(Vector3 rhs)
{
	float resultX = vectorData[1] * rhs[2] - vectorData[2] * rhs[1];
	float resultY = vectorData[2] * rhs[0] - vectorData[0] * rhs[2];
	float resultZ = vectorData[0] * rhs[1] - vectorData[1] * rhs[0];
	return Vector3(resultX, resultY, resultZ);
}

void Vector3::Normalize()
{
	float cachedMagnitude = magnitude();
	vectorData[0] /= cachedMagnitude;
	vectorData[1] /= cachedMagnitude;
	vectorData[2] /= cachedMagnitude;
}

Vector3 Vector3::operator+(Vector3 rhs)
{
	Vector3 result(vectorData[0], vectorData[1], vectorData[2]);
	result[0] += rhs[0];
	result[1] += rhs[1];
	result[2] += rhs[2];
	return result;
}

Vector3 Vector3::operator-(Vector3 rhs)
{
	Vector3 result(vectorData[0], vectorData[1], vectorData[2]);
	result[0] -= rhs[0];
	result[1] -= rhs[1];
	result[2] -= rhs[2];
	return result;
}

void Vector3::operator+=(Vector3 rhs)
{
	vectorData[0] += rhs[0];
	vectorData[1] += rhs[1];
	vectorData[2] += rhs[2];
}

void Vector3::operator-=(Vector3 rhs)
{
	vectorData[0] -= rhs[0];
	vectorData[1] -= rhs[1];
	vectorData[2] -= rhs[2];
}

float& Vector3::operator[](char index)
{
	return vectorData[index];
}