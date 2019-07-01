#include "UtilityServiceCentre.h"
#include "GPUServiceCentre.h"
#include "Camera.h"

Camera::Camera()
{
	// Set the camera's default position
	position = _mm_set_ps(0.0f, -10.0f, 0.0f, 700.0f);

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

	// Initialise the spin-speed modifier for mouse-look
	spinSpeed = 50.0f;

	// Initialise the mouse position to the window centre
	SetCursorPos(GetSystemMetrics(SM_CXSCREEN) / 2,
				 GetSystemMetrics(SM_CYSCREEN) / 2);
}

Camera::~Camera() {}

void Camera::Update()
{
	// Cache a local reference to the Input service
	Input* localInput = AthruCore::Utility::AccessInput();

	// Translate the view in-game with WASD
	float dt = (float)TimeStuff::deltaTime();
	float speed = 400000.0f;
	if (localInput->KeyHeld(87))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, speed * dt, 0, 0), coreRotationQuaternion));
	}

	if (localInput->KeyHeld(65))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, 0, (speed * dt) * -1), coreRotationQuaternion));
	}

	if (localInput->KeyHeld(83))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, (speed * dt) * -1, 0, 0), coreRotationQuaternion));
	}

	if (localInput->KeyHeld(68))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, 0, speed * dt), coreRotationQuaternion));
	}

	if (localInput->KeyHeld(32))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, speed * dt, 0), coreRotationQuaternion));
	}

	if (localInput->KeyHeld(17))
	{
		this->Translate(DirectX::XMVector3Rotate(_mm_set_ps(0, 0, (speed * dt) * -1, 0), coreRotationQuaternion));
	}

	// Rotate the view with mouse input
	// (if enabled, turning mouse look off can be useful for debugging)
	//#define MOUSE_LOOK
	#ifdef MOUSE_LOOK
		// Even if mouse-look is enabled, only apply it when the game window is in the foreground
		HWND athruHandle = AthruCore::Utility::AccessApp()->GetHWND();
		HWND currHandle = GetForegroundWindow();
		if (currHandle == athruHandle)
		{
			this->MouseLook(localInput, athruHandle);
		}
	#endif
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

DirectX::XMVECTOR Camera::GetTranslation() const
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

DirectX::XMVECTOR Camera::GetRotationQuaternion() const
{
	return coreRotationQuaternion;
}

DirectX::XMFLOAT3 Camera::GetRotationEuler() const
{
	return coreRotationEuler;
}

void Camera::MouseLook(Input* inputPttr, HWND hwnd)
{
	// Retrieve mouse position from the input processor
	DirectX::XMFLOAT2 currMousePos = inputPttr->GetMousePos();

	// Calculate displacement from the window centre
	float mouseDeltaX = (currMousePos.x - (GraphicsStuff::DISPLAY_WIDTH / 2));
	float mouseDeltaY = (currMousePos.y - (GraphicsStuff::DISPLAY_HEIGHT / 2));

	// Map the changes in mouse position onto changes in camera rotationQtn
	float dt = (float)TimeStuff::deltaTime();
	float xRotationDelta = spinSpeed * mouseDeltaY * dt;
	float yRotationDelta = spinSpeed * mouseDeltaX * dt;

	// Apply the changes in camera rotationQtn to the camera by incrementing the
	// current X/Y euler angles and passing the results into [SetRotation()]
	SetRotation(DirectX::XMFLOAT3(coreRotationEuler.x + xRotationDelta,
								  coreRotationEuler.y + yRotationDelta,
								  0));

	// Move the cursor back to the window centre
	RECT rc = {};
	LPRECT lpRc = &rc;
	BOOL err = GetWindowRect(hwnd, lpRc);
	assert(err);
	SetCursorPos(rc.left + (std::abs(rc.right - rc.left) / 2),
				 rc.bottom - (std::abs(rc.top - rc.bottom) / 2));
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

DirectX::XMMATRIX Camera::GetViewMatrix() const
{
	return viewMatrix;
}

CameraLookData Camera::GetLookData() const
{
	return lookInfo;
}

// Push constructions for this class through Athru's custom allocator
void* Camera::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<Camera>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Camera::operator delete(void* target)
{
	return;
}