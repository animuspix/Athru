#include "MathIncludes.h"
#include "Camera.h"

Camera::Camera()
{
	// HEAVY debugging needed for the SQT transformer; use the DirectXMath classes
	// until engine debugging is finished

	// Initialise the camera scale (not really a thing, but it's easier to leave it in),
	// rotation and translation
	//transformData = SQTTransformer(1, 
	//							   Quaternion(Vector3(1, 0, 0), 0.5f * PI), 
	//							   Vector3(0, 0, -10.0f));
	
	//viewMatrix = transformData.asMatrix().inverse();

	// As above, with a rotation matrix + euler angles instead of SQT (no real SQT support in
	// DirectX afaik)

	// Set values to our position/rotation vectors
	position = _mm_set_ps(0, 0, -10.0f, 0);
	rotation = _mm_set_ps(0, 0, 0, 0);

	// Create our rotation matrix from the rotation vector
	DirectX::XMMATRIX rotationMat = DirectX::XMMatrixRotationRollPitchYawFromVector(rotation);

	// Generate vectors representing "up" relative to the camera + where the camera is looking
	// at initialisation
	DirectX::XMVECTOR localUp = DirectX::XMVECTOR(_mm_set_ps(0, 1, 0, 0));
	DirectX::XMVECTOR lookAt = DirectX::XMVECTOR(_mm_set_ps(0, 0, 1, 0));
	
	// Rotate the "up"-vector and the view-direction vector with the rotation matrix
	// generated above
	localUp = DirectX::XMVector3TransformCoord(localUp, rotationMat);
	lookAt = DirectX::XMVector3TransformCoord(lookAt, rotationMat);

	// Use the position + adjusted lookAt/localUp vectors to generate the view matrix
	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, localUp);
}

Camera::~Camera()
{
}

//Vector3 Camera::GetTranslation()
//{
//	return transformData.translation;
//}
//
//Quaternion Camera::GetRotation()
//{
//	return transformData.rotation;
//}

DirectX::XMVECTOR Camera::GetTranslation()
{
	return position;
}

DirectX::XMVECTOR Camera::GetRotation()
{
	return rotation;
}

DirectX::XMMATRIX Camera::GetViewMatrix()
{
	//return DirectX::XMMATRIX(viewMatrix.GetVector(0),
	//						 viewMatrix.GetVector(1),
	//						 viewMatrix.GetVector(2),
	//						 viewMatrix.GetVector(3));
	return viewMatrix;
}

void Camera::Translate(__m128 displacement)
{
	//transformData.translation[0] += x;
	//transformData.translation[1] += y;
	//transformData.translation[2] += z;
	__m128 currentPos = position;
	position = _mm_add_ps(position, displacement);
}

// Ask Adam how I'd make a RotateAround() function

//void Camera::SetRotation(Vector3& axis, float& angleInRads)
//{
//	transformData.rotation = Quaternion(axis, angleInRads); 
//}

void Camera::SetRotation(__m128 eulerAnglesInRads)
{
	__m128 currentRotation = rotation;
	rotation = _mm_add_ps(currentRotation, eulerAnglesInRads);
}

void Camera::RefreshViewMatrix()
{
	//viewMatrix = transformData.asMatrix().inverse();

	// RasterTek solution instead of inverse SQT; mostly because it's reliable
	// and I don't have to spend a bajillion years debugging linear equations
	DirectX::XMMATRIX rotationMat = DirectX::XMMatrixRotationRollPitchYawFromVector(rotation);

	DirectX::XMVECTOR localUp = DirectX::XMVECTOR(_mm_set_ps(0, 1, 0, 0));
	DirectX::XMVECTOR lookAt = DirectX::XMVECTOR(_mm_set_ps(0, 0, 1, 0));

	localUp = DirectX::XMVector3TransformCoord(localUp, rotationMat);
	lookAt = DirectX::XMVector3TransformCoord(lookAt, rotationMat);

	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, localUp);
}
