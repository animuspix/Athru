#pragma once

#include <directxmath.h>
#include "Graphics.h"

class Camera
{
	public:
		Camera(DirectX::XMMATRIX& projectorMatrix);
		~Camera();

		void Translate(DirectX::XMVECTOR displacement);
		DirectX::XMVECTOR GetTranslation();

		void SetRotation(float eulerX, float eulerY, float eulerZ);
		DirectX::XMVECTOR GetRotation();

		void RefreshViewMatrix(DirectX::XMMATRIX& projectorMatrix);
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
		// conplane into incremental camera rotations applied
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

			// Construct the initial frustum planes from a given frustum matrix
			// (defined as the combination of the view matrix with a given
			// projection matrix)
			Frustum(DirectX::XMMATRIX frustumMatrix)
			{
				// Our plane-array exists on the stack and will have been safely
				// initialised by default, but the planes themselves won't match
				// up with the frustum defined by the combination of the view +
				// projection matrices; to ensure they do, we need to "freshen"
				// them by explicitly initialising each plane with values
				// extracted from the "frustum matrix"
				// (see: http://www.chadvernon.com/blog/resources/directx9/frustum-culling/
				// for a description of the [view * projection] matrix (aka the "frustum matrix")
				// and how it connects to the planes that actually make up the frustum)
				FreshenPlanes(frustumMatrix);
			}

			// Update the frustum planes with a fresher frustum matrix
			void Update(DirectX::XMMATRIX frustumMatrix)
			{
				FreshenPlanes(frustumMatrix);
			}

			// Check if any of the given points intersect with the camera frustum
			bool CheckIntersection(DirectX::XMVECTOR vert0, DirectX::XMVECTOR vert1,
								   DirectX::XMVECTOR vert2, DirectX::XMVECTOR vert3,
								   DirectX::XMVECTOR vert4, DirectX::XMVECTOR vert5,
								   DirectX::XMVECTOR vert6, DirectX::XMVECTOR vert7)
			{
				return PointIntersection(vert0) || PointIntersection(vert1) ||
					   PointIntersection(vert2) || PointIntersection(vert3) ||
					   PointIntersection(vert4) || PointIntersection(vert5) ||
					   PointIntersection(vert6) || PointIntersection(vert7);
			}

			// Array containing the near, far, left, right, upper, and lower planes
			// of the frustum
			DirectX::XMVECTOR frustumPlanes[6];

			private:
				enum class PLANE_TYPES
				{
					LEFT_PLANE,
					RIGHT_PLANE,
					UPPER_PLANE,
					LOWER_PLANE,
					NEAR_PLANE,
					FAR_PLANE
				};

