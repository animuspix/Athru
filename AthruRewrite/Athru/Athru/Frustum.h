#pragma once

#include <directxmath.h>
#include "Typedefs.h"
#include "Plane.h"

class Frustum
{
	public:
		Frustum();
		~Frustum();

		void Update(DirectX::XMVECTOR cameraPos, DirectX::XMVECTOR cameraRotQuaternion);
		bool CheckIntersection(DirectX::XMVECTOR vert0, DirectX::XMVECTOR vert1,
							   DirectX::XMVECTOR vert2, DirectX::XMVECTOR vert3,
							   DirectX::XMVECTOR vert4, DirectX::XMVECTOR vert5,
							   DirectX::XMVECTOR vert6, DirectX::XMVECTOR vert7);

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

		// Helper function; checks if any of the provided points are below the specified
		// plane and returns the result
		bool isAPointBelow(byteUnsigned planeIndex);

		// Helper function; checks if a point is above the specified plane and
		// returns the result
		bool isAPointAbove(byteUnsigned planeIndex);

		// Array containing the near, far, left, right, upper, and lower planes
		// of the frustum
		Plane frustumPlanes[6];

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

