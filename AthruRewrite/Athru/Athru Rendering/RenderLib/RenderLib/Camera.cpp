#include "UtilityServiceCentre.h"
#include "RenderServiceCentre.h"
#include "Camera.h"

Camera::Camera()
{
	// Set the camera's default position
	position = _mm_set_ps(0, 0, ((float)GraphicsStuff::VOXEL_GRID_WIDTH / 2) + 2, 0);

	// Set the camera's default rotation
	coreRotationQuaternion = DirectX::XMQuaternionRotationRollPitchYaw(0, 0, 0);
	coreRotationEuler = DirectX::XMFLOAT3(0, 0, 0);

	// Setup the local "up"-vector
	DirectX::XMVECTOR localUp = _mm_set_ps(0, 0, 1, 0);

	// Define a default focal position
	DirectX::XMVECTOR lookAt = _mm_set_ps(0, 1, 0, 0);

	// Rotate the focal-position + local-Up vectors to match the camera orientation
	lookAt = DirectX::XMVector3Rotate(lookAt, coreRotationQuaternion);
	localUp = DirectX::XMVector3Rotate(localUp, coreRotationQuaternion);

	// Extract the look-direction vector by normalizing the focal position (only doable
	// because we haven't shifted it in front of the camera yet, so it's still technically
	// an origin-relative direction)
	lookInfo.lookDirNormal = DirectX::XMVector3Normalize(lookAt);

	// Translate the focal position so that it's "in front of" the camera (e.g. visible)
	lookAt = _mm_add_ps(position, lookAt);

	// Cache the non-normalized focal position
	lookInfo.focalPos = lookAt;

	// Create the view matrix with [position], [lookAt], and [localUp]
	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, localUp);

	// Create the camera's viewfinder (screen rect)
	viewfinder = new ScreenRect(AthruRendering::RenderServiceCentre::AccessD3D()->GetDevice());

	// Initialise the spinSpeed modifier for mouse-look
	spinSpeed = 1.8f;
}

Camera::~Camera()
{
	viewfinder->~ScreenRect();
}

void Camera::Update()
{
	// Cache a local reference to the Input service
	Input* localInput = AthruUtilities::UtilityServiceCentre::AccessInput();

	// Translate the view in-game with WASD
	float speed = 10;
	if (localInput->IsKeyDown(87))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, speed * TimeStuff::deltaTime(), 0, 0), coreRotationQuaternion));
	}

	if (localInput->IsKeyDown(65))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, 0, (speed * TimeStuff::deltaTime()) * -1), coreRotationQuaternion));
	}

	if (localInput->IsKeyDown(83))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, (speed * TimeStuff::deltaTime()) * -1, 0, 0), coreRotationQuaternion));
	}

	if (localInput->IsKeyDown(68))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, 0, speed * TimeStuff::deltaTime()), coreRotationQuaternion));
	}

	if (localInput->IsKeyDown(32))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, speed * TimeStuff::deltaTime(), 0), coreRotationQuaternion));
	}

	if (localInput->IsKeyDown(17))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, (speed * TimeStuff::deltaTime()) * -1, 0), coreRotationQuaternion));
	}

	// Rotate the view with mouse input
	this->MouseLook(localInput);

	// Update the view matrix to reflect
	// the translation + rotation above
	this->RefreshViewData();
}

void Camera::Translate(DirectX::XMVECTOR displacement)
{
	// Translate the software camera
	DirectX::XMVECTOR positionCopy = position;
	position = _mm_add_ps(positionCopy, displacement);
}

DirectX::XMVECTOR Camera::GetTranslation()
{
	return position;
}

void Camera::SetRotation(DirectX::XMFLOAT3 eulerAngles)
{
	// Rotate the software camera
	coreRotationEuler = eulerAngles;
	coreRotationQuaternion = DirectX::XMQuaternionRotationRollPitchYaw(coreRotationEuler.x, coreRotationEuler.y, coreRotationEuler.z);
}

DirectX::XMVECTOR Camera::GetRotationQuaternion()
{
	return coreRotationQuaternion;
}

DirectX::XMFLOAT3 Camera::GetRotationEuler()
{
	return coreRotationEuler;
}

void Camera::MouseLook(Input* inputPttr)
{
	DirectX::XMFLOAT2 currMousePos = inputPttr->GetMousePos();
	float mouseDispX = (currMousePos.x - (GraphicsStuff::DISPLAY_WIDTH / 2));
	float mouseDispY = (currMousePos.y - (GraphicsStuff::DISPLAY_HEIGHT / 2));

	// Calculate magnitude
	float dispMag = sqrt(mouseDispX * mouseDispX + mouseDispY * mouseDispY);

	// Normalize x-displacement (only if [mouseDispX] is nonzero)
	if (mouseDispX != 0)
	{
		mouseDispX /= dispMag;
	}

	// Normalize y-displacement (only if [mouseDispY] is nonzero)
	if (mouseDispY != 0)
	{
		mouseDispY /= dispMag;
	}

	// Rotate the camera through mouse XY coordinates
	float xRotationDisp = spinSpeed * mouseDispY * TimeStuff::deltaTime();
	float yRotationDisp = spinSpeed * mouseDispX * TimeStuff::deltaTime();

	// We're using an incremental mouse look now, so add the current rotation
	// into the displacements generated above before setting the new camera angles
	SetRotation(DirectX::XMFLOAT3(coreRotationEuler.x + xRotationDisp,
								  coreRotationEuler.y + yRotationDisp,
								  0));

	SetCursorPos(GetSystemMetrics(SM_CXSCREEN) / 2, GetSystemMetrics(SM_CYSCREEN) / 2);
}

void Camera::RefreshViewData()
{
	// Setup the local "up"-vector
	DirectX::XMVECTOR localUp = _mm_set_ps(0, 0, 1, 0);

	// Define a default focal position
	DirectX::XMVECTOR lookAt = _mm_set_ps(0, 1, 0, 0);

	// Rotate the focal-position + local-Up vectors to match the camera orientation
	lookAt = DirectX::XMVector3Rotate(lookAt, coreRotationQuaternion);
	localUp = DirectX::XMVector3Rotate(localUp, coreRotationQuaternion);

	// Extract the look-direction vector by normalizing the focal position (only doable
	// because we haven't shifted it in front of the camera yet, so it's still technically
	// an origin-relative direction)
	lookInfo.lookDirNormal = DirectX::XMVector3Normalize(lookAt);

	// Translate the focal position so that it's "in front of" the camera (e.g. visible)
	lookAt = _mm_add_ps(position, lookAt);

	// Cache the non-normalized focal position
	lookInfo.focalPos = lookAt;

	// Create the view matrix with [position], [lookAt], and [localUp]
	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, localUp);
}

DirectX::XMMATRIX Camera::GetViewMatrix()
{
	return viewMatrix;
}

CameraLookData Camera::GetLookData()
{
	return lookInfo;
}

ScreenRect* Camera::GetViewFinder()
{
	return viewfinder;
}

// Push constructions for this class through Athru's custom allocator
void* Camera::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Camera>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Camera::operator delete(void* target)
{
	return;
}