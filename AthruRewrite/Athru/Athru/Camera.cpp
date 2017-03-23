#include "ServiceCentre.h"
#include "Graphics.h"
#include "Material.h"
#include "Camera.h"

Camera::Camera(DirectX::XMMATRIX& projectorMatrix)
{
	// Set the camera's default position
	position = _mm_set_ps(0, 0, (CHUNK_WIDTH / 2) + 2, 0);

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

	// Create the view matrix with [position], [lookAt], and [localUp]
	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, localUp);

	// Initialise the screen rect
	viewFinder = AthruRect(Material(Sound(), 
									1.0f, 1.0f, 1.0f, 1.0f, 
									DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
									DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
									DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
									DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
									DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
									FORWARD::AVAILABLE_OBJECT_SHADERS::BUFFER_RASTERIZER, 
									FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
									FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
									FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
									FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER, AthruTexture()), 
						   FRUSTUM_WIDTH_AT_NEAR, FRUSTUM_HEIGHT_AT_NEAR);

	// Initialise screen rect transformations
	DirectX::XMVECTOR viewFinderPos = _mm_set_ps(1, SCREEN_NEAR, (CHUNK_WIDTH / 2) + 2, 0);
	DirectX::XMVECTOR viewFinderRot = DirectX::XMQuaternionRotationRollPitchYaw(0, 0, 0);
	viewFinder.FetchTransformations() = SQT(viewFinderPos, viewFinderRot, 1);

	// Initialise the "last" mouse position to zero since the mouse won't have
	// moved before the main [Camera] instance is constructed :P
	lastMousePos.x = 0;
	lastMousePos.y = 0;

	// Initialise the spinSpeed modifier for mouse-look
	spinSpeed = 0.4f;
}

Camera::~Camera()
{

}

void Camera::Translate(DirectX::XMVECTOR displacement)
{
	// Translate the software camera
	DirectX::XMVECTOR positionCopy = position;
	position = _mm_add_ps(positionCopy, displacement);

	// Translate the view-finder (screen rect)
	DirectX::XMVECTOR rectPositionCopy = viewFinder.FetchTransformations().pos;
	viewFinder.FetchTransformations().pos = _mm_add_ps(rectPositionCopy, displacement);
}

DirectX::XMVECTOR Camera::GetTranslation()
{
	return position;
}

void Camera::SetRotation(DirectX::XMFLOAT3 eulerAngles)
{
	// Rotate the software camera
	rotationEuler = eulerAngles;
	rotationQuaternion = DirectX::XMQuaternionRotationRollPitchYaw(rotationEuler.x, rotationEuler.y, rotationEuler.z);

	// Rotate the view-finder (screen rect)
	viewFinder.FetchTransformations().rotationQuaternion = rotationQuaternion;
}

DirectX::XMVECTOR Camera::GetRotationQuaternion()
{
	return rotationQuaternion;
}

DirectX::XMFLOAT3 Camera::GetRotationEuler()
{
	return rotationEuler;
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
	float xRotationDisp = spinSpeed * mouseDispY * TimeStuff::deltaTime();
	float yRotationDisp = spinSpeed * mouseDispX * TimeStuff::deltaTime();

	// We're using an incremental mouse look now, so add the current rotation
	// into the displacements generated above before setting the new camera angles
	SetRotation(DirectX::XMFLOAT3(rotationEuler.x + xRotationDisp,
								  rotationEuler.y + yRotationDisp,
								  0));

	// Update [lastMousePos] with the coordinates stored for this frame
	lastMousePos.x = currMousePos.x;
	lastMousePos.y = currMousePos.y;
}

AthruRect& Camera::GetViewFinder()
{
	return viewFinder;
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

// Push constructions for this class through Athru's custom allocator
void* Camera::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Camera>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Camera::operator delete(void* target)
{
	return;
}