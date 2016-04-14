#pragma once

#include "materials.h"

class Box : Material {
	// One Box is a single voxel
	// Each box is 1M^3 at the start of the game, and has 
	// 16-bit colour (RGBA) on each side (only one side is 
	// displayed at a time) (colour input can be gradiated)

	// Face values should be stored in files

	// Face properties should go to a separate Face class

	// Each face has 32^3 pixel volume (experimental)

	// Methods include destroy, create, and 
	// getters/setters for each property

	// [Box] is a cipher, designed to handle individual 
	// voxels within the player's view cone; all of it's
	// properties are inherited from [Material], and the
	// above documentation is a little bit outdated but
	// still mostly applicable; just for [Material] 
	// instead of [Box]

	public:
		void create();
		void destroy();

		Box();
		~Box();
};