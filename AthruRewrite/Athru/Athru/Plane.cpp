#include "Plane.h"

Plane::Plane()
{
	point = _mm_set_ps(0, 0, 0, 0);
	normal = _mm_set_ps(0, 1, 0, 0);
	innerVector = _mm_set_ps(0, 0, -1, 0);
}

Plane::Plane(DirectX::XMVECTOR basePoint,
			 DirectX::XMVECTOR baseNormal) :
			 point(basePoint),
			 normal(baseNormal)
{
	innerVector = DirectX::XMVector3Cross(point, normal);
}

Plane::~Plane()
{
}

DirectX::XMVECTOR& Plane::FetchPoint()
{
	return point;
}

DirectX::XMVECTOR & Plane::FetchNormal()
{
	return normal;
}

DirectX::XMVECTOR Plane::GetInnerVector()
{
	return innerVector;
}

void Plane::Translate(DirectX::XMVECTOR displacement)
{
	point = _mm_add_ps(point, displacement);
}

void Plane::Rotate(DirectX::XMVECTOR rotationQuaternion)
{
	normal = DirectX::XMVector3Rotate(normal, rotationQuaternion);
}

void Plane::UpdateInnerVector()
{
	innerVector = DirectX::XMVector3Cross(point, normal);
}

bool Plane::pointBelow(DirectX::XMVECTOR point)
{
	return (_mm_cvtss_f32(DirectX::XMVector3Dot(innerVector, point)) < 0);
}

bool Plane::pointAbove(DirectX::XMVECTOR point)
{
	return (_mm_cvtss_f32(DirectX::XMVector3Dot(innerVector, point)) > 0);
}