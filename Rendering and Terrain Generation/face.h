#pragma once

class ColourMap;

class Face {
	public:
		// getters, setters

		void getPixelXCount();
		void getColourMap();
		void getWidth3D();

	private:
		short pixelXCount;

		// width3D is the width of the face in the worldspace (standardized to 1M)

		// pixelXCount is the pixel count across each face, irrespective of how much
		// those pixels are stretched and interpolated to fit the width3D

		short width3D;

		// Colours should be in the [RGBA] space for best transparency support

		// colourMapLocation is the location of the file holding the pixel values
		// for the given face

		// ColourMap should be a separate class, carrying a pixelWidth*pixelWidth
		// array of [Pixel]s; each [Pixel] is an array of four one-byte segments 
		// (one byte has a range of 2^8, thus each channel needs at least one 
		// byte for the standard 256^4 RGBA gamut), which we'll allocate using an
		// [int] pointer ([int]s consume 4 bytes, so an [int] pointer has exactly
		// one byte-per-channel in the RGBA colour space)

		// [colours] points to a [ColourMap] instance holding the colors for a 
		// given [Face], just as [boxFaces] points to six [Face]s holding the
		// actual data for each [box]

		ColourMap * colours;
};