#include "MathIncludes.h"
#include "Camera.h"

Camera::Camera()
{
	// HEAVY debugging needed for the SQT transformer; use the DirectXMath classes
	// until engine debugging is finished

	// Initialise the camera scale (not really a thing, but it's easier to leave it in),
	// rotation and translation
	transformData = SQTTransformer(1, 
								   Quaternion(Vector3(1, 0, 0), 0.5f * PI), 
								   Vector3(0, 0, 1));
	
	viewMatrix = transformData.asMatrix().inverse();
}

Camera::~Camera()
{
}

Vector3 Camera::GetTranslation()
{
	return transformData.translation;
}

Quaternion Camera::GetRotation()
{
	return transformData.rotation;
}

DirectX::XMMATRIX Camera::GetViewMatrix()
{
	return DirectX::XMMATRIX(viewMatrix.GetVector(0),
							 viewMatrix.GetVector(1),
							 viewMatrix.GetVector(2),
							 viewMatrix.GetVector(3));
}

void Camera::Translate(float& x, float& y, float& z)
{
	transformData.translation[0] += x;
	transformData.translation[1] += y;
	transformData.translation[2] += z;
}

// Ask Adam how I'd make a RotateAround() function

void Camera::SetRotation(Vector3& axis, float& angleInRads)
{
	transformData.rotation = Quaternion(axis, angleInRads);
}

void Camera::RefreshViewMatrix()
{
	viewMatrix = transformData.asMatrix().inverse();
}
