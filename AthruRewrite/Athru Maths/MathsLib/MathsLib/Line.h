#pragma once

#include "Vector3.h"

class Line
{
	public:
		Line(Vector3 suppliedStart, Vector3 suppliedEnd, float suppliedThicknessInPx);
		~Line();

	private:
		Vector3 start;
		Vector3 end;
		float thicknessInPx;
};

