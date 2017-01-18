#pragma once

#include "Vector2.h"

class Line
{
	public:
		Line(Vector2 suppliedStart, Vector2 suppliedEnd, float suppliedThicknessInPx);
		~Line();

	private:
		Vector2 start;
		Vector2 end;
		float thicknessInPx;
};

