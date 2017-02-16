#pragma once

#include "Matrix4.h"
#include "Quaternion.h"

struct SQTTransformer
{
	SQTTransformer();
	SQTTransformer(float baseScale, Quaternion& baseRotation, Vector3& baseTranslation);
	~SQTTransformer();

	Matrix4 asMatrix();

	float scaleFactor;
	Quaternion rotation;
	Vector3 translation;
};
