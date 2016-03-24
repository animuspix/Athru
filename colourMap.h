#pragma once

class Pixel;

class ColourMap {
	public:

		// Modify individual pixels

		void setPixel(short pixelIndex, char pixelValue);

		char * getPixel(short pixelIndex);

		// Constructors, destructors

		ColourMap(char * colourMapLocation, short colourMapWidth);
		~ColourMap();

	private:

		// The map itself, dynamically 
		// given a 
		// [pixelWidth] * [pixelWidth]
		// array on the heap 
		// (heap arrays can have
		// variable length)

		// The data contained within
		// the array assigned to 
		// [pixelPointer] is read
		// out of a file accessed
		// through the 
		// [colourMapLocation] 
		// c-string

		Pixel * pixelPointer;
};