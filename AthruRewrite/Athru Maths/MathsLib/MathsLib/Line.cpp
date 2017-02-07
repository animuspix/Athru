#include "stdafx.h"
#include "Line.h"

Line::Line(Vector3 suppliedStart, Vector3 suppliedEnd, float suppliedThicknessInPx)
{
	start = suppliedStart;
	end = suppliedEnd;
	thicknessInPx = suppliedThicknessInPx;
}

Line::~Line()
{

}
