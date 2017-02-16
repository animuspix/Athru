#pragma once

#include "directxmath.h"
#include "SQTTransformer.h"

class Camera
{
	public:
		Camera();
		~Camera();

		Vector3 GetTranslation();
		Quaternion GetRotation();
		DirectX::XMMATRIX GetViewMatrix();

		void Translate(float& x, float& y, float& z);
		void SetRotation(Vector3& axis, float& angleInRads);
		void RefreshViewMatrix();

	private:
		SQTTransformer transformData;
		Matrix4 viewMatrix;
};

