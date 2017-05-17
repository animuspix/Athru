#pragma once

#include <directxmath.h>
#include "ScreenRect.h"

struct CameraLookData
{
	// Normalized look vector; the /direction/ that the camera's
	// currently looking in
	DirectX::XMVECTOR lookDirNormal;

	// Non-normalized focal position; the world-space point at the
	// center of the camera's FOV at any particular time
	DirectX::XMVECTOR focalPos;
};

class Input;
class Camera
{
	public:
		Camera();
		~Camera();
		void Update();

		// Set/get camera world position
		void Translate(DirectX::XMVECTOR displacement);
		DirectX::XMVECTOR GetTranslation();

		// Set camera rotation, extract rotation info in
		// complex (quaternion) or angular (euler) form
		void SetRotation(DirectX::XMFLOAT3 eulerAngles);
		DirectX::XMVECTOR GetRotationQuaternion();
		DirectX::XMFLOAT3 GetRotationEuler();

		// Update the view matrix and look-vector, extract
		// either for external calculations
		void RefreshViewData();
		DirectX::XMMATRIX GetViewMatrix();
		CameraLookData GetLookData();

		// Fetch the viewfinder (screen rect) for post-processing/rendering
		ScreenRect* GetViewFinder();

		// Classical mouse-look function; causes the camera to rotate
		// horizontally (about local-Y) when the mouse moves
		// horizontally in screen space, and vertically (about local-X)
		// when the mouse moves vertically in screen space :)
		void MouseLook(Input* inputPttr);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Virtual camera position, rotation (as a quaternion), rotation
		// (in Euler angles), fixed viewing position (so the player can't
		// travel outside the voxel grid used to store data sampled from
		// the scene), and view-space projection (view matrix)
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR coreRotationQuaternion;
		DirectX::XMFLOAT3 coreRotationEuler;
		DirectX::XMVECTOR fixedViewPosition;
		DirectX::XMMATRIX viewMatrix;

		// Where the camera is _looking_ at any particular time (direction and focal position)
		// Needed for PBR lighting calculations, useful for fast directional
		// render-culling; also useful for some screen-space shaders e.g.
		// simulated depth of field
		CameraLookData lookInfo;

		// A rect (one-sided square) projected onto the screen; used to render the screen
		// texture after post-processing
		ScreenRect* viewfinder;

		// How quickly the camera should rotate after accounting
		// for displacement and deltatime (only used by MouseLook(...))
		float spinSpeed;
};