				// Extract frustum planes from the "frustum matrix", a combination of the view and
				// a given projection matrix
				// Based on the algorithm used here:
				// http://www.chadvernon.com/blog/resources/directx9/frustum-culling/
				void FreshenPlanes(DirectX::XMMATRIX frustumMatrix)
				{
					// Extract rows from the frustum matrix (calculated by multiplying
					// the view matrix with a projection matrix)
					DirectX::XMVECTOR* frustumCoords = frustumMatrix.r;

					// Shuffle each component from each row into it's own vector so
					// we can perform component-wise operations without leaving the SSE
					// registers
					DirectX::XMVECTOR frustumRow0XXXX = _mm_shuffle_ps(frustumCoords[0], frustumCoords[0], _MM_SHUFFLE(0, 0, 0, 0));
					DirectX::XMVECTOR frustumRow0YYYY = _mm_shuffle_ps(frustumCoords[0], frustumCoords[0], _MM_SHUFFLE(1, 1, 1, 1));
					DirectX::XMVECTOR frustumRow0ZZZZ = _mm_shuffle_ps(frustumCoords[0], frustumCoords[0], _MM_SHUFFLE(2, 2, 2, 2));
					DirectX::XMVECTOR frustumRow0WWWW = _mm_shuffle_ps(frustumCoords[0], frustumCoords[0], _MM_SHUFFLE(3, 3, 3, 3));

					DirectX::XMVECTOR frustumRow1XXXX = _mm_shuffle_ps(frustumCoords[1], frustumCoords[1], _MM_SHUFFLE(0, 0, 0, 0));
					DirectX::XMVECTOR frustumRow1YYYY = _mm_shuffle_ps(frustumCoords[1], frustumCoords[1], _MM_SHUFFLE(1, 1, 1, 1));
					DirectX::XMVECTOR frustumRow1ZZZZ = _mm_shuffle_ps(frustumCoords[1], frustumCoords[1], _MM_SHUFFLE(2, 2, 2, 2));
					DirectX::XMVECTOR frustumRow1WWWW = _mm_shuffle_ps(frustumCoords[1], frustumCoords[1], _MM_SHUFFLE(3, 3, 3, 3));

					DirectX::XMVECTOR frustumRow2XXXX = _mm_shuffle_ps(frustumCoords[2], frustumCoords[2], _MM_SHUFFLE(0, 0, 0, 0));
					DirectX::XMVECTOR frustumRow2YYYY = _mm_shuffle_ps(frustumCoords[2], frustumCoords[2], _MM_SHUFFLE(1, 1, 1, 1));
					DirectX::XMVECTOR frustumRow2ZZZZ = _mm_shuffle_ps(frustumCoords[2], frustumCoords[2], _MM_SHUFFLE(2, 2, 2, 2));
					DirectX::XMVECTOR frustumRow2WWWW = _mm_shuffle_ps(frustumCoords[2], frustumCoords[2], _MM_SHUFFLE(3, 3, 3, 3));

					DirectX::XMVECTOR frustumRow3XXXX = _mm_shuffle_ps(frustumCoords[3], frustumCoords[3], _MM_SHUFFLE(0, 0, 0, 0));
					DirectX::XMVECTOR frustumRow3YYYY = _mm_shuffle_ps(frustumCoords[3], frustumCoords[3], _MM_SHUFFLE(1, 1, 1, 1));
					DirectX::XMVECTOR frustumRow3ZZZZ = _mm_shuffle_ps(frustumCoords[3], frustumCoords[3], _MM_SHUFFLE(2, 2, 2, 2));
					DirectX::XMVECTOR frustumRow3WWWW = _mm_shuffle_ps(frustumCoords[3], frustumCoords[3], _MM_SHUFFLE(3, 3, 3, 3));

					// Generate each plane value from the components extracted above
					DirectX::XMVECTOR frustumPlane0CoordZero = _mm_add_ps(frustumRow0WWWW, frustumRow0XXXX);
					DirectX::XMVECTOR frustumPlane0CoordOne = _mm_add_ps(frustumRow1WWWW, frustumRow1XXXX);
					DirectX::XMVECTOR frustumPlane0CoordTwo = _mm_add_ps(frustumRow2WWWW, frustumRow2XXXX);
					DirectX::XMVECTOR frustumPlane0CoordThree = _mm_add_ps(frustumRow3WWWW, frustumRow3XXXX);

					DirectX::XMVECTOR frustumPlane1CoordZero = _mm_sub_ps(frustumRow0WWWW, frustumRow0XXXX);
					DirectX::XMVECTOR frustumPlane1CoordOne = _mm_sub_ps(frustumRow2WWWW, frustumRow1XXXX);
					DirectX::XMVECTOR frustumPlane1CoordTwo = _mm_sub_ps(frustumRow1WWWW, frustumRow2XXXX);
					DirectX::XMVECTOR frustumPlane1CoordThree = _mm_sub_ps(frustumRow3WWWW, frustumRow3XXXX);

					DirectX::XMVECTOR frustumPlane2CoordZero = _mm_sub_ps(frustumRow0WWWW, frustumRow0YYYY);
					DirectX::XMVECTOR frustumPlane2CoordOne = _mm_sub_ps(frustumRow1WWWW, frustumRow1YYYY);
					DirectX::XMVECTOR frustumPlane2CoordTwo = _mm_sub_ps(frustumRow2WWWW, frustumRow2YYYY);
					DirectX::XMVECTOR frustumPlane2CoordThree = _mm_sub_ps(frustumRow3WWWW, frustumRow3YYYY);

					DirectX::XMVECTOR frustumPlane3CoordZero = _mm_add_ps(frustumRow0WWWW, frustumRow0YYYY);
					DirectX::XMVECTOR frustumPlane3CoordOne = _mm_add_ps(frustumRow1WWWW, frustumRow1YYYY);
					DirectX::XMVECTOR frustumPlane3CoordTwo = _mm_add_ps(frustumRow2WWWW, frustumRow2YYYY);
					DirectX::XMVECTOR frustumPlane3CoordThree = _mm_add_ps(frustumRow3WWWW, frustumRow3YYYY);

					DirectX::XMVECTOR frustumPlane4CoordZero = frustumRow0ZZZZ;
					DirectX::XMVECTOR frustumPlane4CoordOne = frustumRow1ZZZZ;
					DirectX::XMVECTOR frustumPlane4CoordTwo = frustumRow2ZZZZ;
					DirectX::XMVECTOR frustumPlane4CoordThree = frustumRow3ZZZZ;

					DirectX::XMVECTOR frustumPlane5CoordZero = _mm_sub_ps(frustumRow0WWWW, frustumRow0ZZZZ);
					DirectX::XMVECTOR frustumPlane5CoordOne = _mm_sub_ps(frustumRow1WWWW, frustumRow1ZZZZ);
					DirectX::XMVECTOR frustumPlane5CoordTwo = _mm_sub_ps(frustumRow2WWWW, frustumRow2ZZZZ);
					DirectX::XMVECTOR frustumPlane5CoordThree = _mm_sub_ps(frustumRow3WWWW, frustumRow3ZZZZ);

					// Blend the plane values from above into a set of six (one for each plane in the frustum)
					// 4D vectors representing plane equations
					frustumPlanes[0] = _mm_blend_ps(frustumPlane0CoordZero, frustumPlane0CoordOne, 0b0100);
					frustumPlanes[0] = _mm_blend_ps(frustumPlanes[0], frustumPlane0CoordTwo, 0b0010);
					frustumPlanes[0] = _mm_blend_ps(frustumPlanes[0], frustumPlane0CoordThree, 0b001);

					frustumPlanes[1] = _mm_blend_ps(frustumPlane1CoordZero, frustumPlane1CoordOne, 0b0100);
					frustumPlanes[1] = _mm_blend_ps(frustumPlanes[1], frustumPlane1CoordTwo, 0b0010);
					frustumPlanes[1] = _mm_blend_ps(frustumPlanes[1], frustumPlane1CoordThree, 0b001);

					frustumPlanes[2] = _mm_blend_ps(frustumPlane2CoordZero, frustumPlane2CoordOne, 0b0100);
					frustumPlanes[2] = _mm_blend_ps(frustumPlanes[2], frustumPlane2CoordTwo, 0b0010);
					frustumPlanes[2] = _mm_blend_ps(frustumPlanes[2], frustumPlane2CoordThree, 0b001);

					frustumPlanes[3] = _mm_blend_ps(frustumPlane3CoordZero, frustumPlane3CoordOne, 0b0100);
					frustumPlanes[3] = _mm_blend_ps(frustumPlanes[3], frustumPlane3CoordTwo, 0b0010);
					frustumPlanes[3] = _mm_blend_ps(frustumPlanes[3], frustumPlane3CoordThree, 0b001);

					frustumPlanes[4] = _mm_blend_ps(frustumPlane4CoordZero, frustumPlane4CoordOne, 0b0100);
					frustumPlanes[4] = _mm_blend_ps(frustumPlanes[4], frustumPlane4CoordTwo, 0b0010);
					frustumPlanes[4] = _mm_blend_ps(frustumPlanes[4], frustumPlane4CoordThree, 0b001);

					frustumPlanes[5] = _mm_blend_ps(frustumPlane5CoordZero, frustumPlane5CoordOne, 0b0100);
					frustumPlanes[5] = _mm_blend_ps(frustumPlanes[5], frustumPlane5CoordTwo, 0b0010);
					frustumPlanes[5] = _mm_blend_ps(frustumPlanes[5], frustumPlane5CoordThree, 0b001);

					// Normalize each frustum plane so our intersection checks (which depend on the dot-product)
					// return valid results
					frustumPlanes[0] = DirectX::XMPlaneNormalize(frustumPlanes[0]);
					frustumPlanes[1] = DirectX::XMPlaneNormalize(frustumPlanes[1]);
					frustumPlanes[2] = DirectX::XMPlaneNormalize(frustumPlanes[2]);
					frustumPlanes[3] = DirectX::XMPlaneNormalize(frustumPlanes[3]);
					frustumPlanes[4] = DirectX::XMPlaneNormalize(frustumPlanes[4]);
					frustumPlanes[5] = DirectX::XMPlaneNormalize(frustumPlanes[5]);
				}

