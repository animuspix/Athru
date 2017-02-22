#include "stdafx.h"
#include "Quaternion.h"
#include "Vector4.h"
#include "Vector3.h"

Quaternion::Quaternion()
{
	components[0] = 0;
	components[1] = 0;
	components[2] = 0;
	components[3] = 1;
	currentAxis = Vector3(0, 0, 1);
	currentAngle = 0;
}

Quaternion::Quaternion(Vector3 axis, float angleInRads)
{
	axis.Normalize();
	currentAxis = axis;
	currentAngle = angleInRads;

	components[0] = axis[0] * sin(angleInRads / 2);
	components[1] = axis[1] * sin(angleInRads / 2);
	components[2] = axis[2] * sin(angleInRads / 2);
	components[3] = cos(angleInRads / 2);
}

Quaternion::~Quaternion()
{

}

Quaternion Quaternion::conjugate()
{
	Vector3 conjugateAxis = currentAxis;
	conjugateAxis.Scale(-1);
	return Quaternion(conjugateAxis, currentAngle);
}

Matrix4 Quaternion::asMatrix()
{
	Matrix4 result = Matrix4();

	float twoWI = 2 * components[0] * components[1];
	float twoWJ = 2 * components[0] * components[1];
	float twoIJ = 2 * components[1] * components[2];
	float twoIK = 2 * components[1] * components[3];
	float twoJK = 2 * components[2] * components[3];
	float twoWK = 2 * components[0] * components[3];

	float twoWSquared = 2 * components[0] * components[0];
	float twoISquared = 2 * components[1] * components[1];
	float twoJSquared = 2 * components[2] * components[2];
	float twoKSquared = 2 * components[3] * components[3];
	float oneMinusTwoWSquared = 1 - twoWSquared;

	result.FetchValue(0, 0) = 1 - twoISquared - twoKSquared;
	result.FetchValue(0, 1) = twoWI + twoJK;
	result.FetchValue(0, 2) = twoWJ - twoIK;

	result.FetchValue(1, 0) = twoWI - twoJK;
	result.FetchValue(1, 1) = oneMinusTwoWSquared - twoKSquared;
	result.FetchValue(1, 2) = twoIJ + twoWK;

	result.FetchValue(2, 0) = twoWJ + twoIK;
	result.FetchValue(2, 1) = twoWI - twoWK;
	result.FetchValue(2, 2) = oneMinusTwoWSquared - twoJSquared;

	return result;
}

Quaternion Quaternion::BasicBlend(Quaternion blendWith)
{
	// Investigate ways to extract the axis of rotation + angle
	// without the dodgy inverse-multiplication method I'm using
	// atm

	Vector3 vectorA = Vector3(components[0], components[1], components[2]);
	Vector3 vectorB = Vector3(blendWith[0], blendWith[1], blendWith[2]);

	float scalarA = components[3];
	float scalarB = blendWith[3];

	Vector3 resultVectorA = vectorA.GetScaled(scalarB) + vectorB.GetScaled(scalarA);
	Vector3 resultVectorB = vectorA.Cross(vectorB);

	Vector3 resultVector = resultVectorA + resultVectorB;
	float resultScalar = (scalarA * scalarB) - vectorA.Dot(vectorB);

	float resultAngle = acos(resultScalar) * 2;
	resultVector.Scale(1 / sin(resultAngle));

	Quaternion result = Quaternion(resultVector, resultAngle);
	return result;
}

Quaternion Quaternion::SlerpyBlend(Quaternion blendWith, float blendAmount)
{
	// Investigate ways to extract the axis of rotation + angle
	// without the dodgy inverse-multiplication method I'm using
	// atm

	// [Theta] is defined as the angle directly between [this] and [blendWith]
	Vector4 cosThetaLHS = Vector4(components[0], components[1], components[2], components[3]);
	Vector4 cosThetaRHS = Vector4(blendWith[0], blendWith[1], blendWith[2], blendWith[3]);
	float theta = acos(cosThetaLHS.Dot(cosThetaRHS));

	float slerpLHSCoefficient = sin(1 - blendAmount) * theta / sin(theta);
	float slerpRHSCoefficient = sin(blendAmount * theta) / sin(theta);

	Vector4 slerpLHSComponents = Vector4(slerpLHSCoefficient * components[0],
										 slerpLHSCoefficient * components[1],
										 slerpLHSCoefficient * components[2],
										 slerpLHSCoefficient * components[3] );

	Vector4 slerpRHSComponents = Vector4( slerpRHSCoefficient * blendWith[0],
										  slerpRHSCoefficient * blendWith[1],
										  slerpRHSCoefficient * blendWith[2],
										  slerpRHSCoefficient * blendWith[3] );

	Vector4 slerpComponents = slerpLHSComponents + slerpRHSComponents;
	Vector3 slerpAxis = Vector3(slerpComponents[0], slerpComponents[1], slerpComponents[2]);

	float slerpAngle = acos(slerpComponents[3]) * 2;
	slerpAxis.Scale(1 / sin(slerpAngle));

	return Quaternion(slerpAxis, slerpAngle);
}

void Quaternion::IncrementRotation(float amountInRads)
{
	float newAngle = currentAngle + amountInRads;
	components[0] = currentAxis[0] * sin(newAngle / 2);
	components[1] = currentAxis[1] * sin(newAngle / 2);
	components[2] = currentAxis[2] * sin(newAngle / 2);
	components[3] = cos(newAngle / 2);
	currentAngle = newAngle;
}

float Quaternion::GetAngle()
{
	return currentAngle;
}

void Quaternion::SetRotation(float angleInRads)
{
	float newAngle = angleInRads;
	components[0] = currentAxis[0] * sin(newAngle / 2);
	components[1] = currentAxis[1] * sin(newAngle / 2);
	components[2] = currentAxis[2] * sin(newAngle / 2);
	components[3] = cos(newAngle / 2);
	currentAngle = newAngle;
}

void Quaternion::SetAxis(Vector3 newAxis)
{
	currentAxis = newAxis;
	components[0] = currentAxis[0] * sin(currentAngle / 2);
	components[1] = currentAxis[1] * sin(currentAngle / 2);
	components[2] = currentAxis[2] * sin(currentAngle / 2);
	components[3] = cos(currentAngle / 2);
}

Vector3 Quaternion::GetAxis()
{
	return currentAxis;
}

float Quaternion::operator[](char index)
{
	return components[index];
}