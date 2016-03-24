#pragma once

// Rapid access with nearly no processing, so [struct]s are most efficient 

struct Pixel {
	
	// Channels

	// All [char] because characters have 2^8 range rather than 2^16 ([short])
	// or 2^32 ([int])
	
	char red;
	char green;
	char blue;
	char alpha;

	// Constructors, destructors

	Pixel(char redChannel, char greenChannel, char blueChannel, char alphaChannel);
	~Pixel();
};