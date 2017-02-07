#include "stdafx.h"
#include "SQTTransformer.h"
#include "Quaternion.h"

SQTTransformer::SQTTransformer()
{
	scaleFactor = 1;
	rotation = EMPTY_QUATERNION;
	translation[0] = 0;
	translation[1] = 0;
	translation[2] = 0;
}

SQTTransformer::SQTTransformer(float baseScale, Quaternion baseRotation,
							   float xTranslation, float yTranslation, float zTranslation)
{
	scaleFactor = baseScale;
	rotation = baseRotation;
	translation[0] = xTranslation;
	translation[1] = yTranslation;
	translation[2] = zTranslation;
}

SQTTransformer::~SQTTransformer()
{

}

Matrix4 SQTTransformer::asMatrix()
{
	Matrix4 scalingMatrix = Matrix4();
	scalingMatrix.SetScale(scaleFactor, scaleFactor, scaleFactor);

	Matrix4 rotationMatrix;
	rotationMatrix = rotation.asMatrix();

	Matrix4 translationMatrix = Matrix4();
	translationMatrix.SetTranslate(translation[0], translation[1], translation[2]);

	return scalingMatrix * rotationMatrix * translationMatrix;
}
