#pragma once

// Remember to fill out supporting classes

class zPlane;

// zPlanes should have:
// - height
// - width
// - both as a proportion of window height/width
// - x/y/z position + transformations

// Visual2D objects are projected into zPlanes, which
// are positioned in the world to suit. If the zPlane
// is supposed to be fullscreen, it will have max x,
// max y, and be projected 0 points away from the
// player camera

class GraphicalUI {
	public:
		void drawBox(float floatWidthPercentage, float floatHeightPercentage, char * stringSVGSource, float xPositionOnZPlane, float yPositionOnZPlane, zPlane projectionPlane);
};