#pragma once

class camera {
	public:
		void clearScreen();
		void fillScreen();

		void make2DProjection();
		char * get2DProjection();
		
	private:
		double viewWidthRads;
		
		float viewDepthMeters;
		float viewHeight;

		char * viewShape;
};