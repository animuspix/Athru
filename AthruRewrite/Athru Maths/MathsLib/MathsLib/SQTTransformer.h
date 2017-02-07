#pragma once

#include "Matrix4.h"
#include "Quaternion.h"

struct SQTTransformer
{
	SQTTransformer();
	SQTTransformer(float baseScale, Quaternion baseRotation,
				   float xTranslation, float yTranslation, float zTranslation);
	~SQTTransformer();

	Matrix4 asMatrix();

	float scaleFactor;
	Quaternion rotation;
	float translation[3];
};
