#include "ServiceCentre.h"
#include "Camera.h"

Camera::Camera()
{
	// Set the camera's default position
	position = _mm_set_ps(0, -4.5f, 0, 0);

	// Set the camera's default rotation
	rotationQuaternion = DirectX::XMQuaternionRotationRollPitchYaw(0, 0, 0);
	rotationEuler = DirectX::XMFLOAT3(0, 0, 0);

	// Setup the local "up"-vector
	DirectX::XMVECTOR localUp;
	localUp = _mm_set_ps(0, 0, 1, 0);

	// Setup where the camera is looking by default
	DirectX::XMVECTOR lookAt;
	lookAt = _mm_set_ps(0, 1, 0, 0);

	// Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin
	lookAt = DirectX::XMVector3Rotate(lookAt, rotationQuaternion);
	localUp = DirectX::XMVector3Rotate(localUp, rotationQuaternion);

	// Translate the rotated camera position to the location of the viewer
	lookAt = _mm_add_ps(position, lookAt);

	// Finally create the view matrix with [position], [lookAt], and [localUp]
	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, localUp);

	// Initialise the "last" mouse position to zero since the mouse won't have
	// moved before the main Camera instance is constructed :P
	lastMousePos.x = 0;
	lastMousePos.y = 0;

	// Initialise the mouse speed modifier
	speed = 0.1f;
}


Camera::~Camera()
{

}

void Camera::Translate(DirectX::XMVECTOR displacement)
{
	DirectX::XMVECTOR positionCopy = position;
	position = _mm_add_ps(positionCopy, displacement);
}

DirectX::XMVECTOR Camera::GetTranslation()
{
	return position;
}

void Camera::SetRotation(float eulerX, float eulerY, float eulerZ)
{
	rotationEuler.x = eulerX;
	rotationEuler.y = eulerY;
	rotationEuler.z = eulerZ;
	rotationQuaternion = DirectX::XMQuaternionRotationRollPitchYaw(rotationEuler.x, rotationEuler.y, rotationEuler.z);
}

DirectX::XMVECTOR Camera::GetRotation()
{
	return rotationQuaternion;
}

DirectX::XMMATRIX Camera::GetViewMatrix()
{
	return viewMatrix;
}

void Camera::MouseLook(Input* inputPttr)
{
	DirectX::XMFLOAT2 currMousePos = inputPttr->GetMousePos();
	float mouseDispX = currMousePos.x - lastMousePos.x;
	float mouseDispY = currMousePos.y - lastMousePos.y;

	// Rotate the camera through mouse XY coordinates
	float xRotationDisp = speed * mouseDispY * TimeStuff::deltaTime();
	float yRotationDisp = speed * mouseDispX * TimeStuff::deltaTime();

	// We're using an incremental mouse look now, so add the current rotation
	// into the displacements generated above before setting the new camera angles
	SetRotation(rotationEuler.x + xRotationDisp, rotationEuler.y + yRotationDisp, 0);

	// Update [lastMousePos] with the coordinates stored for this frame
	lastMousePos.x = currMousePos.x;
	lastMousePos.y = currMousePos.y;
}

void Camera::RefreshViewMatrix()
{
	// Setup the local "up"-vector
	DirectX::XMVECTOR localUp;
	localUp = _mm_set_ps(0, 0, 1, 0);

	// Setup where the camera is looking by default
	DirectX::XMVECTOR lookAt;
	lookAt = _mm_set_ps(0, 1, 0, 0);

	// Transform the lookAt and up vector by the rotation quaternion so the view is correctly rotated at the origin
	lookAt = DirectX::XMVector3Rotate(lookAt, rotationQuaternion);
	localUp = DirectX::XMVector3Rotate(localUp, rotationQuaternion);

	// Translate the rotated camera position to the location of the viewer
	lookAt = _mm_add_ps(position, lookAt);

	// Finally create the view matrix with [position], [lookAt], and [localUp]
	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, localUp);
}