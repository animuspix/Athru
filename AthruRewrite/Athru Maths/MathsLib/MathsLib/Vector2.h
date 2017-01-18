#pragma once

class Vector2
{
	public:
		Vector2(float x, float y);
		~Vector2();

		// Overload subscript operators for iterative access

	private:
		float vectorData[2];
};

