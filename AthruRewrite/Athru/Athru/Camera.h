#pragma once

#include <directxmath.h>
#include "Plane.h"
#include "Graphics.h"

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

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Camera position, rotation (as a quaternion), rotation
		// (in Euler angles), and view-space projection (view matrix)
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR rotationQuaternion;
		DirectX::XMFLOAT3 rotationEuler;
		DirectX::XMMATRIX viewMatrix;

		// The mouse position (smoothed) captured last time
		// MouseLook(...) was called
		// Used to calculate mouse displacements that easily
		// convert into incremental camera rotations applied
		// by the MouseLook(...) function
		DirectX::XMFLOAT2 lastMousePos;

		// How quickly the camera should rotate after accounting
		// for displacement and deltatime (only used by MouseLook(...))
		float speed;

		// Camera frustum properties + methods
		class Frustum
		{
			public:
				Frustum() {}
				Frustum(DirectX::XMVECTOR cameraPos, DirectX::XMVECTOR quaternionRotation)
				{
					const float tanHalfFOV = tan(VERT_FIELD_OF_VIEW_RADS);
					const float frustumHeightNear = (SCREEN_NEAR * tanHalfFOV) * 2;
					const float frustumWidthNear = frustumHeightNear * DISPLAY_ASPECT_RATIO;

					const float frustumHeightFar = (SCREEN_FAR * tanHalfFOV) * 2;
					const float frustumWidthFar = frustumWidthNear * DISPLAY_ASPECT_RATIO;

					// Initialise the near plane
					DirectX::XMVECTOR pointOnNearPlane = _mm_set_ps(0, SCREEN_NEAR, 0, 0);
					DirectX::XMVECTOR nearPlaneNormal = _mm_set_ps(0, 0, 1, 0);
					BuildPlane(pointOnNearPlane, nearPlaneNormal, cameraPos, quaternionRotation, (byteSigned)(PLANE_TYPES::NEAR_PLANE));

					// Initialise the far plane
					DirectX::XMVECTOR pointOnFarPlane = _mm_set_ps(0, SCREEN_FAR, 0, 0);
					DirectX::XMVECTOR farPlaneNormal = _mm_set_ps(0, 0, 1, 0);
					BuildPlane(pointOnFarPlane, farPlaneNormal, cameraPos, quaternionRotation, (byteUnsigned)PLANE_TYPES::FAR_PLANE);

					// Initialise the left plane
					DirectX::XMVECTOR pointOnLeftPlane = _mm_set_ps(0, 0, 0, (frustumWidthNear / 2) * -1);
					DirectX::XMVECTOR leftPlaneNormal = _mm_set_ps(0, SCREEN_NEAR, 0, (frustumWidthNear / 2) * -1);
					BuildPlane(pointOnLeftPlane, leftPlaneNormal, cameraPos, quaternionRotation, (byteUnsigned)PLANE_TYPES::LEFT_PLANE);

					// Initialise the right plane
					DirectX::XMVECTOR pointOnRightPlane = _mm_set_ps(0, 0, 0, frustumWidthNear / 2);
					DirectX::XMVECTOR rightPlaneNormal = _mm_set_ps(0, SCREEN_NEAR, 0, (frustumWidthNear / 2));
					BuildPlane(pointOnLeftPlane, leftPlaneNormal, cameraPos, quaternionRotation, (byteUnsigned)PLANE_TYPES::RIGHT_PLANE);

					// Initialise the upper plane
					DirectX::XMVECTOR pointOnUpperPlane = _mm_set_ps(0, 0, frustumHeightNear / 2, 0);
					DirectX::XMVECTOR upperPlaneNormal = _mm_set_ps(0, SCREEN_NEAR, 0, (frustumHeightNear / 2));
					BuildPlane(pointOnLeftPlane, leftPlaneNormal, cameraPos, quaternionRotation, (byteUnsigned)PLANE_TYPES::UPPER_PLANE);

					// Initialise the lower plane
					DirectX::XMVECTOR pointOnLowerPlane = _mm_set_ps(0, 0, (frustumHeightNear / 2) * -1, 0);
					DirectX::XMVECTOR lowerPlaneNormal = _mm_set_ps(0, SCREEN_NEAR, 0, (frustumHeightNear / 2) * -1);
					BuildPlane(pointOnLeftPlane, leftPlaneNormal, cameraPos, quaternionRotation, (byteUnsigned)PLANE_TYPES::LOWER_PLANE);
				}

				void Translate(DirectX::XMVECTOR displacement)
				{
					frustumPlanes[0].Translate(displacement);
					frustumPlanes[1].Translate(displacement);
					frustumPlanes[2].Translate(displacement);
					frustumPlanes[3].Translate(displacement);
					frustumPlanes[4].Translate(displacement);
					frustumPlanes[5].Translate(displacement);
				}

				void Rotate(DirectX::XMVECTOR rotationQuaternion)
				{
					frustumPlanes[0].Rotate(rotationQuaternion);
					frustumPlanes[1].Rotate(rotationQuaternion);
					frustumPlanes[2].Rotate(rotationQuaternion);
					frustumPlanes[3].Rotate(rotationQuaternion);
					frustumPlanes[4].Rotate(rotationQuaternion);
					frustumPlanes[5].Rotate(rotationQuaternion);
				}

				void UpdateInnerVectors()
				{
					frustumPlanes[0].UpdateInnerVector();
					frustumPlanes[1].UpdateInnerVector();
					frustumPlanes[2].UpdateInnerVector();
					frustumPlanes[3].UpdateInnerVector();
					frustumPlanes[4].UpdateInnerVector();
					frustumPlanes[5].UpdateInnerVector();
				}

				bool CheckIntersection(DirectX::XMVECTOR vert0, DirectX::XMVECTOR vert1,
											  DirectX::XMVECTOR vert2, DirectX::XMVECTOR vert3,
											  DirectX::XMVECTOR vert4, DirectX::XMVECTOR vert5,
											  DirectX::XMVECTOR vert6, DirectX::XMVECTOR vert7)
				{
					// Copy vectors into class storage so they can be accessed from within [AnyPointBelow(..)]
					// and [AnyPointAbove(...)]
					tempVert0 = vert0;
					tempVert1 = vert1;
					tempVert2 = vert2;
					tempVert3 = vert3;
					tempVert4 = vert4;
					tempVert5 = vert5;
					tempVert6 = vert6;
					tempVert7 = vert7;

					bool vertIntersectionsNearPlane = AnyPointAbove((byteUnsigned)PLANE_TYPES::NEAR_PLANE);
					bool vertIntersectionsFarPlane = AnyPointBelow((byteUnsigned)PLANE_TYPES::FAR_PLANE);

					bool vertIntersectionsLeftPlane = AnyPointBelow((byteUnsigned)PLANE_TYPES::LEFT_PLANE);
					bool vertIntersectionsRightPlane = AnyPointAbove((byteUnsigned)PLANE_TYPES::RIGHT_PLANE);

					bool vertIntersectionsUpperPlane = AnyPointBelow((byteUnsigned)PLANE_TYPES::UPPER_PLANE);
					bool vertIntersectionsLowerPlane = AnyPointAbove((byteUnsigned)PLANE_TYPES::LOWER_PLANE);

					return vertIntersectionsNearPlane || vertIntersectionsFarPlane ||
						   vertIntersectionsLeftPlane || vertIntersectionsRightPlane ||
						   vertIntersectionsUpperPlane || vertIntersectionsLowerPlane;
				}

			// Array containing the near, far, left, right, upper, and lower planes
			// of the frustum
			Plane frustumPlanes[6];

			private:
				enum class PLANE_TYPES
				{
					NEAR_PLANE,
					FAR_PLANE,
					LEFT_PLANE,
					RIGHT_PLANE,
					UPPER_PLANE,
					LOWER_PLANE
				};

				// Helper function; used to setup each plane during initialisation
				void BuildPlane(DirectX::XMVECTOR point, DirectX::XMVECTOR normal,
									   DirectX::XMVECTOR pos, DirectX::XMVECTOR quaternionRotation,
									   byteUnsigned planeIndex)
				{
					point = _mm_add_ps(pos, point);
					normal = DirectX::XMVector3Rotate(normal, quaternionRotation);
					normal = DirectX::XMVector3Normalize(normal);
					frustumPlanes[(byteSigned)(Frustum::PLANE_TYPES::FAR_PLANE)] = Plane(point, normal);
				}

				// Helper function; checks if any of the provided points are below the specified
				// plane and returns the result
				bool AnyPointBelow(byteUnsigned planeIndex)
				{
					DirectX::XMVECTOR planeInnerVector = frustumPlanes[planeIndex].GetInnerVector();
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

				// Helper function; checks if a point is above the specified plane and
				// returns the result
				bool AnyPointAbove(byteUnsigned planeIndex)
				{
					DirectX::XMVECTOR planeInnerVector = frustumPlanes[planeIndex].GetInnerVector();
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

				// Temporary (refreshed per-frame) stores containing the vertices passed in
				// during calls to [CheckIntersection] (used to avoid wasting memory with
				// separate copies during each call to [isAPointBelow(...)] or [isAPointAbove(...)])
				DirectX::XMVECTOR tempVert0;
				DirectX::XMVECTOR tempVert1;
				DirectX::XMVECTOR tempVert2;
				DirectX::XMVECTOR tempVert3;
				DirectX::XMVECTOR tempVert4;
				DirectX::XMVECTOR tempVert5;
				DirectX::XMVECTOR tempVert6;
				DirectX::XMVECTOR tempVert7;
		};

		// Frustum instance (statics break everything, singletons feel hacky in C++)
		Frustum frustum;
};

