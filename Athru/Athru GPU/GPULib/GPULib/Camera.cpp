#include "UtilityServiceCentre.h"
#include "GPUServiceCentre.h"
#include "Camera.h"

Camera::Camera()
{
	// Set the camera's default position
	// Default position should be towards the outer middle of the
	// first star system
	//position = _mm_set_ps(0.0f, -4000.0f, 0.0f, 2000.0f);
	//position = _mm_set_ps(0.0f, -400.0f, 0.0f, 2000.0f);
	position = _mm_set_ps(0.0f, -700.0f, 0.0f, 600.0f);

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
	lookInfo.viewDirNormal = DirectX::XMVector3Normalize(lookAt);

	// Translate the focal position so that it's "in front of" the camera (e.g. visible)
	lookAt = _mm_add_ps(position, lookAt);

	// Cache the non-normalized focal position
	lookInfo.focalPos = lookAt;

	// Create the view matrix with a vector inside the voxel grid, the generated look
	// vector, and [localUp]
	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, localUp);

	// Create the camera's viewfinder (screen rect)
	viewfinder = new ScreenRect(AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice());

	// Initialise the spin-speed modifier for mouse-look
	spinSpeed = 0.5f;

	// Initialise the mouse position to the window centre
	SetCursorPos(GetSystemMetrics(SM_CXSCREEN) / 2,
				 GetSystemMetrics(SM_CYSCREEN) / 2);
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
	float speed = 500;
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
	// (if enabled)
	if (mouseLookActive)
	{
		this->MouseLook(localInput);
	}

	// Update the view matrix to reflect
	// the translation + rotationQtn above
	this->RefreshViewData();
}

void Camera::Translate(DirectX::XMVECTOR displacement)
{
	// Translate the software camera
	DirectX::XMVECTOR positionCopy = position;
	position = _mm_add_ps(positionCopy, displacement);
}

DirectX::XMVECTOR& Camera::GetTranslation()
{
	return position;
}

void Camera::SetRotation(DirectX::XMFLOAT3 eulerAngles)
{
	// Rotate the software camera
	coreRotationEuler = eulerAngles;
	coreRotationQuaternion = DirectX::XMQuaternionRotationRollPitchYaw(coreRotationEuler.x,
																	   coreRotationEuler.y,
																	   coreRotationEuler.z);
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
	// Retrieve mouse position from the input processor
	DirectX::XMFLOAT2 currMousePos = inputPttr->GetMousePos();

	// Calculate displacement from the window centre
	float mouseDeltaX = (currMousePos.x - (GraphicsStuff::DISPLAY_WIDTH / 2));
	float mouseDeltaY = (currMousePos.y - (GraphicsStuff::DISPLAY_HEIGHT / 2));

	// Map the changes in mouse position onto changes in camera rotationQtn
	float xRotationDelta = spinSpeed * mouseDeltaY * TimeStuff::deltaTime();
	float yRotationDelta = spinSpeed * mouseDeltaX * TimeStuff::deltaTime();

	// Apply the changes in camera rotationQtn to the camera by incrementing the
	// current X/Y euler angles and passing the results into [SetRotation()]
	SetRotation(DirectX::XMFLOAT3(coreRotationEuler.x + xRotationDelta,
								  coreRotationEuler.y + yRotationDelta,
								  0));

	// Move the cursor back to the screen centre
	SetCursorPos(GetSystemMetrics(SM_CXSCREEN) / 2,
				 GetSystemMetrics(SM_CYSCREEN) / 2);
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
	lookInfo.viewDirNormal = DirectX::XMVector3Normalize(lookAt);

	// Translate the focal position so that it's "in front of" the camera (e.g. visible)
	lookAt = _mm_add_ps(position, lookAt);

	// Cache the non-normalized focal position
	lookInfo.focalPos = lookAt;

	// Create the view matrix from the viewing position + the look-at and local-up
	// vectors defined above
	viewMatrix = DirectX::XMMatrixLookAtLH(position, lookAt, localUp);
}

DirectX::XMMATRIX& Camera::GetViewMatrix()
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