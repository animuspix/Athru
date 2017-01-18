#pragma once

class Vector4
{
	public:
		Vector4(float x, float y, float z, float w);
		~Vector4();

		// Overload subscript operators for iterative access

	private:
		float vectorData[4];
};

