#pragma once

class fractalPlane {
	public:
		void getShapeWithinSegment();
		void drawInitialProjection();
		void updateProjection();
		
		double zValueAtXY();

		fractalPlane();
		~fractalPlane();
	private:
		short currentWidth;
		short currentLength;
};