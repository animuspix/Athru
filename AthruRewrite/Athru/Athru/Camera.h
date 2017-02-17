#pragma once

#include <directxmath.h>

class Camera
{
	public:
		Camera();
		~Camera();

		void Translate(__m128 displacement);
		void SetRotation(__m128 eulerAnglesInRads);
		DirectX::XMMATRIX& GetViewMatrix();

		void RefreshViewMatrix();

	private:
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR rotation;
		DirectX::XMMATRIX viewMatrix;
};

