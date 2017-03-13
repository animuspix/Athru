#pragma once

#include <directxmath.h>

class Plane
{
	public:
		Plane();
		Plane(DirectX::XMVECTOR point, DirectX::XMVECTOR normal);
		~Plane();

		DirectX::XMVECTOR& FetchPoint();
		DirectX::XMVECTOR& FetchNormal();
		DirectX::XMVECTOR GetInnerVector();

		void Translate(DirectX::XMVECTOR displacement);
		void Rotate(DirectX::XMVECTOR rotationQuaternion);
		void UpdateInnerVector();
		bool pointBelow(DirectX::XMVECTOR point);
		bool pointAbove(DirectX::XMVECTOR point);

	private:
		// The corner of the plane, used to track it's position
		DirectX::XMVECTOR point;

		// The normal used to define the orientation of the plane
		DirectX::XMVECTOR normal;

		// A vector emerging from the positive side of the plane
		// (if the plane is oriented downward) or the negative side
		// (if the plane is oriented upward)
		// Referred to as the "inner vector" because it always points
		// "behind" the plane
		DirectX::XMVECTOR innerVector;

};

