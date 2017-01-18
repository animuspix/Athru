#pragma once

class Vector3
{
	public:
		Vector3(float x, float y, float z);
		~Vector3();

		// Overload subscript operators for iterative access

	private:
		float vectorData[3];
};

