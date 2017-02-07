#include "stdafx.h"
#include "Vector4.h"
#include "Vector3.h"

Vector4::Vector4()
{
	sseVectorData = _mm_set_ps(0, 0, 0, 0);
}

Vector4::Vector4(float x, float y, float z, float w)
{
	sseVectorData = _mm_set_ps(x, y, z, w);
}

Vector4::~Vector4()
{

}

float Vector4::magnitude()
{
	__m128 sseRHSVector = sseVectorData;
	__m128 sseSquareVector = _mm_mul_ps(sseVectorData, sseRHSVector);
	__m128 sseSquareVectorCopy = sseSquareVector;

	__m128 sseXXXXVector = _mm_shuffle_ps(sseSquareVector, sseSquareVectorCopy, _MM_SHUFFLE(3, 3, 3, 3));
	__m128 sseYYYYVector = _mm_shuffle_ps(sseSquareVector, sseSquareVectorCopy, _MM_SHUFFLE(2, 2, 2, 2));
	__m128 sseZZZZVector = _mm_shuffle_ps(sseSquareVector, sseSquareVectorCopy, _MM_SHUFFLE(1, 1, 1, 1));
	__m128 sseWWWWVector = _mm_shuffle_ps(sseSquareVector, sseSquareVectorCopy, _MM_SHUFFLE(0, 0, 0, 0));

	__m128 sseXPlusYVector = _mm_add_ps(sseXXXXVector, sseYYYYVector);
	__m128 sseSqrSumVector = _mm_add_ps(sseXPlusYVector, sseWWWWVector);
	__m128 sseSumVector = _mm_sqrt_ps(sseSqrSumVector);

	float eSquared = _mm_cvtss_f32(sseSumVector);
	return eSquared;
}

float Vector4::sqrMagnitude()
{
	__m128 sseRHSVector = sseVectorData;
	__m128 sseSquareVector = _mm_mul_ps(sseVectorData, sseRHSVector);
	__m128 sseSquareVectorCopy = sseSquareVector;

	__m128 sseXXXXVector = _mm_shuffle_ps(sseSquareVector, sseSquareVectorCopy, _MM_SHUFFLE(3, 3, 3, 3));
	__m128 sseYYYYVector = _mm_shuffle_ps(sseSquareVector, sseSquareVectorCopy, _MM_SHUFFLE(2, 2, 2, 2));
	__m128 sseZZZZVector = _mm_shuffle_ps(sseSquareVector, sseSquareVectorCopy, _MM_SHUFFLE(1, 1, 1, 1));
	__m128 sseWWWWVector = _mm_shuffle_ps(sseSquareVector, sseSquareVectorCopy, _MM_SHUFFLE(0, 0, 0, 0));

	__m128 sseXPlusYVector = _mm_add_ps(sseXXXXVector, sseYYYYVector);
	__m128 sseSqrSumVector = _mm_add_ps(sseXPlusYVector, sseWWWWVector);

	float eSquared = _mm_cvtss_f32(sseSqrSumVector);
	return eSquared;
}

void Vector4::Scale(const Vector4& scaleBy)
{
	__m128 sseRHSVector = _mm_set_ps(scaleBy[0], scaleBy[1], scaleBy[2], scaleBy[3]);
	__m128 sseScaledVector = _mm_mul_ps(sseVectorData, sseRHSVector);
	sseVectorData = sseScaledVector;
}

void Vector4::Scale(float scaleBy)
{
	__m128 sseRHSVector = _mm_set_ps(scaleBy, scaleBy, scaleBy, scaleBy);
	__m128 sseScaledVector = _mm_mul_ps(sseVectorData, sseRHSVector);
	sseVectorData = sseScaledVector;
}

Vector4 Vector4::GetScaled(const Vector4& scaleBy)
{
	__m128 sseRHSVector = _mm_set_ps(scaleBy[0], scaleBy[1], scaleBy[2], scaleBy[3]);
	__m128 sseResultVector = _mm_mul_ps(sseVectorData, sseRHSVector);
	Vector4 returnableResult = Vector4(sseVectorData.m128_f32[0], sseVectorData.m128_f32[1],
									   sseVectorData.m128_f32[2], sseVectorData.m128_f32[3]);
	return returnableResult;
}

Vector4 Vector4::GetScaled(float scaleBy)
{
	__m128 sseRHSVector = _mm_set_ps(scaleBy, scaleBy, scaleBy, scaleBy);
	__m128 sseResultVector = _mm_mul_ps(sseVectorData, sseRHSVector);
	Vector4 returnableResult = Vector4(sseVectorData.m128_f32[0], sseVectorData.m128_f32[1],
									   sseVectorData.m128_f32[2], sseVectorData.m128_f32[3]);
	return returnableResult;
}

