#pragma once

#include <directxmath.h>

class Camera
{
	public:
		Camera();
		~Camera();

		void Translate(DirectX::XMVECTOR displacement);
		DirectX::XMVECTOR GetTranslation();

		void SetRotation(float eulerX, float eulerY, float eulerZ);
		DirectX::XMVECTOR GetRotation();

		void RefreshViewMatrix();
		DirectX::XMMATRIX GetViewMatrix();

		void MouseLook(Input* inputPttr);

	private:
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR rotationQuaternion;
		DirectX::XMFLOAT3 rotationEuler;
		DirectX::XMMATRIX viewMatrix;
		// Frustum data here...

		// The mouse position (smoothed) captured last time
		// MouseLook(...) was called
		// Used to calculate mouse displacements that easily
		// convert into incremental camera rotations applied
		// by the MouseLook(...) function
		DirectX::XMFLOAT2 lastMousePos;

		// How quickly the camera should rotate after accounting
		// for displacement and deltatime (only used by MouseLook(...))
		float speed;
};