				// Helper function; checks if a given point sits "in front" of every plane in the frustum and
				// returns the result
				bool PointIntersection(DirectX::XMVECTOR point)
				{
					bool leftPlaneIntersection = _mm_cvtss_f32(DirectX::XMPlaneDotCoord(frustumPlanes[0], point)) < 0;
					bool rightPlaneIntersection = _mm_cvtss_f32(DirectX::XMPlaneDotCoord(frustumPlanes[1], point)) < 0;
					bool upperPlaneIntersection = _mm_cvtss_f32(DirectX::XMPlaneDotCoord(frustumPlanes[2], point)) < 0;
					bool lowerPlaneIntersection = _mm_cvtss_f32(DirectX::XMPlaneDotCoord(frustumPlanes[3], point)) < 0;
					bool nearPlaneIntersection = _mm_cvtss_f32(DirectX::XMPlaneDotCoord(frustumPlanes[4], point)) < 0;
					bool farPlaneIntersection = _mm_cvtss_f32(DirectX::XMPlaneDotCoord(frustumPlanes[5], point)) < 0;

					bool planeIntersections = leftPlaneIntersection && rightPlaneIntersection &&
											  upperPlaneIntersection && lowerPlaneIntersection &&
											  nearPlaneIntersection && farPlaneIntersection;

					return planeIntersections;
				}
		};

		// Frustum instance (statics break everything, singletons feel hacky in C++)
		Frustum frustum;
};

