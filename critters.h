#pragma once

// critters are derived from AICharacters
// everything unique about them can be 
// contained in their constructor, so 
// there's not much need for explicit
// declaration at this level

#include "AICharacters.h"

class Critter : AICharacter {
	public:
		Critter();
		~Critter();
};