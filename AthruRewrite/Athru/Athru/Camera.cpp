#include "Camera.h"

Camera::Camera()
{
	// Set the camera's default position
	position = _mm_set_ps(0, -4.5f, 0, 0);

	// Set the camera's default rotation
	rotation = _mm_set_ps(0, 0, 0, 0);

	// Setup the local "up"-vector
	DirectX::XMVECTOR localUp;
	localUp = _mm_set_ps(0, 0, 1, 0);

	// Setup where the camera is looking by default
	DirectX::XMVECTOR lookAt;
	lookAt = _mm_set_ps(0, 1, 0, 0);

	// Create a rotation matrix from the euler angles stored in [rotation]
	DirectX::XMMATRIX rotationMatrix;
	rotationMatrix = DirectX::XMMatrixRotationRollPitchYawFromVector(rotation);

	// Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin
	lookAt = XMVector3TransformCoord(lookAt, rotationMatrix);
	localUp = XMVector3TransformCoord(localUp, rotationMatrix);

	// Translate the rotated camera position to the location of the viewer
	lookAt = _mm_add_ps(position, lookAt);

	// Finally create the view matrix with [position], [lookAt], and [localUp]
	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, localUp);
}


Camera::~Camera()
{

}

void Camera::Translate(__m128 displacement)
{
	__m128 positionCopy = position;
	position = _mm_add_ps(position, displacement);
}

void Camera::SetRotation(__m128 eulerAnglesInRads)
{
	rotation = eulerAnglesInRads;
}

DirectX::XMMATRIX& Camera::GetViewMatrix()
{
	return viewMatrix;
}

void Camera::RefreshViewMatrix()
{
	// Fill out once I start moving the camera around, no real
	// reason to bother atm
}