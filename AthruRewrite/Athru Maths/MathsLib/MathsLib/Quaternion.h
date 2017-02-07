#pragma once

#include "Matrix4.h"
#include "Vector3.h"

#define EMPTY_QUATERNION Quaternion()

class Quaternion
{
	public:
		Quaternion();
		Quaternion(Vector3 axis, float angleInRads);
		~Quaternion();

		// Retrieve the conjugate of [this] for use in
		// vector rotations; see Game Engine Architecture
		// for an explanation of what conjugates are and
		// how they relate to vector rotation
		Quaternion conjugate();

		// Generate and return a 4x4 rotation matrix having the
		// same angle and axes of rotation as [this]
		Matrix4 asMatrix();

		// Generate a composite rotation between [this]
		// and a given quaternion
		Quaternion BasicBlend(Quaternion blendWith);

		// Generate a composite rotation a given fraction of the
		// way betwen [this] and a given quaternion
		Quaternion SlerpyBlend(Quaternion blendWith, float blendAmount);

		// Increment rotation by a given number of radians
		void IncrementRotation(float amountInRads);

		// Retrieve the angle of the current rotation (in radians)
		float GetAngle();

		// Set the current rotation (in radians)
		void SetRotation(float angleInRads);

		// Set the axis of rotation
		void SetAxis(Vector3 newAxis);

		// Retrieve the axis of rotation
		Vector3 GetAxis();

		// Subscript operator overload for flexible
		// component access (read-only)
		float operator[](char index);

	private:
		float components[4];
		float currentAngle;
		Vector3 currentAxis;
};