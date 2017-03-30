#include "stdafx.h"
#include "SQTTransformer.h"
#include "Quaternion.h"

SQTTransformer::SQTTransformer()
{
	scaleFactor = 1;
	rotation = EMPTY_QUATERNION;
	translation = Vector3(0, 0, 0);
}

SQTTransformer::SQTTransformer(float baseScale, Quaternion& baseRotation, Vector3& baseTranslation)
{
	scaleFactor = baseScale;
	rotation = baseRotation;
	translation = baseTranslation;
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

	return translationMatrix * rotationMatrix * scalingMatrix;
}