float Vector4::Dot(const Vector4& rhs)
{
	__m128 sseRHSVector = _mm_set_ps(rhs[0], rhs[1], rhs[2], rhs[3]);
	__m128 sseScaledVector = _mm_mul_ps(sseVectorData, sseRHSVector);
	__m128 sseScaledVectorCopy = sseScaledVector;

	__m128 sseXXXXVector = _mm_shuffle_ps(sseScaledVector, sseScaledVectorCopy, _MM_SHUFFLE(3, 3, 3, 3));
	__m128 sseYYYYVector = _mm_shuffle_ps(sseScaledVector, sseScaledVectorCopy, _MM_SHUFFLE(2, 2, 2, 2));
	__m128 sseZZZZVector = _mm_shuffle_ps(sseScaledVector, sseScaledVectorCopy, _MM_SHUFFLE(1, 1, 1, 1));
	__m128 sseWWWWVector = _mm_shuffle_ps(sseScaledVector, sseScaledVectorCopy, _MM_SHUFFLE(0, 0, 0, 0));

	__m128 sseXPlusYVector = _mm_add_ps(sseXXXXVector, sseYYYYVector);
	__m128 sseSumVector = _mm_add_ps(sseXPlusYVector, sseWWWWVector);

	return _mm_cvtss_f32(sseSumVector);
}

void Vector4::Transform(SQTTransformer transformer)
{
	Scale(transformer.scaleFactor);
	Rotate(transformer.rotation);

	__m128 sseTranslation = _mm_set_ps(transformer.translation[0], transformer.translation[1],
									   transformer.translation[2], 0);
	sseVectorData = _mm_add_ps(sseVectorData, sseTranslation);
}

void Vector4::Rotate(Quaternion rotateWith)
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
	Quaternion quaternionFormVector = Quaternion(Vector3(sseVectorData.m128_f32[3], sseVectorData.m128_f32[2], sseVectorData.m128_f32[1]), PI);

	// Generate the product qv
	Quaternion QuaternionLHS = rotateWith.BasicBlend(quaternionFormVector);

	// Use the previous product to complete the quaternion operation (qv)q*
	Quaternion completeRotation = QuaternionLHS.BasicBlend(rotationConjugate);

	// Retrieve + assign vector components here
	sseVectorData.m128_f32[1] = completeRotation.GetAxis()[2];
	sseVectorData.m128_f32[2] = completeRotation.GetAxis()[1];
	sseVectorData.m128_f32[3] = completeRotation.GetAxis()[0];
}

void Vector4::Normalize()
{
	float cachedMagnitude = magnitude();
	__m128 sseDivisor = _mm_set_ps(cachedMagnitude, cachedMagnitude,
								   cachedMagnitude, cachedMagnitude);
	sseVectorData = _mm_div_ps(sseVectorData, sseDivisor);
}

Vector4 Vector4::operator+(const Vector4& rhs)
{
	__m128 sseRHSVector = _mm_set_ps(rhs[0], rhs[1], rhs[2], rhs[3]);
	__m128 sseResultVector = _mm_add_ps(sseVectorData, sseRHSVector);
	Vector4 returnableResult = Vector4(sseVectorData.m128_f32[3], sseVectorData.m128_f32[2],
									   sseVectorData.m128_f32[1], sseVectorData.m128_f32[0]);
	return returnableResult;
}

Vector4 Vector4::operator-(const Vector4& rhs)
{
	__m128 sseRHSVector = _mm_set_ps(rhs[0], rhs[1], rhs[2], rhs[3]);
	__m128 sseResultVector = _mm_sub_ps(sseVectorData, sseRHSVector);
	Vector4 returnableResult = Vector4(sseVectorData.m128_f32[3], sseVectorData.m128_f32[2],
									   sseVectorData.m128_f32[1], sseVectorData.m128_f32[0]);
	return returnableResult;
}

void Vector4::operator+=(const Vector4& rhs)
{
	__m128 sseRHSVector = _mm_set_ps(rhs[0], rhs[1], rhs[2], rhs[3]);
	__m128 sseSumVector = _mm_add_ps(sseVectorData, sseRHSVector);
	sseVectorData = sseSumVector;
}

void Vector4::operator-=(const Vector4& rhs)
{
	__m128 sseRHSVector = _mm_set_ps(rhs[0], rhs[1], rhs[2], rhs[3]);
	__m128 sseDiffVector = _mm_sub_ps(sseVectorData, sseRHSVector);
	sseVectorData = sseDiffVector;
}

float& Vector4::operator[](char index)
{
	return sseVectorData.m128_f32[index];
}

const float Vector4::operator[](char index) const
{
	return sseVectorData.m128_f32[index];
}