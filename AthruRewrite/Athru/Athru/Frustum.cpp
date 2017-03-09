#include "Frustum.h"

Frustum::Frustum()
{

}

Frustum::~Frustum()
{
}

void Frustum::Update(DirectX::XMVECTOR cameraPos, DirectX::XMVECTOR cameraRotQuaternion)
{
	// Move each plane to the current location of the camera encompassing [this]
	frustumPlanes[0].cornerPos = _mm_add_ps(frustumPlanes[0].cornerPos, cameraPos);
	frustumPlanes[1].cornerPos = _mm_add_ps(frustumPlanes[1].cornerPos, cameraPos);
	frustumPlanes[2].cornerPos = _mm_add_ps(frustumPlanes[2].cornerPos, cameraPos);
	frustumPlanes[3].cornerPos = _mm_add_ps(frustumPlanes[3].cornerPos, cameraPos);
	frustumPlanes[4].cornerPos = _mm_add_ps(frustumPlanes[4].cornerPos, cameraPos);
	frustumPlanes[5].cornerPos = _mm_add_ps(frustumPlanes[5].cornerPos, cameraPos);

	// Rotate each plane normal to mirror the orientation of the camera encompassing [this]
	frustumPlanes[0].normal = DirectX::XMVector3Rotate(frustumPlanes[0].normal, cameraRotQuaternion);
	frustumPlanes[1].normal = DirectX::XMVector3Rotate(frustumPlanes[1].normal, cameraRotQuaternion);
	frustumPlanes[2].normal = DirectX::XMVector3Rotate(frustumPlanes[2].normal, cameraRotQuaternion);
	frustumPlanes[3].normal = DirectX::XMVector3Rotate(frustumPlanes[3].normal, cameraRotQuaternion);
	frustumPlanes[4].normal = DirectX::XMVector3Rotate(frustumPlanes[4].normal, cameraRotQuaternion);
	frustumPlanes[5].normal = DirectX::XMVector3Rotate(frustumPlanes[5].normal, cameraRotQuaternion);

	// Regenerate the inner vector (cross product of the plane normal and the plane corner vector, used
	// with the dot-product to test whether or not points lie inside or outside a given plane) within
	// each frustum plane
	frustumPlanes[0].innerVector = DirectX::XMVector3Cross(frustumPlanes[0].cornerPos, frustumPlanes[0].normal);
	frustumPlanes[1].innerVector = DirectX::XMVector3Cross(frustumPlanes[1].cornerPos, frustumPlanes[1].normal);
	frustumPlanes[2].innerVector = DirectX::XMVector3Cross(frustumPlanes[2].cornerPos, frustumPlanes[2].normal);
	frustumPlanes[3].innerVector = DirectX::XMVector3Cross(frustumPlanes[3].cornerPos, frustumPlanes[3].normal);
	frustumPlanes[4].innerVector = DirectX::XMVector3Cross(frustumPlanes[4].cornerPos, frustumPlanes[4].normal);
	frustumPlanes[5].innerVector = DirectX::XMVector3Cross(frustumPlanes[5].cornerPos, frustumPlanes[5].normal);

}

bool Frustum::CheckIntersection(DirectX::XMVECTOR vert0, DirectX::XMVECTOR vert1,
								DirectX::XMVECTOR vert2, DirectX::XMVECTOR vert3,
								DirectX::XMVECTOR vert4, DirectX::XMVECTOR vert5,
								DirectX::XMVECTOR vert6, DirectX::XMVECTOR vert7)
{
	// Copy vectors into class storage so they can be accessed from within [isAPointBelow(..)]
	// and [isAPointAbove(...)]
	tempVert0 = vert0;
	tempVert1 = vert1;
	tempVert2 = vert2;
	tempVert3 = vert3;
	tempVert4 = vert4;
	tempVert5 = vert5;
	tempVert6 = vert6;
	tempVert7 = vert7;

	bool vertIntersectionsNearPlane = isAPointAbove((byteUnsigned)PLANE_TYPES::NEAR_PLANE);
	bool vertIntersectionsFarPlane = isAPointBelow((byteUnsigned)PLANE_TYPES::FAR_PLANE);

	bool vertIntersectionsLeftPlane = isAPointBelow((byteUnsigned)PLANE_TYPES::LEFT_PLANE);
	bool vertIntersectionsRightPlane = isAPointAbove((byteUnsigned)PLANE_TYPES::RIGHT_PLANE);

	bool vertIntersectionsUpperPlane = isAPointBelow((byteUnsigned)PLANE_TYPES::UPPER_PLANE);
	bool vertIntersectionsLowerPlane = isAPointAbove((byteUnsigned)PLANE_TYPES::LOWER_PLANE);

	return vertIntersectionsNearPlane || vertIntersectionsFarPlane ||
		   vertIntersectionsLeftPlane || vertIntersectionsRightPlane ||
		   vertIntersectionsUpperPlane || vertIntersectionsLowerPlane;
}

bool Frustum::isAPointBelow(byteUnsigned planeIndex)
{
	DirectX::XMVECTOR planeCornerPos = frustumPlanes[planeIndex].cornerPos;
	DirectX::XMVECTOR planeNormal = frustumPlanes[planeIndex].normal;
	DirectX::XMVECTOR planeInnerVector = frustumPlanes[planeIndex].innerVector;

	bool vert0Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert0)) > 0);
	bool vert1Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert1)) > 0);
	bool vert2Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert2)) > 0);
	bool vert3Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert3)) > 0);
	bool vert4Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert4)) > 0);
	bool vert5Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert5)) > 0);
	bool vert6Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert6)) > 0);
	bool vert7Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert7)) > 0);

	bool vertIntersections = vert0Intersection || vert1Intersection ||
							 vert2Intersection || vert3Intersection ||
							 vert4Intersection || vert5Intersection ||
							 vert6Intersection || vert7Intersection;

	return vertIntersections;
}

bool Frustum::isAPointAbove(byteUnsigned planeIndex)
{
	DirectX::XMVECTOR planeCornerPos = frustumPlanes[planeIndex].cornerPos;
	DirectX::XMVECTOR planeNormal = frustumPlanes[planeIndex].normal;
	DirectX::XMVECTOR planeInnerVector = frustumPlanes[planeIndex].innerVector;

	bool vert0Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert0)) < 0);
	bool vert1Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert1)) < 0);
	bool vert2Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert2)) < 0);
	bool vert3Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert3)) < 0);
	bool vert4Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert4)) < 0);
	bool vert5Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert5)) < 0);
	bool vert6Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert6)) < 0);
	bool vert7Intersection = (_mm_cvtss_f32(DirectX::XMVector3Dot(planeInnerVector, tempVert7)) < 0);

	bool vertIntersections = vert0Intersection || vert1Intersection ||
							 vert2Intersection || vert3Intersection ||
							 vert4Intersection || vert5Intersection ||
							 vert6Intersection || vert7Intersection;

	return vertIntersections;
}