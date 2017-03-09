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
		
		// Simple frustum collision check, used to evaluate whether
		// or not the given [Boxecule] contains points within the
		// viewing frustum
		bool IsIntersecting(Boxecule* item);

		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR rotationQuaternion;
		DirectX::XMFLOAT3 rotationEuler;
		DirectX::XMMATRIX viewMatrix;
		
		// The near and far planes of the camera frustum
		DirectX::XMVECTOR farPlanePos;
		DirectX::XMVECTOR farPlaneNormal;

		DirectX::XMVECTOR nearPlanePos;
		DirectX::XMVECTOR nearPlaneNormal;

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

