#include "stdafx.h"
#include "Line.h"

Line::Line(Vector2 suppliedStart, Vector2 suppliedEnd, float suppliedThicknessInPx)
{
	start = suppliedStart;
	end = suppliedEnd;
	thicknessInPx = suppliedThicknessInPx;
}

Line::~Line()
{

}
