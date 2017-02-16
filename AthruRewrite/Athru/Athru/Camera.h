#pragma once

#include "directxmath.h"
//#include "SQTTransformer.h"

class Camera
{
	public:
		Camera();
		~Camera();

		//Vector3 GetTranslation();
		//Quaternion GetRotation();
		DirectX::XMVECTOR GetTranslation();
		DirectX::XMVECTOR GetRotation();
		DirectX::XMMATRIX GetViewMatrix();

		//void Translate(float& x, float& y, float& z);
		void Translate(__m128 displacement);
		//void SetRotation(Vector3& axis, float& angleInRads);
		void SetRotation(__m128 eulerAnglesInRads);
		void RefreshViewMatrix();

	private:
		//SQTTransformer transformData;
		//Matrix4 viewMatrix;
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR rotation;
		DirectX::XMMATRIX viewMatrix;
};

